#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay_basic.h>

#include "aurum/software-serial.h"
#include "aurum_private.h"

#define AU_SOFTWARE_SERIAL_RX_BUFFER_SIZE 64

static uint8_t receive_buffer[AU_SOFTWARE_SERIAL_RX_BUFFER_SIZE];
static volatile uint8_t receive_buffer_head = 0;
static volatile uint8_t receive_buffer_tail = 0;

static au_software_serial_t *active_serial = NULL;

static inline void au_software_serial_tuned_delay(uint16_t delay) {
  _delay_loop_2(delay);
}

static uint16_t subtract_cap(uint16_t num, uint16_t sub) {
  if (num > sub) {
    return num - sub;
  }

  return 1;
}

static inline uint8_t rx_pin_read(au_software_serial_t *serial) {
  return (*serial->rx_port_register & serial->rx_bit_mask);
}

static inline void set_rx_int_mask(au_software_serial_t *serial, bool enable) {
  if (enable) {
    *serial->pcint_mask_register |= serial->pcint_mask_value;
  }
  else {
    *serial->pcint_mask_register &= (uint8_t)~serial->pcint_mask_value;
  }
}

static void set_tx(au_software_serial_t *serial, uint8_t pin) {
  serial->tx_pin = pin;

  switch (pin) {
  case 0:
    serial->tx_bit_mask = _BV(PD0);
    serial->tx_port_register = &PORTD;
    DDRD |= _BV(PD0);
    PORTD |= _BV(PD0);
    break;
  case 1:
    serial->tx_bit_mask = _BV(PD1);
    serial->tx_port_register = &PORTD;
    DDRD |= _BV(PD1);
    PORTD |= _BV(PD1);
    break;
  case 2:
    serial->tx_bit_mask = _BV(PD2);
    serial->tx_port_register = &PORTD;
    DDRD |= _BV(PD2);
    PORTD |= _BV(PD2);
    break;
  case 3:
    serial->tx_bit_mask = _BV(PD3);
    serial->tx_port_register = &PORTD;
    DDRD |= _BV(PD3);
    PORTD |= _BV(PD3);
    break;
  case 4:
    serial->tx_bit_mask = _BV(PD4);
    serial->tx_port_register = &PORTD;
    DDRD |= _BV(PD4);
    PORTD |= _BV(PD4);
    break;
  case 5:
    serial->tx_bit_mask = _BV(PD5);
    serial->tx_port_register = &PORTD;
    DDRD |= _BV(PD5);
    PORTD |= _BV(PD5);
    break;
  case 6:
    serial->tx_bit_mask = _BV(PD6);
    serial->tx_port_register = &PORTD;
    DDRD |= _BV(PD6);
    PORTD |= _BV(PD6);
    break;
  case 7:
    serial->tx_bit_mask = _BV(PD7);
    serial->tx_port_register = &PORTD;
    DDRD |= _BV(PD7);
    PORTD |= _BV(PD7);
    break;
  case 8:
    serial->tx_bit_mask = _BV(PB0);
    serial->tx_port_register = &PORTB;
    DDRB |= _BV(PB0);
    PORTB |= _BV(PB0);
    break;
  case 9:
    serial->tx_bit_mask = _BV(PB1);
    serial->tx_port_register = &PORTB;
    DDRB |= _BV(PB1);
    PORTB |= _BV(PB1);
    break;
  case 10:
    serial->tx_bit_mask = _BV(PB2);
    serial->tx_port_register = &PORTB;
    DDRB |= _BV(PB2);
    PORTB |= _BV(PB2);
    break;
  case 11:
    serial->tx_bit_mask = _BV(PB3);
    serial->tx_port_register = &PORTB;
    DDRB |= _BV(PB3);
    PORTB |= _BV(PB3);
    break;
  case 12:
    serial->tx_bit_mask = _BV(PB4);
    serial->tx_port_register = &PORTB;
    DDRB |= _BV(PB4);
    PORTB |= _BV(PB4);
    break;
  case 13:
    serial->tx_bit_mask = _BV(PB5);
    serial->tx_port_register = &PORTB;
    DDRB |= _BV(PB5);
    PORTB |= _BV(PB5);
    break;
  case 14:
    serial->tx_bit_mask = _BV(PC0);
    serial->tx_port_register = &PORTC;
    DDRC |= _BV(PC0);
    PORTC |= _BV(PC0);
    break;
  case 15:
    serial->tx_bit_mask = _BV(PC1);
    serial->tx_port_register = &PORTC;
    DDRC |= _BV(PC1);
    PORTC |= _BV(PC1);
    break;
  case 16:
    serial->tx_bit_mask = _BV(PC2);
    serial->tx_port_register = &PORTC;
    DDRC |= _BV(PC2);
    PORTC |= _BV(PC2);
    break;
  case 17:
    serial->tx_bit_mask = _BV(PC3);
    serial->tx_port_register = &PORTC;
    DDRC |= _BV(PC3);
    PORTC |= _BV(PC3);
    break;
  case 18:
    serial->tx_bit_mask = _BV(PC4);
    serial->tx_port_register = &PORTC;
    DDRC |= _BV(PC4);
    PORTC |= _BV(PC4);
    break;
  case 19:
    serial->tx_bit_mask = _BV(PC5);
    serial->tx_port_register = &PORTC;
    DDRC |= _BV(PC5);
    PORTC |= _BV(PC5);
    break;
  default:
    serial->tx_port_register = NULL;
    serial->tx_bit_mask = 0;
    break;
  }
}

