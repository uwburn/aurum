/*
* Aurum - Arduino Uno C API
*
* This library provide a didactical implementation of an Arduino like API in C
* for Arduino Uno boards. It targets, in particular the original Arduino Uno R1.
*
* The code is mostly written with LLM for enterteinment purpose, it's not aimed
* at real usage or for any kind of replacement of the original Arduino API.
*
* No wrappers for functions existing in AVR lib-c are provided, just use the
* original functions.
*/


#ifndef AURUM_CORE_H
#define AURUM_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
* SYSTEM
*
* au_init needs to be called before using other functions, it takes care
* of setting up timers, ADC and PWM related functions
*/

void au_init();


/*
* TIME
*
* Time and delay related functions
*/

uint32_t au_micros();
uint32_t au_millis();
void au_delay(uint32_t ms);
void au_delay_microseconds(uint32_t us);


/*
* DIGITAL I/O
*
* Digital I/O mode, read and write functions plus constants definitions
*/

#define AU_INPUT          0
#define AU_OUTPUT         1
#define AU_INPUT_PULLUP   2
#define AU_LOW            0
#define AU_HIGH           1

void au_pin_mode(uint8_t pin, uint8_t mode);
void au_digital_write(uint8_t pin, uint8_t value);
uint8_t au_digital_read(uint8_t pin);


/*
* ANALOG I/O
*
* Analog I/O reference, read and write functions plus constants definitions
*/

#define AU_AREF_DEFAULT   1
#define AU_AREF_INTERNAL  3
#define AU_AREF_EXTERNAL  0
#define AU_A0             0
#define AU_A1             1
#define AU_A2             2
#define AU_A3             3
#define AU_A4             4
#define AU_A5             5

uint16_t au_analog_read(uint8_t pin);
void au_analog_reference(uint8_t mode);
void au_analog_write(uint8_t pin, uint8_t value);


/*
* ADVANCED I/O
*
* Functions to implement common I/O tasks
*/

#define AU_LSBFIRST 0
#define AU_MSBFIRST 1

uint32_t au_pulse_in(uint8_t pin, uint8_t state, uint32_t timeout);
uint32_t au_pulse_in_long(uint8_t pin, uint8_t state, uint32_t timeout);
uint8_t au_shift_in(uint8_t data_pin, uint8_t clock_pin, uint8_t bit_order);
void au_shift_out(uint8_t data_pin, uint8_t clock_pin, uint8_t bit_order, uint8_t value);


/*
* EXTERNAL INTERRUPTS
* 
* Handling of interrupts on pins
*/

#define AU_NOT_AN_INTERRUPT 0xFF
#define AU_LOW              0
#define AU_CHANGE           1
#define AU_FALLING          2
#define AU_RISING           3

#define au_digital_pin_to_interrupt(p) ((p) == 2 ? 0 : ((p) == 3 ? 1 : AU_NOT_AN_INTERRUPT))

void au_attach_interrupt(uint8_t interrupt, void (*function)(void), int mode);
void au_detach_interrupt(uint8_t interrupt);

/*
* SERIAL
*
* Functions to read and write on the serial port
*/

#define AU_SERIAL_5N1 0x00
#define AU_SERIAL_6N1 0x02
#define AU_SERIAL_7N1 0x04
#define AU_SERIAL_8N1 0x06

#define AU_SERIAL_5N2 0x08
#define AU_SERIAL_6N2 0x0A
#define AU_SERIAL_7N2 0x0C
#define AU_SERIAL_8N2 0x0E

#define AU_SERIAL_5E1 0x20
#define AU_SERIAL_6E1 0x22
#define AU_SERIAL_7E1 0x24
#define AU_SERIAL_8E1 0x26

#define AU_SERIAL_5E2 0x28
#define AU_SERIAL_6E2 0x2A
#define AU_SERIAL_7E2 0x2C
#define AU_SERIAL_8E2 0x2E

#define AU_SERIAL_5O1 0x30
#define AU_SERIAL_6O1 0x32
#define AU_SERIAL_7O1 0x34
#define AU_SERIAL_8O1 0x36

#define AU_SERIAL_5O2 0x38
#define AU_SERIAL_6O2 0x3A
#define AU_SERIAL_7O2 0x3C
#define AU_SERIAL_8O2 0x3E

void au_serial_begin(uint32_t baud, uint8_t config);
void au_serial_end();
int au_serial_available();
int au_serial_peek();
int au_serial_read();
int au_serial_available_for_write();
void au_serial_flush();
int au_serial_write(uint8_t c);
void au_serial_print_str(const char *s);
void au_serial_println_str(const char *s);
void au_serial_print_uint(uint32_t value);
void au_serial_println_uint(uint32_t value);
void au_serial_print_int(int32_t value);
void au_serial_println_int(int32_t value);

#endif