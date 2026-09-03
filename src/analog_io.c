#include <avr/io.h>

#include "aurum.h"
#include "aurum_private.h"

// OK

uint8_t analog_reference = DEFAULT;

void au_analog_reference(uint8_t mode) {
  // can't actually set the register here because the default setting
	// will connect AVCC and the AREF pin, which would cause a short if
	// there's something connected to AREF.
  analog_reference = mode;
}

uint16_t au_analog_read(uint8_t pin) {
  if (pin > 5) {
    return 0;
  }

	// set the analog reference (high two bits of ADMUX) and select the
	// channel (low 4 bits).  this also sets ADLAR (left-adjust result)
	// to 0 (the default).
  ADMUX = (analog_reference << 6) | (pin & 0x07);

  // start the conversion
  sbi(ADCSRA, ADSC);

  // ADSC is cleared when the conversion finishes
  while (bit_is_set(ADCSRA, ADSC));

  // ADC macro takes care of reading ADC register.
	// avr-gcc implements the proper reading order: ADCL is read first.
  return ADC;
}

void au_analog_write(uint8_t pin, uint8_t value) {
  // We need to make sure the PWM output is enabled for those pins
	// that support it, as we turn it off when digitally reading or
	// writing with them.  Also, make sure the pin is in output mode
	// for consistenty with Wiring, which doesn't require a pinMode
	// call for the analog output pins.
  au_pin_mode(pin, OUTPUT);

  if (value <= 0) {
    au_digital_write(pin, LOW);
    return;
  }

  if (value >= 255) {
    au_digital_write(pin, HIGH);
    return;
  }

  switch (au_digital_pin_to_timer(pin)) {
  case AU_TIMER0A:
    sbi(TCCR0A, COM0A1);
    OCR0A = value;
    break;
  case AU_TIMER0B:
    sbi(TCCR0A, COM0B1);
    OCR0B = value;
    break;
  case AU_TIMER1A:
    sbi(TCCR1A, COM1A1);
    OCR1A = value;
    break;
  case AU_TIMER1B:
    sbi(TCCR1A, COM1B1);
    OCR1B = value;
    break;
  case AU_TIMER2A:
    sbi(TCCR2A, COM2A1);
    OCR2A = value;
    break;
  case AU_TIMER2B:
    sbi(TCCR2A, COM2B1);
    OCR2B = value;
    break;
  case AU_NOT_ON_TIMER:
  default:
    if (value < 128) {
      au_digital_write(pin, LOW);
    }
    else {
      au_digital_write(pin, HIGH);
    }
    break;
  }
}