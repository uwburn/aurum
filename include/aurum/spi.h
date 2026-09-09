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

#ifndef AU_SPI_H
#define AU_SPI_H

/*
* SPI
*
* This header contains functions for using the SPI interface, it matches the
* dedicated Arduino SPI library.
*
* The spi_settings struct is left public to support static memory allocation
* without hacks.
*/

#include <stdint.h>
#include <stddef.h>

#include "core.h"

#define AU_SPI_LSBFIRST 0
#define AU_SPI_MSBFIRST 1

#define AU_SPI_CLOCK_DIV4   0x00
#define AU_SPI_CLOCK_DIV16  0x01
#define AU_SPI_CLOCK_DIV64  0x02
#define AU_SPI_CLOCK_DIV128 0x03
#define AU_SPI_CLOCK_DIV2   0x04
#define AU_SPI_CLOCK_DIV8   0x05
#define AU_SPI_CLOCK_DIV32  0x06

#define AU_SPI_MODE0 0x00
#define AU_SPI_MODE1 0x04
#define AU_SPI_MODE2 0x08
#define AU_SPI_MODE3 0x0C

#define AU_SPI_MODE_MASK     0x0C
#define AU_SPI_CLOCK_MASK    0x03
#define AU_SPI_2XCLOCK_MASK  0x01

typedef struct {
  uint8_t spcr;
  uint8_t spsr;
} au_spi_settings_t;

void au_spi_using_interrupt(uint8_t interrupt_number);
void au_spi_not_using_interrupt(uint8_t interrupt_number);
au_spi_settings_t au_spi_settings(uint32_t clock, uint8_t bit_order, uint8_t data_mode);
void au_spi_begin();
void au_spi_end();
void au_spi_begin_transaction(au_spi_settings_t settings);
void au_spi_end_transaction();
uint8_t au_spi_transfer(uint8_t data);
uint16_t au_spi_transfer16(uint16_t data);
void au_spi_transfer_buffer(void *buffer, size_t count);

#endif