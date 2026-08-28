#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "aurum.h"

// System
static void au_time_init(void) {
  /*
  * Timer0:
  *
  * CPU = 16 MHz
  * Prescaler = 64
  * Timer clock = 250 kHz
  * Tick = 4 us
  */

    TCCR0A =
      (1 << WGM01) |
      (1 << WGM00);

    TCCR0B =
      (1 << CS01) |
      (1 << CS00);

    TCNT0 = 0;

    TIMSK0 = (1 << TOIE0);

    sei();
}

static void au_analog_init(void) {
  // AVcc reference
  // ADC0 initial input
  ADMUX = (1 << REFS0);

  // Enable ADC
  // 16 MHz / 128 = 125 kHz
  ADCSRA =
    (1 << ADEN)  |
    (1 << ADPS2) |
    (1 << ADPS1) |
    (1 << ADPS0);
}

static void au_pwm1_init(void) {
  /*
  * Timer1:
  *
  * Fast PWM 8 bit
  * TOP = 255
  * Prescaler = 64
  *
  * PWM ≈ 976.6 Hz
  */

  TCCR1A =
    (1 << WGM10);

  TCCR1B =
    (1 << WGM12) |
    (1 << CS11)  |
    (1 << CS10);

  TCNT1 = 0;
}

static void au_pwm2_init(void) {
  /*
  * Timer2:
  *
  * Fast PWM
  * TOP = 255
  * Prescaler = 64
  *
  * PWM ≈ 976.6 Hz
  */

  TCCR2A =
    (1 << WGM21) |
    (1 << WGM20);

  TCCR2B =
    (1 << CS22);

  TCNT2 = 0;
}

void au_system_init(void) {
  au_time_init();
  au_analog_init();
  au_pwm1_init();
  au_pwm2_init();
}

// Time

#define MILLIS_INC  1
#define FRACT_INC   3
#define FRACT_MAX   125

static volatile uint32_t timer0_overflow_count = 0;
static volatile uint32_t timer0_millis = 0;
static volatile uint8_t timer0_millis_fract = 0;

ISR(TIMER0_OVF_vect) {
  timer0_overflow_count++;

  uint32_t m = timer0_millis;
  uint8_t f = timer0_millis_fract;

  m += MILLIS_INC;
  f += FRACT_INC;

  if (f >= FRACT_MAX) {
    f -= FRACT_MAX;
    m++;
  }

  timer0_millis = m;
  timer0_millis_fract = f;
}

uint32_t au_micros(void) {
  uint32_t overflow_count;
  uint8_t timer_count;

  // Read counter
  uint8_t sreg = SREG;
  cli();

  overflow_count = timer0_overflow_count;
  timer_count = TCNT0;

  // Fix count i timer overflow while waiting for interrupt
  if ((TIFR0 & (1 << TOV0)) && timer_count < 255) {
    overflow_count++;
  }

  SREG = sreg;

  // 4us each tick
  return ((overflow_count << 8) + timer_count) * 4UL;
}

uint32_t au_millis(void) {
    uint32_t m;

    uint8_t sreg = SREG;
    cli();

    m = timer0_millis;

    SREG = sreg;

    return m;
}

void au_delay(uint32_t ms) {
  while (ms--) {
    _delay_ms(1);
  }
}

void au_delay_microseconds(uint32_t us) {
  while (us--) {
    _delay_us(1);
  }
}

// Digital I/O

void au_pin_mode(uint8_t pin, uint8_t mode) {
  if (pin <= 7) {
    if (mode == OUTPUT) {
      DDRD |= (1 << pin);
    }
    else {
      DDRD &= ~(1 << pin);
    }

    if (mode == INPUT_PULLUP) {
      PORTD |= (1 << pin);
    }
    else {
      PORTD &= ~(1 << pin);
    }
  }
  else if (pin <= 13) {
    uint8_t bit = pin - 8;

    if (mode == OUTPUT) {
      DDRB |= (1 << bit);
    }
    else {
      DDRB &= ~(1 << bit);
    }

    if (mode == INPUT_PULLUP) {
      PORTB |= (1 << bit);
    }
    else {
      PORTB &= ~(1 << bit);
    }
  }
}

void au_digital_write(uint8_t pin, uint8_t value) {
  if (pin <= 7) {
    if (value == HIGH) {
      PORTD |= (1 << pin);
    }
    else {
      PORTD &= ~(1 << pin);
    }
  }
  else if (pin <= 13) {
    uint8_t bit = pin - 8;

    if (value == HIGH) {
      PORTB |= (1 << bit);
    }
    else {
      PORTB &= ~(1 << bit);
    }
  }
}

