#include <avr/io.h>
#include <avr/interrupt.h>

#include "aurum.h"
#include "aurum_private.h"

// OK

void au_init() {
	// this needs to be called before setup() or some functions won't
	// work there
  sei();

  // Timer 0: Fast PWM mode.
  sbi(TCCR0A, WGM01);
	sbi(TCCR0A, WGM00);

  // Timer 0: prescaler = 64.
	sbi(TCCR0B, CS01);
	sbi(TCCR0B, CS00);

  // enable timer 0 overflow interrupt
  sbi(TIMSK0, TOIE0);

	// timers 1 and 2 are used for phase-correct hardware pwm
	// this is better for motors as it ensures an even waveform
	// note, however, that fast pwm mode can achieve a frequency of up
	// 8 MHz (with a 16 MHz clock) at 50% duty cycle
  TCCR1B = 0;

  // set timer 1 prescale factor to 64
	sbi(TCCR1B, CS11);
  sbi(TCCR1B, CS10);

  // put timer 1 in 8-bit phase correct pwm mode
  sbi(TCCR1A, WGM10);

	// set timer 2 prescale factor to 64
  sbi(TCCR2B, CS22);

  // configure timer 2 for phase correct pwm (8-bit)
  sbi(TCCR2A, WGM20);


  // set a2d prescaler so we are inside the desired 50-200 KHz range.
  sbi(ADCSRA, ADPS2);
	sbi(ADCSRA, ADPS1);
	sbi(ADCSRA, ADPS0);

  // enable a2d conversions
  sbi(ADCSRA, ADEN);

	// the bootloader connects pins 0 and 1 to the USART; disconnect them
	// here so they can be used as normal digital i/o; they will be
	// reconnected in Serial.begin()
  UCSR0B = 0;
}