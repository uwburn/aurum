#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay_basic.h>
#include <stddef.h>

#include "aurum_private.h"

typedef struct {
  /* RX */
  uint8_t rx_bit_mask;
  volatile uint8_t *rx_port_register;

  /* TX */
  uint8_t tx_bit_mask;
  volatile uint8_t *tx_port_register;

  /* Pin Change Interrupt */
  volatile uint8_t *pcint_mask_register;
  uint8_t pcint_mask_value;

  /* Timing - expressed as 4-cycle delays */
  uint16_t rx_delay_centering;
  uint16_t rx_delay_intrabit;
  uint16_t rx_delay_stopbit;
  uint16_t tx_delay;

  /* State */
  uint8_t buffer_overflow;
  uint8_t inverse_logic;
  uint8_t listening;
} au_software_serial_impl_t;

_Static_assert(
  sizeof(au_software_serial_impl_t) <= AU_SOFTWARE_SERIAL_SIZE,
  "au_software_serial_t storage too small"
);

static inline au_software_serial_impl_t * au_software_serial_impl(au_software_serial_t *serial) {
    return (au_software_serial_impl_t *)serial->storage;
}

#define AU_SOFTWARE_SERIAL_RX_BUFFER_SIZE 64

static uint8_t receive_buffer[AU_SOFTWARE_SERIAL_RX_BUFFER_SIZE];
static volatile uint8_t receive_buffer_head = 0;
static volatile uint8_t receive_buffer_tail = 0;

static au_software_serial_t *active_serial = NULL;

static void enable_pcint(au_software_serial_t *serial);
static void disable_pcint(au_software_serial_t *serial);

static void recv(au_software_serial_t *serial);
static void tuned_delay(uint16_t delay);


static inline void au_software_serial_tuned_delay(uint16_t delay) {
  _delay_loop_2(delay);
}

static uint16_t subtract_cap(uint16_t num, uint16_t sub) {
  if (num > sub) {
    return num - sub;
  }

  return 1;
}

static inline uint8_t rx_pin_read(const au_software_serial_t *serial) {
  au_software_serial_impl_t *impl = serial_impl(serial);

  return (*impl->rx_port_register & impl->rx_bit_mask);
}

static inline void set_rx_int_mask(au_software_serial_t *serial, bool enable) {
  au_software_serial_impl_t *impl = serial_impl(serial);

  if (enable) {
    *impl->pcint_mask_register |= impl->pcint_mask_value;
  }
  else {
    *impl->pcint_mask_register &= (uint8_t)~impl->pcint_mask_value;
  }
}