uint8_t au_digital_read(uint8_t pin) {
  if (pin <= 7) {
    return (PIND & (1 << pin)) ? HIGH : LOW;
  }

  if (pin <= 13) {
    uint8_t bit = pin - 8;
    return (PINB & (1 << bit)) ? HIGH : LOW;
  }

  return LOW;
}

// Analog I/O

uint16_t au_analog_read(uint8_t pin) {
  ADMUX = (ADMUX & 0xF0) | (pin & 0x07);

  ADCSRA |= (1 << ADSC);

  while (ADCSRA & (1 << ADSC));

  return ADC;
}

void au_analog_write(uint8_t pin, uint8_t value) {
  switch (pin) {
  // Timer2 OC2B
  case 3:
    if (value == 0) {
      TCCR2A &= ~(1 << COM2B1);
      TCCR2A &= ~(1 << COM2B0);

      PORTD &= ~(1 << PORTD3);
    }
    else if (value == 255) {
      TCCR2A &= ~(1 << COM2B1);
      TCCR2A &= ~(1 << COM2B0);

      PORTD |= (1 << PORTD3);
    }
    else {
      DDRD |= (1 << DDD3);

      OCR2B = value;

      TCCR2A |= (1 << COM2B1);
      TCCR2A &= ~(1 << COM2B0);
    }
    break;
  // Timer0 OC0B
  case 5:
    if (value == 0) {
      /* PWM disabilitato, uscita LOW */
      TCCR0A &= ~(1 << COM0B1);
      TCCR0A &= ~(1 << COM0B0);
      PORTD &= ~(1 << PORTD5);
    }
    else if (value == 255) {
      /* PWM disabilitato, uscita HIGH */
      TCCR0A &= ~(1 << COM0B1);
      TCCR0A &= ~(1 << COM0B0);
      PORTD |= (1 << PORTD5);
    }
    else {
      /* PWM non-invertente */
      DDRD |= (1 << DDD5);

      TCCR0A |= (1 << COM0B1);
      TCCR0A &= ~(1 << COM0B0);

      OCR0B = value;
    }
    break;
  // Timer0 OC0A
  case 6:
    if (value == 0) {
      /* PWM disabilitato, uscita LOW */
      TCCR0A &= ~(1 << COM0A1);
      TCCR0A &= ~(1 << COM0A0);
      PORTD &= ~(1 << PORTD6);
    }
    else if (value == 255) {
      /* PWM disabilitato, uscita HIGH */
      TCCR0A &= ~(1 << COM0A1);
      TCCR0A &= ~(1 << COM0A0);
      PORTD |= (1 << PORTD6);
    }
    else {
      /* PWM non-invertente */
      DDRD |= (1 << DDD6);

      TCCR0A |= (1 << COM0A1);
      TCCR0A &= ~(1 << COM0A0);

      OCR0A = value;
    }
    break;
  // Timer1 OC1A
  case 9:
    if (value == 0) {
        TCCR1A &= ~(1 << COM1A1);
        TCCR1A &= ~(1 << COM1A0);
        PORTB &= ~(1 << PORTB1);
    }
    else if (value == 255) {
        TCCR1A &= ~(1 << COM1A1);
        TCCR1A &= ~(1 << COM1A0);
        PORTB |= (1 << PORTB1);
    }
    else {
        DDRB |= (1 << DDB1);

        OCR1A = value;

        TCCR1A |= (1 << COM1A1);
        TCCR1A &= ~(1 << COM1A0);
    }
    break;
  // Timer1 OC1B
  case 10:
    if (value == 0) {
        TCCR1A &= ~(1 << COM1B1);
        TCCR1A &= ~(1 << COM1B0);
        PORTB &= ~(1 << PORTB2);
    }
    else if (value == 255) {
        TCCR1A &= ~(1 << COM1B1);
        TCCR1A &= ~(1 << COM1B0);
        PORTB |= (1 << PORTB2);
    }
    else {
        DDRB |= (1 << DDB2);

        OCR1B = value;

        TCCR1A |= (1 << COM1B1);
        TCCR1A &= ~(1 << COM1B0);
    }
    break;
  // Timer2 OC2A
  case 11:
    if (value == 0) {
      TCCR2A &= ~(1 << COM2A1);
      TCCR2A &= ~(1 << COM2A0);

      PORTB &= ~(1 << PORTB3);
    }
    else if (value == 255) {
      TCCR2A &= ~(1 << COM2A1);
      TCCR2A &= ~(1 << COM2A0);

      PORTB |= (1 << PORTB3);
    }
    else {
      DDRB |= (1 << DDB3);

      OCR2A = value;

      TCCR2A |= (1 << COM2A1);
      TCCR2A &= ~(1 << COM2A0);
    }
    break;
  }
}

// Serial