static bool set_rx(au_software_serial_t *serial, uint8_t pin) {
  serial->rx_pin = pin;

  switch (pin) {
  case 0:
    serial->rx_bit_mask = _BV(PD0);
    serial->rx_port_register = &PIND;
    DDRD &= (uint8_t)~_BV(PD0);
    PORTD |= _BV(PD0);
    serial->pcint_mask_register = &PCMSK2;
    serial->pcint_mask_value = _BV(PCINT16);
    break;
  case 1:
    serial->rx_bit_mask = _BV(PD1);
    serial->rx_port_register = &PIND;
    DDRD &= (uint8_t)~_BV(PD1);
    PORTD |= _BV(PD1);
    serial->pcint_mask_register = &PCMSK2;
    serial->pcint_mask_value = _BV(PCINT17);
    break;
  case 2:
    serial->rx_bit_mask = _BV(PD2);
    serial->rx_port_register = &PIND;
    DDRD &= (uint8_t)~_BV(PD2);
    PORTD |= _BV(PD2);
    serial->pcint_mask_register = &PCMSK2;
    serial->pcint_mask_value = _BV(PCINT18);
    break;
  case 3:
    serial->rx_bit_mask = _BV(PD3);
    serial->rx_port_register = &PIND;
    DDRD &= (uint8_t)~_BV(PD3);
    PORTD |= _BV(PD3);
    serial->pcint_mask_register = &PCMSK2;
    serial->pcint_mask_value = _BV(PCINT19);
    break;
  case 4:
    serial->rx_bit_mask = _BV(PD4);
    serial->rx_port_register = &PIND;
    DDRD &= (uint8_t)~_BV(PD4);
    PORTD |= _BV(PD4);
    serial->pcint_mask_register = &PCMSK2;
    serial->pcint_mask_value = _BV(PCINT20);
    break;
  case 5:
    serial->rx_bit_mask = _BV(PD5);
    serial->rx_port_register = &PIND;
    DDRD &= (uint8_t)~_BV(PD5);
    PORTD |= _BV(PD5);
    serial->pcint_mask_register = &PCMSK2;
    serial->pcint_mask_value = _BV(PCINT21);
    break;
  case 6:
    serial->rx_bit_mask = _BV(PD6);
    serial->rx_port_register = &PIND;
    DDRD &= (uint8_t)~_BV(PD6);
    PORTD |= _BV(PD6);
    serial->pcint_mask_register = &PCMSK2;
    serial->pcint_mask_value = _BV(PCINT22);
    break;
  case 7:
    serial->rx_bit_mask = _BV(PD7);
    serial->rx_port_register = &PIND;
    DDRD &= (uint8_t)~_BV(PD7);
    PORTD |= _BV(PD7);
    serial->pcint_mask_register = &PCMSK2;
    serial->pcint_mask_value = _BV(PCINT23);
    break;
  case 8:
    serial->rx_bit_mask = _BV(PB0);
    serial->rx_port_register = &PINB;
    DDRB &= (uint8_t)~_BV(PB0);
    PORTB |= _BV(PB0);
    serial->pcint_mask_register = &PCMSK0;
    serial->pcint_mask_value = _BV(PCINT0);
    break;
  case 9:
    serial->rx_bit_mask = _BV(PB1);
    serial->rx_port_register = &PINB;
    DDRB &= (uint8_t)~_BV(PB1);
    PORTB |= _BV(PB1);
    serial->pcint_mask_register = &PCMSK0;
    serial->pcint_mask_value = _BV(PCINT1);
    break;
  case 10:
    serial->rx_bit_mask = _BV(PB2);
    serial->rx_port_register = &PINB;
    DDRB &= (uint8_t)~_BV(PB2);
    PORTB |= _BV(PB2);
    serial->pcint_mask_register = &PCMSK0;
    serial->pcint_mask_value = _BV(PCINT2);
    break;
  case 11:
    serial->rx_bit_mask = _BV(PB3);
    serial->rx_port_register = &PINB;
    DDRB &= (uint8_t)~_BV(PB3);
    PORTB |= _BV(PB3);
    serial->pcint_mask_register = &PCMSK0;
    serial->pcint_mask_value = _BV(PCINT3);
    break;
  case 12:
    serial->rx_bit_mask = _BV(PB4);
    serial->rx_port_register = &PINB;
    DDRB &= (uint8_t)~_BV(PB4);
    PORTB |= _BV(PB4);
    serial->pcint_mask_register = &PCMSK0;
    serial->pcint_mask_value = _BV(PCINT4);
    break;
  case 13:
    serial->rx_bit_mask = _BV(PB5);
    serial->rx_port_register = &PINB;
    DDRB &= (uint8_t)~_BV(PB5);
    PORTB |= _BV(PB5);
    serial->pcint_mask_register = &PCMSK0;
    serial->pcint_mask_value = _BV(PCINT5);
    break;
  case 14:
    serial->rx_bit_mask = _BV(PC0);
    serial->rx_port_register = &PINC;
    DDRC &= (uint8_t)~_BV(PC0);
    PORTC |= _BV(PC0);
    serial->pcint_mask_register = &PCMSK1;
    serial->pcint_mask_value = _BV(PCINT8);
    break;
  case 15:
    serial->rx_bit_mask = _BV(PC1);
    serial->rx_port_register = &PINC;
    DDRC &= (uint8_t)~_BV(PC1);
    PORTC |= _BV(PC1);
    serial->pcint_mask_register = &PCMSK1;
    serial->pcint_mask_value = _BV(PCINT9);
    break;
  case 16:
    serial->rx_bit_mask = _BV(PC2);
    serial->rx_port_register = &PINC;
    DDRC &= (uint8_t)~_BV(PC2);
    PORTC |= _BV(PC2);
    serial->pcint_mask_register = &PCMSK1;
    serial->pcint_mask_value = _BV(PCINT10);
    break;
  case 17:
    serial->rx_bit_mask = _BV(PC3);
    serial->rx_port_register = &PINC;
    DDRC &= (uint8_t)~_BV(PC3);
    PORTC |= _BV(PC3);
    serial->pcint_mask_register = &PCMSK1;
    serial->pcint_mask_value = _BV(PCINT11);
    break;
  case 18:
    serial->rx_bit_mask = _BV(PC4);
    serial->rx_port_register = &PINC;
    DDRC &= (uint8_t)~_BV(PC4);
    PORTC |= _BV(PC4);
    serial->pcint_mask_register = &PCMSK1;
    serial->pcint_mask_value = _BV(PCINT12);
    break;
  case 19:
    serial->rx_bit_mask = _BV(PC5);
    serial->rx_port_register = &PINC;
    DDRC &= (uint8_t)~_BV(PC5);
    PORTC |= _BV(PC5);
    serial->pcint_mask_register = &PCMSK1;
    serial->pcint_mask_value = _BV(PCINT13);
    break;
  default:
    return false;
  }

  return true;
}