static void set_tx(au_software_serial_t *serial, uint8_t pin) {
  au_software_serial_impl_t *impl = serial_impl(serial);

  switch (pin) {
  case 0:
    impl->tx_bit_mask = _BV(PD0);
    impl->tx_port_register = &PORTD;
    DDRD |= _BV(PD0);
    PORTD |= _BV(PD0);
    break;
  case 1:
    impl->tx_bit_mask = _BV(PD1);
    impl->tx_port_register = &PORTD;
    DDRD |= _BV(PD1);
    PORTD |= _BV(PD1);
    break;
  case 2:
    impl->tx_bit_mask = _BV(PD2);
    impl->tx_port_register = &PORTD;
    DDRD |= _BV(PD2);
    PORTD |= _BV(PD2);
    break;
  case 3:
    impl->tx_bit_mask = _BV(PD3);
    impl->tx_port_register = &PORTD;
    DDRD |= _BV(PD3);
    PORTD |= _BV(PD3);
    break;
  case 4:
    impl->tx_bit_mask = _BV(PD4);
    impl->tx_port_register = &PORTD;
    DDRD |= _BV(PD4);
    PORTD |= _BV(PD4);
    break;
  case 5:
    impl->tx_bit_mask = _BV(PD5);
    impl->tx_port_register = &PORTD;
    DDRD |= _BV(PD5);
    PORTD |= _BV(PD5);
    break;
  case 6:
    impl->tx_bit_mask = _BV(PD6);
    impl->tx_port_register = &PORTD;
    DDRD |= _BV(PD6);
    PORTD |= _BV(PD6);
    break;
  case 7:
    impl->tx_bit_mask = _BV(PD7);
    impl->tx_port_register = &PORTD;
    DDRD |= _BV(PD7);
    PORTD |= _BV(PD7);
    break;
  case 8:
    impl->tx_bit_mask = _BV(PB0);
    impl->tx_port_register = &PORTB;
    DDRB |= _BV(PB0);
    PORTB |= _BV(PB0);
    break;
  case 9:
    impl->tx_bit_mask = _BV(PB1);
    impl->tx_port_register = &PORTB;
    DDRB |= _BV(PB1);
    PORTB |= _BV(PB1);
    break;
  case 10:
    impl->tx_bit_mask = _BV(PB2);
    impl->tx_port_register = &PORTB;
    DDRB |= _BV(PB2);
    PORTB |= _BV(PB2);
    break;
  case 11:
    impl->tx_bit_mask = _BV(PB3);
    impl->tx_port_register = &PORTB;
    DDRB |= _BV(PB3);
    PORTB |= _BV(PB3);
    break;
  case 12:
    impl->tx_bit_mask = _BV(PB4);
    impl->tx_port_register = &PORTB;
    DDRB |= _BV(PB4);
    PORTB |= _BV(PB4);
    break;
  case 13:
    impl->tx_bit_mask = _BV(PB5);
    impl->tx_port_register = &PORTB;
    DDRB |= _BV(PB5);
    PORTB |= _BV(PB5);
    break;
  case 14:
    impl->tx_bit_mask = _BV(PC0);
    impl->tx_port_register = &PORTC;
    DDRC |= _BV(PC0);
    PORTC |= _BV(PC0);
    break;
  case 15:
    impl->tx_bit_mask = _BV(PC1);
    impl->tx_port_register = &PORTC;
    DDRC |= _BV(PC1);
    PORTC |= _BV(PC1);
    break;
  case 16:
    impl->tx_bit_mask = _BV(PC2);
    impl->tx_port_register = &PORTC;
    DDRC |= _BV(PC2);
    PORTC |= _BV(PC2);
    break;
  case 17:
    impl->tx_bit_mask = _BV(PC3);
    impl->tx_port_register = &PORTC;
    DDRC |= _BV(PC3);
    PORTC |= _BV(PC3);
    break;
  case 18:
    impl->tx_bit_mask = _BV(PC4);
    impl->tx_port_register = &PORTC;
    DDRC |= _BV(PC4);
    PORTC |= _BV(PC4);
    break;
  case 19:
    impl->tx_bit_mask = _BV(PC5);
    impl->tx_port_register = &PORTC;
    DDRC |= _BV(PC5);
    PORTC |= _BV(PC5);
    break;
  default:
    impl->tx_port_register = NULL;
    impl->tx_bit_mask = 0;
    break;
  }
}