void au_serial_init(uint32_t baud) {
  // baud = F_CPU / (16 * (UBRR0 + 1))
  uint16_t ubrr = (F_CPU / (16UL * baud)) - 1;

  UBRR0H = (uint8_t)(ubrr >> 8);
  UBRR0L = (uint8_t)ubrr;

  // Enable TX and RX
  UCSR0B = (1 << TXEN0);

  // 8 bit, 1 stop bit, no parity
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

static void au_serial_writec(uint8_t c) {
  // Wait data registry to be ready
  while (!(UCSR0A & (1 << UDRE0)));

  UDR0 = c;
}

void au_serial_print_str(const char *s) {
  while (*s)
    au_serial_writec((uint8_t)*s++);
}

void au_serial_println_str(const char *s) {
  au_serial_print_str(s);
  au_serial_writec('\r');
  au_serial_writec('\n');
}

void au_serial_print_uint(uint32_t value) {
  char buffer[10];
  uint8_t i = 0;

  if (value == 0) {
    au_serial_writec('0');
    return;
  }

  while (value > 0) {
    buffer[i++] = '0' + (value % 10);
    value /= 10;
  }

  while (i > 0) {
    au_serial_writec(buffer[--i]);
  }
}

void au_serial_println_uint(uint32_t value) {
  au_serial_print_uint(value);
  au_serial_writec('\r');
  au_serial_writec('\n');
}

void au_serial_print_int(int32_t value) {
  if (value < 0) {
    au_serial_writec('-');

    au_serial_print_uint((uint32_t)(-(value + 1)) + 1);
  }
  else {
    au_serial_print_uint((uint32_t)value);
  }
}

void au_serial_println_int(int32_t value) {
  au_serial_print_int(value);
  au_serial_writec('\r');
  au_serial_writec('\n');
}

// Tone

static volatile uint8_t tone_active = 0;
static volatile uint8_t tone_pin = 0;
static volatile uint32_t tone_end = 0;
static volatile uint32_t tone_count = 0;
static volatile uint32_t tone_count_max = 0;

ISR(TIMER1_COMPA_vect) {
    if (!tone_active)
        return;

    if (tone_pin <= 7)
        PORTD ^= (1 << tone_pin);
    else if (tone_pin <= 13)
        PORTB ^= (1 << (tone_pin - 8));

    if (tone_count_max != 0)
    {
        tone_count++;

        if (tone_count >= tone_count_max)
        {
            tone_active = 0;

            if (tone_pin <= 7)
                PORTD &= ~(1 << tone_pin);
            else if (tone_pin <= 13)
                PORTB &= ~(1 << (tone_pin - 8));

            TIMSK1 &= ~(1 << OCIE1A);
        }
    }
}

void au_tone(uint8_t pin, unsigned int frequency, unsigned long duration) {
    uint32_t ocr;

    if (frequency == 0)
        return;

    tone_pin = pin;

    au_pin_mode(pin, OUTPUT);

    /*
     * Timer1 CTC
     * Prescaler = 8
     *
     * Il pin viene invertito ad ogni compare match.
     */

    ocr = (F_CPU / (2UL * 8UL * frequency)) - 1;

    if (ocr > 65535)
        ocr = 65535;

    TCCR1A = 0;

    TCCR1B =
        (1 << WGM12) |
        (1 << CS11);

    OCR1A = (uint16_t)ocr;

    TCNT1 = 0;

    /*
     * Calcola la durata usando la frequenza reale
     * degli interrupt del timer.
     */

    if (duration != 0)
    {
        uint32_t interrupt_frequency;

        interrupt_frequency =
            F_CPU / (8UL * ((uint32_t)ocr + 1UL));

        tone_count_max =
            ((uint32_t)duration * interrupt_frequency) / 1000UL;

        if (tone_count_max == 0)
            tone_count_max = 1;
    }
    else
    {
        /* durata 0 = tono continuo */
        tone_count_max = 0;
    }

    tone_count = 0;
    tone_active = 1;

    TIMSK1 |= (1 << OCIE1A);
}

void au_no_tone(uint8_t pin) {
    if (!tone_active || pin != tone_pin)
        return;

    tone_active = 0;

    TIMSK1 &= ~(1 << OCIE1A);

    if (pin <= 7)
        PORTD &= ~(1 << pin);
    else if (pin <= 13)
        PORTB &= ~(1 << (pin - 8));

    TCCR1A = 0;
    TCCR1B = 0;

    TCNT1 = 0;
}

static inline uint8_t au_pin_read_fast(uint8_t pin) {
  if (pin <= 7)
    return (PIND & (1 << pin)) != 0;

  if (pin <= 13)
    return (PINB & (1 << (pin - 8))) != 0;

  return 0;
}

uint32_t au_pulse_in(uint8_t pin, uint8_t state, uint32_t timeout) {
  uint32_t start;
  uint32_t pulse_start;
  uint32_t pulse_end;

  /* Attendi la fine dell'eventuale impulso precedente */
  start = au_micros();

  while (au_pin_read_fast(pin) == state) {
    if ((uint32_t)(au_micros() - start) >= timeout) {
      return 0;
    }
  }

  /* Attendi l'inizio dell'impulso */
  start = au_micros();

  while (au_pin_read_fast(pin) != state) {
    if ((uint32_t)(au_micros() - start) >= timeout) {
      return 0;
    }
  }

  pulse_start = au_micros();

  /*
    * Misura dell'impulso.
    *
    * Polling diretto del registro PINx.
    */
  cli();

  while (au_pin_read_fast(pin) == state);

  pulse_end = au_micros();

  sei();

  return pulse_end - pulse_start;
}

uint32_t au_pulse_in_long(uint8_t pin, uint8_t state, uint32_t timeout) {
  uint32_t start;
  uint32_t pulse_start;

  /* Attendi che finisca un eventuale impulso già presente */
  start = au_micros();

  while (au_pin_read_fast(pin) == state) {
    if ((uint32_t)(au_micros() - start) >= timeout) {
      return 0;
    }
  }

  /* Attendi l'inizio dell'impulso */
  start = au_micros();

  while (au_pin_read_fast(pin) != state) {
    if ((uint32_t)(au_micros() - start) >= timeout) {
      return 0;
    }
  }

  pulse_start = au_micros();

  /* Misura l'impulso */
  while (au_pin_read_fast(pin) == state) {
    if ((uint32_t)(au_micros() - pulse_start) >= timeout) {
      return 0;
    }
  }

  return au_micros() - pulse_start;
}

uint8_t au_shift_in(uint8_t data_pin, uint8_t clock_pin, uint8_t bit_order) {
  uint8_t i;
  uint8_t value = 0;

  au_pin_mode(data_pin, INPUT);
  au_pin_mode(clock_pin, OUTPUT);

  for (i = 0; i < 8; i++) {
    au_digital_write(clock_pin, HIGH);

    if (au_digital_read(data_pin)) {
      if (bit_order == LSBFIRST) {
        value |= (1 << i);
      }
      else {
        value |= (1 << (7 - i));
      }
    }

    au_digital_write(clock_pin, LOW);
  }

  return value;
}

void au_shift_out(uint8_t data_pin, uint8_t clock_pin, uint8_t bit_order, uint8_t value) {
  uint8_t i;

  au_pin_mode(data_pin, OUTPUT);
  au_pin_mode(clock_pin, OUTPUT);

  for (i = 0; i < 8; i++) {
    if (bit_order == LSBFIRST) {
      au_digital_write(data_pin, (value >> i) & 1);
    }
    else {
      au_digital_write(data_pin, (value >> (7 - i)) & 1);
    }

    au_digital_write(clock_pin, HIGH);
    au_digital_write(clock_pin, LOW);
  }
}

// External interrupts

static void (*int0_function)(void) = 0;
static void (*int1_function)(void) = 0;

ISR(INT0_vect) {
  if (int0_function) {
    int0_function();
  }
}

ISR(INT1_vect) {
  if (int1_function) {
    int1_function();
  }
}

void au_attach_interrupt(uint8_t interrupt, void (*function)(void), uint8_t mode) {
  if (interrupt == 0) {
    EICRA &= ~((1 << ISC01) | (1 << ISC00));

    switch (mode) {
    case LOW:
      break;

    case CHANGE:
      EICRA |= (1 << ISC00);
      break;

    case FALLING:
      EICRA |= (1 << ISC01);
      break;

    case RISING:
      EICRA |= (1 << ISC01) | (1 << ISC00);
      break;
    }

    int0_function = function;

    EIMSK |= (1 << INT0);
  }
  else if (interrupt == 1) {
    EICRA &= ~((1 << ISC11) | (1 << ISC10));

    switch (mode) {
    case LOW:
        break;
    case CHANGE:
        EICRA |= (1 << ISC10);
        break;
    case FALLING:
        EICRA |= (1 << ISC11);
        break;
    case RISING:
        EICRA |= (1 << ISC11) | (1 << ISC10);
        break;
    }

    int1_function = function;

    EIMSK |= (1 << INT1);
  }
}

void au_detach_interrupt(uint8_t interrupt) {
  if (interrupt == 0) {
    EIMSK &= ~(1 << INT0);
    int0_function = 0;
  }
  else if (interrupt == 1) {
    EIMSK &= ~(1 << INT1);
    int1_function = 0;
  }
}

int8_t au_digital_pin_to_interrupt(uint8_t pin) {
  switch (pin) {
  case 2:
    return 0;   // INT0
  case 3:
    return 1;   // INT1
  default:
    return -1;
  }
}