static void enable_pcint_group(au_software_serial_t *serial)
{
  if (serial->pcint_mask_register == &PCMSK0) {
    PCICR |= _BV(PCIE0);
  }
  else if (serial->pcint_mask_register == &PCMSK1) {
    PCICR |= _BV(PCIE1);
  }
  else if (serial->pcint_mask_register == &PCMSK2) {
    PCICR |= _BV(PCIE2);
  }
}

void au_software_serial_init(au_software_serial_t *serial, uint8_t rx_pin, uint8_t tx_pin, bool inverse_logic) {
  serial->rx_pin = rx_pin;
  serial->tx_pin = tx_pin;

  serial->inverse_logic = inverse_logic ? 1 : 0;

  serial->rx_delay_centering = 0;
  serial->rx_delay_intrabit = 0;
  serial->rx_delay_stopbit = 0;
  serial->tx_delay = 0;

  serial->buffer_overflow = 0;
  serial->listening = 0;

  serial->rx_port_register = NULL;
  serial->tx_port_register = NULL;
  serial->pcint_mask_register = NULL;

  serial->rx_bit_mask = 0;
  serial->tx_bit_mask = 0;
  serial->pcint_mask_value = 0;

  set_tx(serial, tx_pin);
  set_rx(serial, rx_pin);
}

