#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>
#include <stdbool.h>
#include <stdio.h>

#include "aurum.h"
#include "aurum_private.h"

// Arduino AVR core therefore uses 16-byte RX/TX buffers by default:
//   RAMEND - RAMSTART < 1023
// 
// The buffers use one slot as a sentinel, so the maximum number of
// simultaneously buffered bytes is SERIAL_*_BUFFER_SIZE - 1.
#define SERIAL_RX_BUFFER_SIZE 16
#define SERIAL_TX_BUFFER_SIZE 16

typedef uint8_t rx_buffer_index_t;
typedef uint8_t tx_buffer_index_t;

// RX buffer
static volatile rx_buffer_index_t _rx_buffer_head = 0;
static volatile rx_buffer_index_t _rx_buffer_tail = 0;
static uint8_t _rx_buffer[SERIAL_RX_BUFFER_SIZE];

// TX buffer
static volatile tx_buffer_index_t _tx_buffer_head = 0;
static volatile tx_buffer_index_t _tx_buffer_tail = 0;
static uint8_t _tx_buffer[SERIAL_TX_BUFFER_SIZE];


// True once at least one byte has been written after begin().
// This corresponds to HardwareSerial::_written in the Arduino AVR core.
static bool _written = false;

void au_serial_begin(uint32_t baud, uint8_t config) {
// Try u2x mode first
  uint16_t baud_setting = (F_CPU / 4 / baud - 1) / 2;
  UCSR0A = 1 << U2X0;

  // hardcoded exception for 57600 for compatibility with the bootloader
  // shipped with the Duemilanove and previous boards and the firmware
  // on the 8U2 on the Uno and Mega 2560. Also, The baud_setting cannot
  // be > 4095, so switch back to non-u2x mode if the baud rate is too
  // low.
  if (((F_CPU == 16000000UL) && (baud == 57600)) || (baud_setting >4095))
  {
    UCSR0A = 0;
    baud_setting = (F_CPU / 8 / baud - 1) / 2;
  }

  // assign the baud_setting, a.k.a. ubrr (USART Baud Rate Register)
  UBRR0H = baud_setting >> 8;
  UBRR0L = baud_setting;

  _written = false;

  //set the data bits, parity, and stop bits
  UCSR0C = config;
  
  sbi(UCSR0B, RXEN0);
  sbi(UCSR0B, TXEN0);
  sbi(UCSR0B, RXCIE0);
  cbi(UCSR0B, UDRIE0);
}

void au_serial_end(void) {
  // wait for transmission of outgoing data
  au_serial_flush();

  cbi(UCSR0B, RXEN0);
  cbi(UCSR0B, TXEN0);
  cbi(UCSR0B, RXCIE0);
  cbi(UCSR0B, UDRIE0);
  
  // clear any received data
  _rx_buffer_head = _rx_buffer_tail;
}

int au_serial_available(void) {
  return ((unsigned int)(SERIAL_RX_BUFFER_SIZE + _rx_buffer_head - _rx_buffer_tail)) % SERIAL_RX_BUFFER_SIZE;
}

int au_serial_peek(void) {
  if (_rx_buffer_head == _rx_buffer_tail) {
    return -1;
  }
  else {
    return _rx_buffer[_rx_buffer_tail];
  }
}

int au_serial_read(void) {
  // if the head isn't ahead of the tail, we don't have any characters
  if (_rx_buffer_head == _rx_buffer_tail) {
    return -1;
  }
  else {
    unsigned char c = _rx_buffer[_rx_buffer_tail];
    _rx_buffer_tail = (rx_buffer_index_t)(_rx_buffer_tail + 1) % SERIAL_RX_BUFFER_SIZE;
    return c;
  }
}

static void _rx_complete_irq(void) {
  if (bit_is_clear(UCSR0A, UPE0)) {
    // No Parity error, read byte and store it in the buffer if there is
    // room
    unsigned char c = UDR0;
    rx_buffer_index_t i = (unsigned int)(_rx_buffer_head + 1) % SERIAL_RX_BUFFER_SIZE;

    // if we should be storing the received character into the location
    // just before the tail (meaning that the head would advance to the
    // current location of the tail), we're about to overflow the buffer
    // and so we don't write the character or advance the head.
    if (i != _rx_buffer_tail) {
      _rx_buffer[_rx_buffer_head] = c;
      _rx_buffer_head = i;
    }
  }
  else {
    // Parity error, read byte but discard it
    UDR0;
  };
}

int au_serial_available_for_write(void) {
  tx_buffer_index_t head;
  tx_buffer_index_t tail;

  head = _tx_buffer_head;
  tail = _tx_buffer_tail;
  if (head >= tail) {
    return SERIAL_TX_BUFFER_SIZE - 1 - head + tail;
  }

  return tail - head - 1;
}

static void _tx_udr_empty_irq(void) {
  // If interrupts are enabled, there must be more data in the output
  // buffer. Send the next byte
  unsigned char c = _tx_buffer[_tx_buffer_tail];
  _tx_buffer_tail = (_tx_buffer_tail + 1) % SERIAL_TX_BUFFER_SIZE;

  UDR0 = c;

  // clear the TXC bit -- "can be cleared by writing a one to its bit
  // location". This makes sure flush() won't return until the bytes
  // actually got written. Other r/w bits are preserved, and zeroes
  // written to the rest.
  UCSR0A = (UCSR0A & (_BV(U2X0) | _BV(MPCM0))) | _BV(TXC0);

  if (_tx_buffer_head == _tx_buffer_tail) {
    // Buffer empty, so disable interrupts
    cbi(UCSR0B, UDRIE0);
  }
}

