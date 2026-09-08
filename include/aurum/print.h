#ifndef AURUM_PRINT_H
#define AURUM_PRINT_H

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