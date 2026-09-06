#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "aurum_private.h"

// timerx_toggle_count:
//  > 0 - duration specified
//  = 0 - stopped
//  < 0 - infinitely
volatile long timer2_toggle_count;

volatile uint8_t *timer2_pin_port;
volatile uint8_t timer2_pin_mask;

#define AVAILABLE_TONE_PINS 1
#define USE_TIMER2

const uint8_t PROGMEM tone_pin_to_timer_PGM[] = { 2 };
static uint8_t tone_pins[AVAILABLE_TONE_PINS] = { 255 };

static int8_t au_tone_begin(uint8_t pin) {
  int8_t timer = -1;

  // if we're already using the pin, the timer should be configured.  
  for (int i = 0; i < AVAILABLE_TONE_PINS; i++) {
    if (tone_pins[i] == pin) {
      return pgm_read_byte(tone_pin_to_timer_PGM + i);
    }
  }
  
  // search for an unused timer.
  for (int i = 0; i < AVAILABLE_TONE_PINS; i++) {
    if (tone_pins[i] == 255) {
      tone_pins[i] = pin;
      timer = pgm_read_byte(tone_pin_to_timer_PGM + i);
      break;
    }
  }

  if (timer == -1) {
    return timer;
  }

  // Set timer specific stuff
  // All timers in CTC mode
  // 8 bit timers will require changing prescalar values,
  // whereas 16 bit timers are set to either ck/1 or ck/64 prescalar
  if (timer == 2) {
    // 8 bit timer
    TCCR2A = 0;
    TCCR2B = 0;
    au_bit_write(TCCR2A, WGM21, 1);
    au_bit_write(TCCR2B, CS20, 1);
    timer2_pin_port = au_port_output_register(au_digital_pin_to_port(pin));
    timer2_pin_mask = au_digital_pin_to_bit_mask(pin);
  }

  return timer;
}

void au_tone(uint8_t _pin, unsigned int frequency, unsigned long duration) {
  uint8_t prescalarbits = 0b001;
  uint32_t ocr = 0;
  int8_t _timer;

  _timer = au_tone_begin(_pin);

  // Timer2 is the only timer available for tone().
  if (_timer < 0) {
    return;
  }

  // Set the pinMode as OUTPUT
  au_pin_mode(_pin, AU_OUTPUT);

  // we are using an 8 bit timer, scan through prescalars to find the best fit
  ocr = F_CPU / frequency / 2 - 1;
  prescalarbits = 0b001;  // ck/1: same for both timers

  if (ocr > 255) {
    ocr = F_CPU / frequency / 2 / 8 - 1;
    prescalarbits = 0b010;  // ck/8: same for both timers

    if (_timer == 2 && ocr > 255) {
      ocr = F_CPU / frequency / 2 / 32 - 1;
      prescalarbits = 0b011;
    }

    if (ocr > 255) {
      ocr = F_CPU / frequency / 2 / 64 - 1;
      prescalarbits = _timer == 0 ? 0b011 : 0b100;

      if (_timer == 2 && ocr > 255) {
        ocr = F_CPU / frequency / 2 / 128 - 1;
        prescalarbits = 0b101;
      }

      if (ocr > 255) {
        ocr = F_CPU / frequency / 2 / 256 - 1;
        prescalarbits = _timer == 0 ? 0b100 : 0b110;

        if (ocr > 255) {
          // can't do any better than /1024
          ocr = F_CPU / frequency / 2 / 1024 - 1;
          prescalarbits = _timer == 0 ? 0b101 : 0b111;
        }
      }
    }
  }

  TCCR2B = (TCCR2B & 0b11111000) | prescalarbits;

  // Calculate the toggle count
  if (duration > 0) {
    timer2_toggle_count = 2 * frequency * duration / 1000;
  }
  else {
    timer2_toggle_count = -1;
  }

  // Set the OCR for the timer,
  // then turn on the interrupts
  OCR2A = ocr;
  au_bit_write(TIMSK2, OCIE2A, 1);
}

static void au_disable_timer(uint8_t timer) {
  if (timer != 2) {
    return;
  }

  au_bit_write(TIMSK2, OCIE2A, 0); // disable interrupt

  TCCR2A = (1 << WGM20);

  TCCR2B = (TCCR2B & 0b11111000) | (1 << CS22);

  OCR2A = 0;
}

void au_no_tone(uint8_t pin) {
  int8_t timer = -1;
    
  for (int i = 0; i < AVAILABLE_TONE_PINS; i++) {
    if (tone_pins[i] == pin) {
      timer = pgm_read_byte(tone_pin_to_timer_PGM + i);
      tone_pins[i] = 255;
      break;
    }
  }
  
  au_disable_timer(timer);

  au_digital_write(pin, 0);
}

ISR(TIMER2_COMPA_vect) {
  if (timer2_toggle_count != 0) {
    // toggle the pin
    *timer2_pin_port ^= timer2_pin_mask;

    if (timer2_toggle_count > 0) {
      timer2_toggle_count--;
    }
  }
  else {
    // timer gets initialized next time we call tone().
    // XXX: this assumes timer 2 is always the first one used.
    au_no_tone(tone_pins[0]);
    // disableTimer(2);
    // *timer2_pin_port &= ~(timer2_pin_mask);
    // keep pin low after stop
  }
}