size_t au_serial_write(uint8_t c) {
  _written = true;
  // If the buffer and the data register is empty, just write the byte
  // to the data register and be done. This shortcut helps
  // significantly improve the effective datarate at high (>
  // 500kbit/s) bitrates, where interrupt overhead becomes a slowdown.
  if (_tx_buffer_head == _tx_buffer_tail && bit_is_set(UCSR0A, UDRE0)) {
    // If TXC is cleared before writing UDR and the previous byte
    // completes before writing to UDR, TXC will be set but a byte
    // is still being transmitted causing flush() to return too soon.
    // So writing UDR must happen first.
    // Writing UDR and clearing TC must be done atomically, otherwise
    // interrupts might delay the TXC clear so the byte written to UDR
    // is transmitted (setting TXC) before clearing TXC. Then TXC will
    // be cleared when no bytes are left, causing flush() to hang
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      UDR0 = c;

      UCSR0A = (UCSR0A & (_BV(U2X0) | _BV(MPCM0))) | _BV(TXC0);
    }
    return 1;
  }

  tx_buffer_index_t i = (_tx_buffer_head + 1) % SERIAL_TX_BUFFER_SIZE;
	
  // If the output buffer is full, there's nothing for it other than to 
  // wait for the interrupt handler to empty it a bit
  while (i == _tx_buffer_tail) {
    if (bit_is_clear(SREG, SREG_I)) {
      // Interrupts are disabled, so we'll have to poll the data
      // register empty flag ourselves. If it is set, pretend an
      // interrupt has happened and call the handler to free up
      // space for us.
      if(bit_is_set(UCSR0A, UDRE0)) {
	      _tx_udr_empty_irq();
      }
    } else {
      // nop, the interrupt handler will free up space for us
    }
  }

  _tx_buffer[_tx_buffer_head] = c;

  // make atomic to prevent execution of ISR between setting the
  // head pointer and setting the interrupt flag resulting in buffer
  // retransmission
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    _tx_buffer_head = i;
    sbi(UCSR0B, UDRIE0);
  }
  
  return 1;
}

// Not yet used
// Porting of Arduino Print::write(buffer, size)
/*static size_t au_serial_write_buffer(const uint8_t *buffer, size_t size) {
  size_t written = 0;

  if (buffer == NULL) {
    return 0;
  }

  while (written < size) {
    au_serial_write(buffer[written]);
    ++written;
  }

  return written;
}*/

void au_serial_flush(void) {
  // If we have never written a byte, no need to flush. This special
  // case is needed since there is no way to force the TXC (transmit
  // complete) bit to 1 during initialization
  if (!_written) {
    return;
  }

  // Wait until:
  // - the TX ring buffer is empty
  // - the final byte has actually finished transmitting
  // UDRIE0 can become disabled as soon as the last byte is moved from
  // the software buffer into UDR0. TXC0 then tells us when transmission
  // on the physical UART is complete.
  while (bit_is_set(UCSR0B, UDRIE0) || bit_is_clear(UCSR0A, TXC0)) {
    // Interrupts are globally disabled, but the DR empty
    // interrupt should be enabled, so poll the DR empty flag to
    // prevent deadlock
    if (bit_is_clear(SREG, SREG_I) && bit_is_set(UCSR0B, UDRIE0)) {
      if (bit_is_set(UCSR0A, UDRE0)) {
        _tx_udr_empty_irq();
      }
    }
  }

  // If we get here, nothing is queued anymore (DRIE is disabled) and
  // the hardware finished transmission (TXC is set).
}

ISR(USART_RX_vect) {
  _rx_complete_irq();
}


ISR(USART_UDRE_vect) {
  _tx_udr_empty_irq();
}

void au_serial_print_str(const char *s) {
  while (*s) {
    au_serial_write((uint8_t)*s++);
  }
}

void au_serial_println_str(const char *s) {
  au_serial_print_str(s);
  au_serial_write('\r');
  au_serial_write('\n');
}

void au_serial_print_uint(uint32_t value) {
  char buffer[10];
  uint8_t i = 0;

  if (value == 0) {
    au_serial_write('0');
    return;
  }

  while (value > 0) {
    buffer[i++] = '0' + (value % 10);
    value /= 10;
  }

  while (i > 0) {
    au_serial_write(buffer[--i]);
  }
}

void au_serial_println_uint(uint32_t value) {
  au_serial_print_uint(value);
  au_serial_write('\r');
  au_serial_write('\n');
}

void au_serial_print_int(int32_t value) {
  if (value < 0) {
    au_serial_write('-');

    au_serial_print_uint((uint32_t)(-(value + 1)) + 1);
  }
  else {
    au_serial_print_uint((uint32_t)value);
  }
}

void au_serial_println_int(int32_t value) {
  au_serial_print_int(value);
  au_serial_write('\r');
  au_serial_write('\n');
}