void au_software_serial_begin(au_software_serial_t *serial, uint32_t baud) {
  if (baud == 0) {
    return;
  }

  serial->rx_delay_centering = 0;
  serial->rx_delay_intrabit = 0;
  serial->rx_delay_stopbit = 0;
  serial->tx_delay = 0;

  uint16_t bit_delay = (uint16_t)((F_CPU / baud) / 4UL);

  serial->tx_delay = subtract_cap(bit_delay, 15 / 4);

  if (serial->pcint_mask_register == NULL) {
    return;
  }

  /*
    * Timing corresponding to the current avr-gcc implementation
    * used by Arduino SoftwareSerial.
    *
    * These values are expressed in 4-cycle units.
    */

  serial->rx_delay_centering = subtract_cap(bit_delay / 2, (4 + 4 + 75 + 17 - 23) / 4);
  serial->rx_delay_intrabit = subtract_cap(bit_delay, 23 / 4);
  serial->rx_delay_stopbit = subtract_cap(bit_delay * 3 / 4, (37 + 11) / 4);

  /*
    * Enable the PCINT group.
    */
  enable_pcint_group(serial);

  /*
    * Enable this particular pin.
    */
  set_rx_int_mask(serial, true);

  /*
    * Establish the end of the current TX bit.
    */
  au_software_serial_tuned_delay(serial->tx_delay);

  au_software_serial_listen(serial);
}

bool au_software_serial_listen(au_software_serial_t *serial) {
  if (serial->rx_delay_stopbit == 0) {
    return false;
  }

  if (active_serial != serial) {
    if (active_serial != NULL) {
      au_software_serial_stop_listening(active_serial);
    }

    serial->buffer_overflow = 0;

    receive_buffer_head = 0;
    receive_buffer_tail = 0;

    active_serial = serial;
    serial->listening = 1;

    set_rx_int_mask(serial, true);

    return true;
  }

  return false;
}

void au_software_serial_stop_listening(au_software_serial_t *serial) {
  if (active_serial == serial) {
    set_rx_int_mask(serial, false);

    active_serial = NULL;
    serial->listening = 0;
  }
}

static void au_software_serial_recv(au_software_serial_t *serial) {
  uint8_t d = 0;

  if (rx_pin_read(serial)) {
    return;
  }

  set_rx_int_mask(serial, false);

  au_software_serial_tuned_delay(
      serial->rx_delay_centering
  );

  if (rx_pin_read(serial)) {
    set_rx_int_mask(serial, true);
    return;
  }

  // Bit 0
  au_software_serial_tuned_delay(serial->rx_delay_intrabit);
  d >>= 1;
  if (rx_pin_read(serial)) {
    d |= 0x80;
  }

  // Bit 1
  au_software_serial_tuned_delay(serial->rx_delay_intrabit);
  d >>= 1;
  if (rx_pin_read(serial)) {
    d |= 0x80;
  }

  // Bit 2
  au_software_serial_tuned_delay(
    serial->rx_delay_intrabit
  );
  d >>= 1;
  if (rx_pin_read(serial)) {
    d |= 0x80;
  }

  // Bit 3
  au_software_serial_tuned_delay(serial->rx_delay_intrabit);
  d >>= 1;
  if (rx_pin_read(serial)) {
    d |= 0x80;
  }

  // Bit 4
  au_software_serial_tuned_delay(serial->rx_delay_intrabit);
  d >>= 1;
  if (rx_pin_read(serial)) {
    d |= 0x80;
  }

  // Bit 5
  au_software_serial_tuned_delay(serial->rx_delay_intrabit);
  d >>= 1;
  if (rx_pin_read(serial)) {
    d |= 0x80;
  }

  // Bit 6
  au_software_serial_tuned_delay(serial->rx_delay_intrabit);
  d >>= 1;
  if (rx_pin_read(serial)) {
    d |= 0x80;
  }

  // Bit 7
  au_software_serial_tuned_delay(serial->rx_delay_intrabit);
  d >>= 1;
  if (rx_pin_read(serial)) {
    d |= 0x80;
  }

  // Stop bit
  au_software_serial_tuned_delay(serial->rx_delay_stopbit);

  // Store byte
  uint8_t next_head = (uint8_t)(receive_buffer_head + 1);

  if (next_head == AU_SOFTWARE_SERIAL_RX_BUFFER_SIZE) {
    next_head = 0;
  }

  if (next_head == receive_buffer_tail) {
    serial->buffer_overflow = 1;
  }
  else {
    receive_buffer[receive_buffer_head] = d;
    receive_buffer_head = next_head;
  }

  set_rx_int_mask(serial, true);
}

