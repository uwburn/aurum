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


#ifndef AURUM_H
#define AURUM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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
void au_serial_end(void);
int au_serial_available(void);
int au_serial_peek(void);
int au_serial_read(void);
int au_serial_available_for_write(void);
void au_serial_flush(void);
int au_serial_write(uint8_t c);
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

/*
* SOFTWARE SERIAL
*
* Allows serial communication on other digital pins of the board, using
* software to replicate the functionality. 
*
* The following limitations are present:
* - It cannot transmit and receive data at the same time.
* - If using multiple software serial ports, only one can receive data
*   at a time.
*/

typedef struct {
  /* RX pin */
  uint8_t rx_pin;
  uint8_t rx_bit_mask;
  volatile uint8_t *rx_port_register;

  /* TX pin */
  uint8_t tx_pin;
  uint8_t tx_bit_mask;
  volatile uint8_t *tx_port_register;

  /* Pin Change Interrupt */
  volatile uint8_t *pcint_mask_register;
  uint8_t pcint_mask_value;

  /* Timing - expressed as 4-cycle delays */
  uint16_t rx_delay_centering;
  uint16_t rx_delay_intrabit;
  uint16_t rx_delay_stopbit;
  uint16_t tx_delay;

  /* State */
  uint8_t buffer_overflow;
  uint8_t inverse_logic;

  uint8_t listening;
} au_software_serial_t;

void au_software_serial_init(au_software_serial_t *serial, uint8_t rx_pin, uint8_t tx_pin, bool inverse_logic);
void au_software_serial_begin(au_software_serial_t *serial, uint32_t baud);
void au_software_serial_end(au_software_serial_t *serial);
bool au_software_serial_listen(au_software_serial_t *serial);
void au_software_serial_stop_listening(au_software_serial_t *serial);
bool au_software_serial_is_listening(const au_software_serial_t *serial);
int au_software_serial_available(const au_software_serial_t *serial);
int au_software_serial_read(au_software_serial_t *serial);
int au_software_serial_peek(const au_software_serial_t *serial);
bool au_software_serial_overflow(au_software_serial_t *serial);
size_t au_software_serial_write(au_software_serial_t *serial, uint8_t value);
void au_software_serial_flush(au_software_serial_t *serial);
void au_software_serial_print_str(au_software_serial_t *serial, const char *s);
void au_software_serial_println_str(au_software_serial_t *serial, const char *s);
void au_software_serial_print_uint(au_software_serial_t *serial, uint32_t value);
void au_software_serial_println_uint(au_software_serial_t *serial, uint32_t value);
void au_software_serial_print_int(au_software_serial_t *serial, int32_t value);
void au_software_serial_println_int(au_software_serial_t *serial, int32_t value);

#endif