static bool set_rx(au_software_serial_t *serial, uint8_t pin) {
  au_software_serial_impl_t *impl = serial_impl(serial);

  switch (pin) {
  case 0:
    impl->rx_bit_mask = _BV(PD0);
    impl->rx_port_register = &PIND;
    DDRD &= (uint8_t)~_BV(PD0);
    PORTD |= _BV(PD0);
    impl->pcint_mask_register = &PCMSK2;
    impl->pcint_mask_value = _BV(PCINT16);
    break;
  case 1:
    impl->rx_bit_mask = _BV(PD1);
    impl->rx_port_register = &PIND;
    DDRD &= (uint8_t)~_BV(PD1);
    PORTD |= _BV(PD1);
    impl->pcint_mask_register = &PCMSK2;
    impl->pcint_mask_value = _BV(PCINT17);
    break;
  case 2:
    impl->rx_bit_mask = _BV(PD2);
    impl->rx_port_register = &PIND;
    DDRD &= (uint8_t)~_BV(PD2);
    PORTD |= _BV(PD2);
    impl->pcint_mask_register = &PCMSK2;
    impl->pcint_mask_value = _BV(PCINT18);
    break;
  case 3:
    impl->rx_bit_mask = _BV(PD3);
    impl->rx_port_register = &PIND;
    DDRD &= (uint8_t)~_BV(PD3);
    PORTD |= _BV(PD3);
    impl->pcint_mask_register = &PCMSK2;
    impl->pcint_mask_value = _BV(PCINT19);
    break;
  case 4:
    impl->rx_bit_mask = _BV(PD4);
    impl->rx_port_register = &PIND;
    DDRD &= (uint8_t)~_BV(PD4);
    PORTD |= _BV(PD4);
    impl->pcint_mask_register = &PCMSK2;
    impl->pcint_mask_value = _BV(PCINT20);
    break;
  case 5:
    impl->rx_bit_mask = _BV(PD5);
    impl->rx_port_register = &PIND;
    DDRD &= (uint8_t)~_BV(PD5);
    PORTD |= _BV(PD5);
    impl->pcint_mask_register = &PCMSK2;
    impl->pcint_mask_value = _BV(PCINT21);
    break;
  case 6:
    impl->rx_bit_mask = _BV(PD6);
    impl->rx_port_register = &PIND;
    DDRD &= (uint8_t)~_BV(PD6);
    PORTD |= _BV(PD6);
    impl->pcint_mask_register = &PCMSK2;
    impl->pcint_mask_value = _BV(PCINT22);
    break;
  case 7:
    impl->rx_bit_mask = _BV(PD7);
    impl->rx_port_register = &PIND;
    DDRD &= (uint8_t)~_BV(PD7);
    PORTD |= _BV(PD7);
    impl->pcint_mask_register = &PCMSK2;
    impl->pcint_mask_value = _BV(PCINT23);
    break;
  case 8:
    impl->rx_bit_mask = _BV(PB0);
    impl->rx_port_register = &PINB;
    DDRB &= (uint8_t)~_BV(PB0);
    PORTB |= _BV(PB0);
    impl->pcint_mask_register = &PCMSK0;
    impl->pcint_mask_value = _BV(PCINT0);
    break;
  case 9:
    impl->rx_bit_mask = _BV(PB1);
    impl->rx_port_register = &PINB;
    DDRB &= (uint8_t)~_BV(PB1);
    PORTB |= _BV(PB1);
    impl->pcint_mask_register = &PCMSK0;
    impl->pcint_mask_value = _BV(PCINT1);
    break;
  case 10:
    impl->rx_bit_mask = _BV(PB2);
    impl->rx_port_register = &PINB;
    DDRB &= (uint8_t)~_BV(PB2);
    PORTB |= _BV(PB2);
    impl->pcint_mask_register = &PCMSK0;
    impl->pcint_mask_value = _BV(PCINT2);
    break;
  case 11:
    impl->rx_bit_mask = _BV(PB3);
    impl->rx_port_register = &PINB;
    DDRB &= (uint8_t)~_BV(PB3);
    PORTB |= _BV(PB3);
    impl->pcint_mask_register = &PCMSK0;
    impl->pcint_mask_value = _BV(PCINT3);
    break;
  case 12:
    impl->rx_bit_mask = _BV(PB4);
    impl->rx_port_register = &PINB;
    DDRB &= (uint8_t)~_BV(PB4);
    PORTB |= _BV(PB4);
    impl->pcint_mask_register = &PCMSK0;
    impl->pcint_mask_value = _BV(PCINT4);
    break;
  case 13:
    impl->rx_bit_mask = _BV(PB5);
    impl->rx_port_register = &PINB;
    DDRB &= (uint8_t)~_BV(PB5);
    PORTB |= _BV(PB5);
    impl->pcint_mask_register = &PCMSK0;
    impl->pcint_mask_value = _BV(PCINT5);
    break;
  case 14:
    impl->rx_bit_mask = _BV(PC0);
    impl->rx_port_register = &PINC;
    DDRC &= (uint8_t)~_BV(PC0);
    PORTC |= _BV(PC0);
    impl->pcint_mask_register = &PCMSK1;
    impl->pcint_mask_value = _BV(PCINT8);
    break;
  case 15:
    impl->rx_bit_mask = _BV(PC1);
    impl->rx_port_register = &PINC;
    DDRC &= (uint8_t)~_BV(PC1);
    PORTC |= _BV(PC1);
    impl->pcint_mask_register = &PCMSK1;
    impl->pcint_mask_value = _BV(PCINT9);
    break;
  case 16:
    impl->rx_bit_mask = _BV(PC2);
    impl->rx_port_register = &PINC;
    DDRC &= (uint8_t)~_BV(PC2);
    PORTC |= _BV(PC2);
    impl->pcint_mask_register = &PCMSK1;
    impl->pcint_mask_value = _BV(PCINT10);
    break;
  case 17:
    impl->rx_bit_mask = _BV(PC3);
    impl->rx_port_register = &PINC;
    DDRC &= (uint8_t)~_BV(PC3);
    PORTC |= _BV(PC3);
    impl->pcint_mask_register = &PCMSK1;
    impl->pcint_mask_value = _BV(PCINT11);
    break;
  case 18:
    impl->rx_bit_mask = _BV(PC4);
    impl->rx_port_register = &PINC;
    DDRC &= (uint8_t)~_BV(PC4);
    PORTC |= _BV(PC4);
    impl->pcint_mask_register = &PCMSK1;
    impl->pcint_mask_value = _BV(PCINT12);
    break;
  case 19:
    impl->rx_bit_mask = _BV(PC5);
    impl->rx_port_register = &PINC;
    DDRC &= (uint8_t)~_BV(PC5);
    PORTC |= _BV(PC5);
    impl->pcint_mask_register = &PCMSK1;
    impl->pcint_mask_value = _BV(PCINT13);
    break;
  default:
    return false;
  }

  return true;
}

