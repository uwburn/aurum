#include <avr/io.h>
#include <avr/interrupt.h>

#include "aurum_private.h"

static void au_turn_off_pwm(uint8_t timer) {
  switch (timer) {
  case AU_TIMER1A:
    cbi(TCCR1A, COM1A1);
    break;
  case AU_TIMER1B:
    cbi(TCCR1A, COM1B1);
    break;
  case AU_TIMER0A:
    cbi(TCCR0A, COM0A1);
    break;
  case AU_TIMER0B:
    cbi(TCCR0A, COM0B1);
    break;
  case AU_TIMER2A:
    cbi(TCCR2A, COM2A1);
    break;
  case AU_TIMER2B:
    cbi(TCCR2A, COM2B1);
    break;
  }
}

void au_pin_mode(uint8_t pin, uint8_t mode) {
	uint8_t bit = au_digital_pin_to_bit_mask(pin);
	uint8_t port = au_digital_pin_to_port(pin);
	volatile uint8_t *reg, *out;

	if (port == AU_NOT_A_PIN) {
    return;
  }

	// JWS: can I let the optimizer do this?
	reg = au_port_mode_register(port);
	out = au_port_output_register(port);

	if (mode == AU_INPUT) { 
		uint8_t oldSREG = SREG;
    cli();
		*reg &= ~bit;
		*out &= ~bit;
		SREG = oldSREG;
	}
  else if (mode == AU_INPUT_PULLUP) {
		uint8_t oldSREG = SREG;
    cli();
		*reg &= ~bit;
		*out |= bit;
		SREG = oldSREG;
	}
  else {
		uint8_t oldSREG = SREG;
    cli();
		*reg |= bit;
		SREG = oldSREG;
	}
}

void au_digital_write(uint8_t pin, uint8_t value) {
  uint8_t timer = au_digital_pin_to_timer(pin);
	uint8_t bit = au_digital_pin_to_bit_mask(pin);
	uint8_t port = au_digital_pin_to_port(pin);
	volatile uint8_t *out;

	if (port == AU_NOT_A_PIN) {
    return;
  }

	// If the pin that support PWM output, we need to turn it off
	// before doing a digital write.
	if (timer != AU_NOT_ON_TIMER) {
    au_turn_off_pwm(timer);
  }

	out = au_port_output_register(port);

	uint8_t oldSREG = SREG;
	cli();

	if (value == AU_LOW) {
		*out &= ~bit;
	} else {
		*out |= bit;
	}

	SREG = oldSREG;
}

uint8_t au_digital_read(uint8_t pin) {
  uint8_t timer = au_digital_pin_to_timer(pin);
	uint8_t bit = au_digital_pin_to_bit_mask(pin);
	uint8_t port = au_digital_pin_to_port(pin);

	if (port == AU_NOT_A_PIN) {
    return AU_LOW;
  }

	// If the pin that support PWM output, we need to turn it off
	// before getting a digital reading.
	if (timer != AU_NOT_ON_TIMER) {
    au_turn_off_pwm(timer);
  }

	if (*au_port_input_register(port) & bit) {
    return AU_HIGH;
  }
	return AU_LOW;
}