int au_software_serial_available(au_software_serial_t *serial) {
  (void)serial;

  uint8_t head = receive_buffer_head;
  uint8_t tail = receive_buffer_tail;

  if (head >= tail) {
    return head - tail;
  }

  return AU_SOFTWARE_SERIAL_RX_BUFFER_SIZE - tail + head;
}

int au_software_serial_read(au_software_serial_t *serial) {
  (void)serial;

  if (receive_buffer_head == receive_buffer_tail) {
    return -1;
  }

  uint8_t c = receive_buffer[receive_buffer_tail];

  receive_buffer_tail++;

  if (receive_buffer_tail == AU_SOFTWARE_SERIAL_RX_BUFFER_SIZE) {
    receive_buffer_tail = 0;
  }

  return c;
}

int au_software_serial_peek(au_software_serial_t *serial) {
  (void)serial;

  if (receive_buffer_head == receive_buffer_tail) {
    return -1;
  }

  return receive_buffer[receive_buffer_tail];
}

bool au_software_serial_overflow(au_software_serial_t *serial) {
  bool overflow = serial->buffer_overflow != 0;

  serial->buffer_overflow = 0;

  return overflow;
}

size_t au_software_serial_write(au_software_serial_t *serial, uint8_t value) {
  if (serial->tx_delay == 0) {
    return 0;
  }

  volatile uint8_t *reg = serial->tx_port_register;
  uint8_t reg_mask = serial->tx_bit_mask;
  uint8_t inv_mask = (uint8_t)~serial->tx_bit_mask;

  uint8_t old_sreg = SREG;

  bool inverse_logic = serial->inverse_logic != 0;
  uint16_t delay = serial->tx_delay;

  // SoftwareSerial transmits the byte LSB first.
  // For inverse logic the whole byte is inverted before
  // transmission, exactly as in the Arduino implementation.
  if (inverse_logic) {
    value = (uint8_t)~value;
  }

  // Disable interrupts for the entire transmission.
  // This is essential: any interrupt would introduce timing
  // jitter into the software-generated serial waveform.
  cli();

  // Start bit.
  // Normal logic:
  //   idle = HIGH
  //   start = LOW
  // Inverse logic:
  //   idle = LOW
  //   start = HIGH
  if (inverse_logic) {
    *reg |= reg_mask;
  }
  else {
    *reg &= inv_mask;
  }

  au_software_serial_tuned_delay(delay);

  // Data bits, LSB first.
  for (uint8_t i = 8; i > 0; --i) {
    if (value & 1) {
        *reg |= reg_mask;
    }
    else {
      *reg &= inv_mask;
    }

    au_software_serial_tuned_delay(delay);

    value >>= 1;
  }

  // Stop bit / return to idle state.
  if (inverse_logic) {
    *reg &= inv_mask;
  }
  else {
    *reg |= reg_mask;
  }

  // Restore interrupt state exactly as it was before write().
  SREG = old_sreg;

  // Allow the stop bit to remain active for one complete bit time.
  au_software_serial_tuned_delay(delay);

  return 1;
}

void au_software_serial_flush(au_software_serial_t *serial) {
  (void)serial;
}

void au_software_serial_end(au_software_serial_t *serial) {
  au_software_serial_stop_listening(serial);
}

static void handle_interrupt(void) {
  if (active_serial != NULL) {
    au_software_serial_recv(active_serial);
  }
}

ISR(PCINT0_vect) {
  handle_interrupt();
}

ISR(PCINT1_vect) {
  handle_interrupt();
}


ISR(PCINT2_vect) {
  handle_interrupt();
}

size_t au_software_serial_print(void *context, uint8_t value) {
  return au_software_serial_write((au_software_serial_t *) context, value);
}

au_printer_t au_software_serial_build_printer(au_software_serial_t *serial) {
  au_printer_t printer = {
    .context = serial,
    .print = au_software_serial_print
  };

  return printer;
}