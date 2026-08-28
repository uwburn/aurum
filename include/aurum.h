/*
* Aurum - Arduino Uno C API
*
* This library provide a didactical implementation of an Arduino like API in C
* for Arduino Uno boards. It targets, in particular the original Arduino Uno R1.
*
* The code is mostly written with LLM for enterteinment purpose, it's not aimed
* at real usage or for any kind of replacement of the original Arduino API.
*
* No wrappers for functions existing in AVR lib are provided, just use the
* original functions.
*/


#ifndef AURUM_H
#define AURUM_H

#include <stdint.h>

/*
* SYSTEM
*
* au_system_init needs to be called before using other functions, it takes care
* of setting up timers, ADC and PWM related functions
*/

void au_system_init(void);


/*
* TIME
*
* Time and delay related functions
*/

uint32_t au_micros(void);
uint32_t au_millis(void);
void au_delay(uint32_t ms);
void au_delay_microseconds(uint32_t us);


/*
* DIGITAL I/O
*
* Digital I/O mode, read and write functions plus constants definitions
*/

#define INPUT        0
#define OUTPUT       1
#define INPUT_PULLUP 2
#define LOW          0
#define HIGH         1

void au_pin_mode(uint8_t pin, uint8_t mode);
void au_digital_write(uint8_t pin, uint8_t value);
uint8_t au_digital_read(uint8_t pin);


/*
* ANALOG I/O
*
* Analog I/O reference, read and write functions plus constants definitions
*/

#define DEFAULT  1
#define INTERNAL 3
#define EXTERNAL 0
#define A0 0
#define A1 1
#define A2 2
#define A3 3
#define A4 4
#define A5 5

uint16_t au_analog_read(uint8_t pin);
void au_analog_reference(uint8_t mode);
void au_analog_write(uint8_t pin, uint8_t value);


/*
* ADVANCED I/O
*
* Functions to implement common I/O tasks
*/

#define LSBFIRST 0
#define MSBFIRST 1

void au_tone(uint8_t pin, unsigned int frequency, unsigned long duration);
void au_no_tone(uint8_t pin);
uint32_t au_pulse_in(uint8_t pin, uint8_t state, uint32_t timeout);
uint32_t au_pulse_in_long(uint8_t pin, uint8_t state, uint32_t timeout);
uint8_t au_shift_in(uint8_t data_pin, uint8_t clock_pin, uint8_t bit_order);
void au_shift_out(uint8_t data_pin, uint8_t clock_pin, uint8_t bit_order, uint8_t value);


/*
* EXTERNAL INTERRUPTS
* 
* Handling of interrupts on pins
*/

#define LOW       0
#define CHANGE    1
#define FALLING   2
#define RISING    3

void au_attach_interrupt(uint8_t interrupt, void (*function)(void), uint8_t mode);
void au_detach_interrupt(uint8_t interrupt);
int8_t au_digital_pin_to_interrupt(uint8_t pin);


/*
* SERIAL
*
* Functions to read and write on the serial port
*/

void au_serial_init(uint32_t baud);
void au_serial_print_str(const char *s);
void au_serial_println_str(const char *s);
void au_serial_print_uint(uint32_t value);
void au_serial_println_uint(uint32_t value);
void au_serial_print_int(int32_t value);
void au_serial_println_int(int32_t value);

#endif