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
#include <avr/pgmspace.h>

#ifndef F_CPU
# warning "F_CPU not defined for <aurum.h>"
# define F_CPU 16000000UL
#endif

#define AU_NOT_A_PIN 0
#define AU_NOT_A_PORT 0
#define AU_NOT_ON_TIMER 0
#define AU_TIMER0A 1
#define AU_TIMER0B 2
#define AU_TIMER1A 3
#define AU_TIMER1B 4
#define AU_TIMER1C 5
#define AU_TIMER2  6
#define AU_TIMER2A 7
#define AU_TIMER2B 8
#define AU_PORT_B 0
#define AU_PORT_C 1
#define AU_PORT_D 2

extern const uint16_t PROGMEM port_to_mode_PGM[];
extern const uint16_t PROGMEM port_to_input_PGM[];
extern const uint16_t PROGMEM port_to_output_PGM[];
extern const uint8_t PROGMEM digital_pin_to_port_PGM[];
extern const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[];
extern const uint8_t PROGMEM digital_pin_to_timer_PGM[];

#define au_digital_pin_to_port(P) ( pgm_read_byte( digital_pin_to_port_PGM + (P) ) )
#define au_digital_pin_to_bit_mask(P) ( pgm_read_byte( digital_pin_to_bit_mask_PGM + (P) ) )
#define au_digital_pin_to_timer(P) ( pgm_read_byte( digital_pin_to_timer_PGM + (P) ) )
#define au_analog_in_pin_to_bit(P) (P)
#define au_port_output_register(P) ( (volatile uint8_t *)( pgm_read_word( port_to_output_PGM + (P))) )
#define au_port_input_register(P) ( (volatile uint8_t *)( pgm_read_word( port_to_input_PGM + (P))) )
#define au_port_mode_register(P) ( (volatile uint8_t *)( pgm_read_word( port_to_mode_PGM + (P))) )

#define au_clock_cycles_per_microsecond() ( F_CPU / 1000000L )
#define au_clock_cycles_to_microseconds(a) ( (a) / au_clock_cycles_per_microsecond() )
#define au_microseconds_to_clock_cycles(a) ( (a) * au_clock_cycles_per_microsecond() )

#define au_bit_read(value, bit) (((value) >> (bit)) & 0x01)
#define au_bit_set(value, bit) ((value) |= (1UL << (bit)))
#define au_bit_clear(value, bit) ((value) &= ~(1UL << (bit)))
#define au_bit_toggle(value, bit) ((value) ^= (1UL << (bit)))
#define au_bit_write(value, bit, bitvalue) ((bitvalue) ? au_bit_set(value, bit) : au_bit_clear(value, bit))

/*
* SYSTEM
*
* au_init needs to be called before using other functions, it takes care
* of setting up timers, ADC and PWM related functions
*/

void au_init(void);


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

#define au_digital_pin_to_interrupt(p) ((p) == 2 ? 0 : ((p) == 3 ? 1 : NOT_AN_INTERRUPT))

void au_attach_interrupt(uint8_t interrupt, void (*function)(void), int mode);
void au_detach_interrupt(uint8_t interrupt);

/*
* SERIAL
*
* Functions to read and write on the serial port
*/

#define SERIAL_5N1 0x00
#define SERIAL_6N1 0x02
#define SERIAL_7N1 0x04
#define SERIAL_8N1 0x06

#define SERIAL_5N2 0x08
#define SERIAL_6N2 0x0A
#define SERIAL_7N2 0x0C
#define SERIAL_8N2 0x0E

#define SERIAL_5E1 0x20
#define SERIAL_6E1 0x22
#define SERIAL_7E1 0x24
#define SERIAL_8E1 0x26

#define SERIAL_5E2 0x28
#define SERIAL_6E2 0x2A
#define SERIAL_7E2 0x2C
#define SERIAL_8E2 0x2E

#define SERIAL_5O1 0x30
#define SERIAL_6O1 0x32
#define SERIAL_7O1 0x34
#define SERIAL_8O1 0x36

#define SERIAL_5O2 0x38
#define SERIAL_6O2 0x3A
#define SERIAL_7O2 0x3C
#define SERIAL_8O2 0x3E

void au_serial_begin(uint32_t baud, uint8_t config);
void au_serial_end(void);
int au_serial_available(void);
int au_serial_peek(void);
int au_serial_read(void);
int au_serial_available_for_write(void);
void au_serial_flush(void);
size_t au_serial_write(uint8_t c);
void au_serial_print_str(const char *s);
void au_serial_println_str(const char *s);
void au_serial_print_uint(uint32_t value);
void au_serial_println_uint(uint32_t value);
void au_serial_print_int(int32_t value);
void au_serial_println_int(int32_t value);


/*
* EEPROM
*
* Functions to read and write on the integrated EEPROM.
* ATmega328P EEPROM size: 1024 bytes, valid addresses: 0..1023
*/

#define AU_EEPROM_SIZE 1024U
#define AU_EEPROM_END  (AU_EEPROM_SIZE - 1U)

uint8_t au_eeprom_read(uint16_t address);
void au_eeprom_write(uint16_t address, uint8_t value);
void au_eeprom_update(uint16_t address, uint8_t value);
void au_eeprom_get(uint16_t address, void *data, uint16_t size);
void au_eeprom_put(uint16_t address, const void *data, uint16_t size);

#endif