static void enable_pcint_group(const au_software_serial_t *serial) {
  au_software_serial_impl_t *impl = serial_impl(serial);

  if (impl->pcint_mask_register == &PCMSK0) {
    PCICR |= _BV(PCIE0);
  }
  else if (impl->pcint_mask_register == &PCMSK1) {
    PCICR |= _BV(PCIE1);
  }
  else if (impl->pcint_mask_register == &PCMSK2) {
    PCICR |= _BV(PCIE2);
  }
}

void au_software_serial_init(au_software_serial_t *serial, uint8_t rx_pin, uint8_t tx_pin, bool inverse_logic) {
  au_software_serial_impl_t *impl = serial_impl(serial);

  impl->inverse_logic = inverse_logic ? 1 : 0;

  impl->rx_delay_centering = 0;
  impl->rx_delay_intrabit = 0;
  impl->rx_delay_stopbit = 0;
  impl->tx_delay = 0;

  impl->buffer_overflow = 0;
  impl->listening = 0;

  impl->rx_port_register = NULL;
  impl->tx_port_register = NULL;
  impl->pcint_mask_register = NULL;

  impl->rx_bit_mask = 0;
  impl->tx_bit_mask = 0;
  impl->pcint_mask_value = 0;

  set_tx(serial, tx_pin);
  set_rx(serial, rx_pin);
}

void au_software_serial_begin(au_software_serial_t *serial, uint32_t baud) {
  au_software_serial_impl_t *impl = serial_impl(serial);

  if (baud == 0) {
    return;
  }

  impl->rx_delay_centering = 0;
  impl->rx_delay_intrabit = 0;
  impl->rx_delay_stopbit = 0;
  impl->tx_delay = 0;

  uint16_t bit_delay = (uint16_t)((F_CPU / baud) / 4UL);

  impl->tx_delay = subtract_cap(bit_delay, 15 / 4);

  if (impl->pcint_mask_register == NULL) {
    return;
  }

  /*
    * Timing corresponding to the current avr-gcc implementation
    * used by Arduino SoftwareSerial.
    *
    * These values are expressed in 4-cycle units.
    */

  impl->rx_delay_centering = subtract_cap(bit_delay / 2, (4 + 4 + 75 + 17 - 23) / 4);
  impl->rx_delay_intrabit = subtract_cap(bit_delay, 23 / 4);
  impl->rx_delay_stopbit = subtract_cap(bit_delay * 3 / 4, (37 + 11) / 4);

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
  au_software_serial_tuned_delay(impl->tx_delay);

  return au_software_serial_listen(serial);
}

bool au_software_serial_listen(au_software_serial_t *serial) {
  au_software_serial_impl_t *impl = serial_impl(serial);

  if (impl->rx_delay_stopbit == 0) {
    return false;
  }

  if (active_serial != serial) {
    if (active_serial != NULL) {
      au_software_serial_stop_listening(active_serial);
    }

    impl->buffer_overflow = 0;

    receive_buffer_head = 0;
    receive_buffer_tail = 0;

    active_serial = serial;
    impl->listening = 1;

    set_rx_int_mask(serial, true);

    return true;
  }

  return false;
}

bool au_software_serial_stop_listening(au_software_serial_t *serial) {
  au_software_serial_impl_t *impl = serial_impl(serial);

  if (active_serial == serial) {
    set_rx_int_mask(serial, false);

    active_serial = NULL;
    impl->listening = 0;

    return true;
  }

  return false;
}

