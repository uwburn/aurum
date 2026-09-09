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

#ifndef AURUM_PRINT_H
#define AURUM_PRINT_H

/*
* PRINT
*
* Struct and functions to deal with common printing needs without recurring to
* the more heavyweigth sprintf.
*
* This is a porting of the Print class from the original Arduino API.
*
* Interfaces (Serial, software serial, i2c/wire) implement functions or macros
* to build the printer struct correctly.
*
* The printer struct is left public to support static memory allocation without
* hacks.
*/

#include <stdint.h>
#include <stddef.h>

#define AU_BIN 2
#define AU_OCT 8
#define AU_DEC 10
#define AU_HEX 16

typedef size_t (*au_print_write_fn)(void *context, uint8_t c);

typedef struct {
  void *context;
  au_print_write_fn print;
} au_printer_t;

size_t au_println(au_printer_t *printer);

size_t au_print_str(au_printer_t *printer, const char *s);
size_t au_println_str(au_printer_t *printer, const char *s);

size_t au_print_uint_base(au_printer_t *printer, uint32_t value, uint8_t base);
size_t au_println_uint_base(au_printer_t *printer, uint32_t value, uint8_t base);
size_t au_print_uint(au_printer_t *printer, uint32_t value);
size_t au_println_uint(au_printer_t *printer, uint32_t value);

size_t au_print_int_base(au_printer_t *printer, int32_t value, uint8_t base);
size_t au_println_int_base(au_printer_t *printer, int32_t value, uint8_t base);
size_t au_print_int(au_printer_t *printer, int32_t value);
size_t au_println_int(au_printer_t *printer, int32_t value);

size_t au_print_ulong_base(au_printer_t *printer, uint64_t value, uint8_t base);
size_t au_println_ulong_base(au_printer_t *printer, uint64_t value, uint8_t base);
size_t au_print_ulong(au_printer_t *printer, uint64_t value);
size_t au_println_ulong(au_printer_t *printer, uint64_t value);

size_t au_print_long_base(au_printer_t *printer, int64_t value, uint8_t base);
size_t au_println_long_base(au_printer_t *printer, int64_t value, uint8_t base);
size_t au_print_long(au_printer_t *printer, int64_t value);
size_t au_println_long(au_printer_t *printer, int64_t value);

size_t au_print_float(au_printer_t *printer, float value, uint8_t digits);
size_t au_println_float(au_printer_t *printer, float value, uint8_t digits);

size_t au_print_double(au_printer_t *printer, double value, uint8_t digits);
size_t au_println_double(au_printer_t *printer, double value, uint8_t digits);

#endif