#include <avr/io.h>
#include <avr/interrupt.h>

#include "aurum.h"
#include "aurum_private.h"

// OK

/* Measures the length (in microseconds) of a pulse on the pin; state is HIGH
 * or LOW, the type of pulse to measure.  Works on pulses from 2-3 microseconds
 * to 3 minutes in length, but must be called at least a few dozen microseconds
 * before the start of the pulse.
 *
 * This function performs better with short pulses in noInterrupt() context
 */
uint32_t au_pulse_in(uint8_t pin, uint8_t state, uint32_t timeout) {
	// cache the port and bit of the pin in order to speed up the
	// pulse width measuring loop and achieve finer resolution.  calling
	// digitalRead() instead yields much coarser resolution.
	uint8_t bit = au_digital_pin_to_bit_mask(pin);
	uint8_t port = au_digital_pin_to_port(pin);
	uint8_t state_mask = (state ? bit : 0);

	// convert the timeout from microseconds to a number of times through
	// the initial loop; it takes approximately 16 clock cycles per iteration
	uint32_t max_loops = au_microseconds_to_clock_cycles(timeout) / 16;

	uint32_t width = au_count_pulse_asm(au_port_input_register(port), bit, state_mask, max_loops);

	// prevent au_clock_cycles_to_microseconds to return bogus values if au_count_pulse_asm timed out
	if (width) {
		return au_clock_cycles_to_microseconds(width * 16 + 16);
  }
	else {
		return 0;
  }
}

/* Measures the length (in microseconds) of a pulse on the pin; state is HIGH
 * or LOW, the type of pulse to measure.  Works on pulses from 2-3 microseconds
 * to 3 minutes in length, but must be called at least a few dozen microseconds
 * before the start of the pulse.
 *
 * ATTENTION:
 * this function relies on au_micros() so cannot be used in noInterrupt() context
 */
uint32_t au_pulse_in_long(uint8_t pin, uint8_t state, uint32_t timeout) {
	// cache the port and bit of the pin in order to speed up the
	// pulse width measuring loop and achieve finer resolution.  calling
	// digitalRead() instead yields much coarser resolution.
	uint8_t bit = au_digital_pin_to_bit_mask(pin);
	uint8_t port = au_digital_pin_to_port(pin);
	uint8_t state_mask = (state ? bit : 0);

	uint32_t start_micros = au_micros();

	// wait for any previous pulse to end
	while ((*au_port_input_register(port) & bit) == state_mask) {
		if (au_micros() - start_micros > timeout)
			return 0;
	}

	// wait for the pulse to start
	while ((*au_port_input_register(port) & bit) != state_mask) {
		if (au_micros() - start_micros > timeout)
			return 0;
	}

	uint32_t start = au_micros();
	// wait for the pulse to stop
	while ((*au_port_input_register(port) & bit) == state_mask) {
		if (au_micros() - start_micros > timeout)
			return 0;
	}
	return au_micros() - start;
}
