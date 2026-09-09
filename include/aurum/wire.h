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

#ifndef AURUM_WIRE_H
#define AURUM_WIRE_H

/*
* WIRE / I2C
*
* This header contains functions for using the wire/i2c interface, it matches
* the dedicated Arduino Wire library.
*/

#include <stdint.h>
#include <stddef.h>

#include "print.h"

void au_wire_begin();
void au_wire_begin_address(uint8_t address);
void au_wire_end();
void au_wire_set_clock(uint32_t clock);
void au_wire_set_timeout(uint32_t timeout_us, uint8_t reset_with_timeout);
uint8_t au_wire_get_timeout_flag();
void au_wire_clear_timeout_flag();
void au_wire_begin_transmission(uint8_t address);
uint8_t au_wire_end_transmission();
uint8_t au_wire_end_transmission_stop(uint8_t send_stop);
uint8_t au_wire_request_from(uint8_t address, uint8_t quantity);
uint8_t au_wire_request_from_stop(uint8_t address, uint8_t quantity, uint8_t send_stop);
uint8_t au_wire_request_from_register(uint8_t address, uint8_t quantity, uint32_t internal_address, uint8_t internal_address_size, uint8_t send_stop);
int au_wire_available();
int au_wire_read();
int au_wire_peek();
void au_wire_flush();
size_t au_wire_write(uint8_t data);
size_t au_wire_write_buffer(const uint8_t *data, size_t length);
size_t au_wire_print(void *context, uint8_t data);
void au_wire_on_receive(void (*callback)(int));
void au_wire_on_request(void (*callback)(void));

#define au_wire_build_printer() \
{ \
  .context = NULL, \
  .print = au_wire_print \
}

#endif