static void au_software_serial_recv(au_software_serial_t *serial) {
  au_software_serial_impl_t *impl = serial_impl(serial);

  uint8_t d = 0;

  if (rx_pin_read(serial)) {
    return;
  }

  set_rx_int_mask(serial, false);

  au_software_serial_tuned_delay(
      impl->rx_delay_centering
  );

  if (rx_pin_read(serial)) {
    set_rx_int_mask(serial, true);
    return;
  }

  // Bit 0
  au_software_serial_tuned_delay(impl->rx_delay_intrabit);
  d >>= 1;
  if (rx_pin_read(serial)) {
    d |= 0x80;
  }

  // Bit 1
  au_software_serial_tuned_delay(impl->rx_delay_intrabit);
  d >>= 1;
  if (rx_pin_read(serial)) {
    d |= 0x80;
  }

  // Bit 2
  au_software_serial_tuned_delay(
    impl->rx_delay_intrabit
  );
  d >>= 1;
  if (rx_pin_read(serial)) {
    d |= 0x80;
  }

  // Bit 3
  au_software_serial_tuned_delay(impl->rx_delay_intrabit);
  d >>= 1;
  if (rx_pin_read(serial)) {
    d |= 0x80;
  }

  // Bit 4
  au_software_serial_tuned_delay(impl->rx_delay_intrabit);
  d >>= 1;
  if (rx_pin_read(serial)) {
    d |= 0x80;
  }

  // Bit 5
  au_software_serial_tuned_delay(impl->rx_delay_intrabit);
  d >>= 1;
  if (rx_pin_read(serial)) {
    d |= 0x80;
  }

  // Bit 6
  au_software_serial_tuned_delay(impl->rx_delay_intrabit);
  d >>= 1;
  if (rx_pin_read(serial)) {
    d |= 0x80;
  }

  // Bit 7
  au_software_serial_tuned_delay(impl->rx_delay_intrabit);
  d >>= 1;
  if (rx_pin_read(serial)) {
    d |= 0x80;
  }

  // Stop bit
  au_software_serial_tuned_delay(impl->rx_delay_stopbit);

  // Store byte
  uint8_t next_head = (uint8_t)(receive_buffer_head + 1);

  if (next_head == AU_SOFTWARE_SERIAL_RX_BUFFER_SIZE) {
    next_head = 0;
  }

  if (next_head == receive_buffer_tail) {
    impl->buffer_overflow = 1;
  }
  else {
    receive_buffer[receive_buffer_head] = d;
    receive_buffer_head = next_head;
  }

  set_rx_int_mask(serial, true);
}

int au_software_serial_available(const au_software_serial_t *serial) {
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
  au_software_serial_impl_t *impl = serial_impl(serial);

  bool overflow = impl->buffer_overflow != 0;

  impl->buffer_overflow = 0;

  return overflow;
}

size_t au_software_serial_write(au_software_serial_t *serial, uint8_t value) {
  au_software_serial_impl_t *impl = serial_impl(serial);
  
  if (impl->tx_delay == 0) {
    return 0;
  }

  volatile uint8_t *reg = impl->tx_port_register;
  uint8_t reg_mask = impl->tx_bit_mask;
  uint8_t inv_mask = (uint8_t)~impl->tx_bit_mask;

  uint8_t old_sreg = SREG;

  bool inverse_logic = impl->inverse_logic != 0;
  uint16_t delay = impl->tx_delay;

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

void au_software_serial_print_str(au_software_serial_t *serial, const char *s) {
  while (*s) {
    au_software_serial_write(serial, (uint8_t)*s++);
  }
}

void au_software_serial_println_str(au_software_serial_t *serial, const char *s) {
  au_software_serial_print_str(serial, s);
  au_software_serial_write(serial, '\r');
  au_software_serial_write(serial, '\n');
}

void au_software_serial_print_uint(au_software_serial_t *serial, uint32_t value) {
  char buffer[10];
  uint8_t i = 0;

  if (value == 0) {
    au_software_serial_write(serial, '0');
    return;
  }

  while (value > 0) {
    buffer[i++] = '0' + (value % 10);
    value /= 10;
  }

  while (i > 0) {
    au_software_serial_write(serial, buffer[--i]);
  }
}

void au_software_serial_println_uint(au_software_serial_t *serial, uint32_t value) {
  au_software_serial_print_uint(serial, value);
  au_software_serial_write(serial, '\r');
  au_software_serial_write(serial, '\n');
}

void au_software_serial_print_int(au_software_serial_t *serial, int32_t value) {
  if (value < 0) {
    au_software_serial_write(serial, '-');

    au_software_serial_print_uint(serial, (uint32_t)(-(value + 1)) + 1);
  }
  else {
    au_software_serial_print_uint(serial, (uint32_t)value);
  }
}

void au_software_serial_println_int(au_software_serial_t *serial, int32_t value) {
  au_software_serial_print_int(serial, value);
  au_software_serial_write(serial, '\r');
  au_software_serial_write(serial, '\n');
}