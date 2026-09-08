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


#ifndef AURUM_SOFTWARE_SERIAL_H
#define AURUM_SOFTWARE_SERIAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "core.h"
#include "print.h"

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
bool au_software_serial_is_listening(au_software_serial_t *serial);
int au_software_serial_available(au_software_serial_t *serial);
int au_software_serial_read(au_software_serial_t *serial);
int au_software_serial_peek(au_software_serial_t *serial);
bool au_software_serial_overflow(au_software_serial_t *serial);
size_t au_software_serial_write(au_software_serial_t *serial, uint8_t value);
size_t au_software_serial_print(void *context, uint8_t value);
au_printer_t au_software_serial_build_printer(au_software_serial_t *serial);

#endif