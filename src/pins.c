#include "aurum.h"

const uint16_t PROGMEM port_to_mode_PGM[] = {
	AU_NOT_A_PORT,
	AU_NOT_A_PORT,
	(uint16_t) &DDRB,
	(uint16_t) &DDRC,
	(uint16_t) &DDRD,
};

const uint16_t PROGMEM port_to_input_PGM[] = {
	AU_NOT_A_PORT,
	AU_NOT_A_PORT,
	(uint16_t) &PINB,
	(uint16_t) &PINC,
	(uint16_t) &PIND,
};

const uint16_t PROGMEM port_to_output_PGM[] = {
	AU_NOT_A_PORT,
	AU_NOT_A_PORT,
	(uint16_t) &PORTB,
	(uint16_t) &PORTC,
	(uint16_t) &PORTD,
};

const uint8_t PROGMEM digital_pin_to_port_PGM[] = {
	AU_PORT_D, /* 0 */
	AU_PORT_D,
	AU_PORT_D,
	AU_PORT_D,
	AU_PORT_D,
	AU_PORT_D,
	AU_PORT_D,
	AU_PORT_D,
	AU_PORT_B, /* 8 */
	AU_PORT_B,
	AU_PORT_B,
	AU_PORT_B,
	AU_PORT_B,
	AU_PORT_B,
	AU_PORT_C, /* 14 */
	AU_PORT_C,
	AU_PORT_C,
	AU_PORT_C,
	AU_PORT_C,
	AU_PORT_C,
};

const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[] = {
	_BV(0), /* 0, port D */
	_BV(1),
	_BV(2),
	_BV(3),
	_BV(4),
	_BV(5),
	_BV(6),
	_BV(7),
	_BV(0), /* 8, port B */
	_BV(1),
	_BV(2),
	_BV(3),
	_BV(4),
	_BV(5),
	_BV(0), /* 14, port C */
	_BV(1),
	_BV(2),
	_BV(3),
	_BV(4),
	_BV(5),
};

const uint8_t PROGMEM digital_pin_to_timer_PGM[] = {
	AU_NOT_ON_TIMER, /* 0 - port D */
	AU_NOT_ON_TIMER,
	AU_NOT_ON_TIMER,
	// on the ATmega168, digital pin 3 has hardware pwm
	AU_TIMER2B,
	AU_NOT_ON_TIMER,
	// on the ATmega168, digital pins 5 and 6 have hardware pwm
	AU_TIMER0B,
	AU_TIMER0A,
	AU_NOT_ON_TIMER,
	AU_NOT_ON_TIMER, /* 8 - port B */
	AU_TIMER1A,
	AU_TIMER1B,
	AU_TIMER2A,
	AU_NOT_ON_TIMER,
	AU_NOT_ON_TIMER,
	AU_NOT_ON_TIMER,
	AU_NOT_ON_TIMER, /* 14 - port C */
	AU_NOT_ON_TIMER,
	AU_NOT_ON_TIMER,
	AU_NOT_ON_TIMER,
	AU_NOT_ON_TIMER,
};