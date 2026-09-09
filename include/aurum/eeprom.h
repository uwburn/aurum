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


#ifndef AURUM_EEPROM_H
#define AURUM_EEPROM_H

/*
* EEPROM
*
* Functions to read and write on the integrated EEPROM. It matches the
* Arduino dedicated EEPROM library.
*
* ATmega328P EEPROM size: 1024 bytes, valid addresses: 0..1023
*/

#include <stdint.h>

#include "core.h"

#define AU_EEPROM_SIZE 1024U
#define AU_EEPROM_END  (AU_EEPROM_SIZE - 1U)

uint8_t au_eeprom_read(uint16_t address);
void au_eeprom_write(uint16_t address, uint8_t value);
void au_eeprom_update(uint16_t address, uint8_t value);
void au_eeprom_get(uint16_t address, void *data, uint16_t size);
void au_eeprom_put(uint16_t address, const void *data, uint16_t size);

#endif