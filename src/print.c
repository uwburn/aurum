#include <math.h>

#include "aurum/print.h"

size_t au_println(au_printer_t *printer) {
  return au_print_str(printer, "\r\n");
}

size_t au_print_str(au_printer_t *printer, const char *s) {
  size_t n = 0;

  while (*s != '\0') {
    n += printer->print(printer->context, (uint8_t)*s);
    s++;
  }

  return n;
}

size_t au_println_str(au_printer_t *printer, const char *s) {
  size_t n = au_print_str(printer, s);
  n += au_println(printer);
  return n;
}

size_t au_print_uint_base(au_printer_t *printer, uint32_t value, uint8_t base) {
  char buf[8 * sizeof(uint32_t) + 1]; // Assumes 8-bit chars plus zero byte.
  char *str = &buf[sizeof(buf) - 1];

  *str = '\0';

  // prevent crash if called with base == 1
  if (base < 2) {
    base = 10;
  }

  do {
    char c = value % base;
    value /= base;

    *--str = c < 10 ? c + '0' : c + 'A' - 10;
  } while(value);

  return au_print_str(printer, str);
}

size_t au_println_uint_base(au_printer_t *printer, uint32_t value, uint8_t base) {
  size_t n = au_print_uint_base(printer, value, base);
  n += au_println(printer);
  return n;
}

size_t au_print_uint(au_printer_t *printer, uint32_t value) {
  return au_print_uint_base(printer, value, AU_DEC);
}

size_t au_println_uint(au_printer_t *printer, uint32_t value) {
  size_t n = au_print_uint(printer, value);
  n += au_println(printer);
  return n;
}

size_t au_print_int_base(au_printer_t *printer, int32_t value, uint8_t base) {
  if (value < 0) {
    size_t n = printer->print(printer->context, '-');
    n += au_print_uint_base(printer, (uint32_t)(-(value + 1)) + 1, base);
    return n;
  }
  else {
    return au_print_uint_base(printer, (uint32_t)value, base);
  }
}

size_t au_println_int_base(au_printer_t *printer, int32_t value, uint8_t base) {
  size_t n = au_print_int_base(printer, value, base);
  n += au_println(printer);
  return n;
}

size_t au_print_int(au_printer_t *printer, int32_t value) {
  return au_print_int_base(printer, value, AU_DEC);
}

size_t au_println_int(au_printer_t *printer, int32_t value) {
  size_t n = au_print_int(printer, value);
  n += au_println(printer);
  return n;
}

size_t au_print_ulong_base(au_printer_t *printer, uint64_t value, uint8_t base) {
  char buf[8 * sizeof(uint64_t) + 1]; // Assumes 8-bit chars plus zero byte.
  char *str = &buf[sizeof(buf) - 1];

  *str = '\0';

  // prevent crash if called with base == 1
  if (base < 2) {
    base = 10;
  }

  do {
    char c = value % base;
    value /= base;

    *--str = c < 10 ? c + '0' : c + 'A' - 10;
  } while(value);

  return au_print_str(printer, str);
}

size_t au_println_ulong_base(au_printer_t *printer, uint64_t value, uint8_t base) {
  size_t n = au_print_ulong_base(printer, value, base);
  n += au_println(printer);
  return n;
}

size_t au_print_ulong(au_printer_t *printer, uint64_t value) {
  return au_print_ulong_base(printer, value, AU_DEC);
}

size_t au_println_ulong(au_printer_t *printer, uint64_t value) {
  size_t n = au_print_ulong(printer, value);
  n += au_println(printer);
  return n;
}

size_t au_print_long_base(au_printer_t *printer, int64_t value, uint8_t base) {
  if (value < 0) {
    size_t n = printer->print(printer->context, '-');
    n += au_print_ulong_base(printer, (uint64_t)(-(value + 1)) + 1, base);
    return n;
  }
  else {
    return au_print_ulong_base(printer, (uint64_t)value, base);
  }
}

size_t au_println_long_base(au_printer_t *printer, int64_t value, uint8_t base) {
  size_t n = au_print_long_base(printer, value, base);
  n += au_println(printer);
  return n;
}

size_t au_print_long(au_printer_t *printer, int64_t value) {
  return au_print_long_base(printer, value, AU_DEC);
}

size_t au_println_long(au_printer_t *printer, int64_t value) {
  return au_println_long_base(printer, value, AU_DEC);
}

size_t au_print_float(au_printer_t *printer, float number, uint8_t digits) { 
  size_t n = 0;
  
  if (isnan(number)) {
    return au_print_str(printer, "nan");
  }
  if (isinf(number)) {
    return au_print_str(printer, "inf");
  }
  // constant determined empirically
  if (number > 4294967040.0) {
    return au_print_str(printer, "ovf");
  }  // constant determined empirically
  if (number < -4294967040.0) {
    return au_print_str(printer, "ovf");
  }  
  
  // Handle negative numbers
  if (number < 0.0) {
    n += printer->print(printer, '-');
    number = -number;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  float rounding = 0.5;
  for (uint8_t i = 0; i < digits; ++i) {
    rounding /= 10.0;
  }
  
  number += rounding;

  // Extract the integer part of the number and print it
  uint32_t int_part = (uint32_t)number;
  float remainder = number - (float)int_part;
  n += au_print_uint(printer, int_part);

  // Print the decimal point, but only if there are digits beyond
  if (digits > 0) {
    n += printer->print(printer, '.');
  }

  // Extract digits from the remainder one at a time
  while (digits-- > 0) {
    remainder *= 10.0;
    uint32_t toPrint = (uint32_t)(remainder);
    n += au_print_uint(printer, toPrint);
    remainder -= toPrint;
  }
  
  return n;
}

size_t au_println_float(au_printer_t *printer, float number, uint8_t digits) {
  size_t n = au_print_float(printer, number, digits);
  n += au_println(printer);
  return n;
}

size_t au_print_double(au_printer_t *printer, double number, uint8_t digits) { 
  size_t n = 0;
  
  if (isnan(number)) {
    return au_print_str(printer, "nan");
  }
  if (isinf(number)) {
    return au_print_str(printer, "inf");
  }
  // constant determined empirically
  if (number > 4294967040.0) {
    return au_print_str(printer, "ovf");
  }  // constant determined empirically
  if (number < -4294967040.0) {
    return au_print_str(printer, "ovf");
  }  
  
  // Handle negative numbers
  if (number < 0.0) {
    n += printer->print(printer, '-');
    number = -number;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  double rounding = 0.5;
  for (uint8_t i = 0; i < digits; ++i) {
    rounding /= 10.0;
  }
  
  number += rounding;

  // Extract the integer part of the number and print it
  uint32_t int_part = (uint32_t)number;
  double remainder = number - (double)int_part;
  n += au_print_uint(printer, int_part);

  // Print the decimal point, but only if there are digits beyond
  if (digits > 0) {
    n += printer->print(printer, '.');
  }

  // Extract digits from the remainder one at a time
  while (digits-- > 0) {
    remainder *= 10.0;
    uint32_t toPrint = (uint32_t)(remainder);
    n += au_print_uint(printer, toPrint);
    remainder -= toPrint;
  }
  
  return n;
}

size_t au_println_double(au_printer_t *printer, double number, uint8_t digits) {
  size_t n = au_print_double(printer, number, digits);
  n += au_println(printer);
  return n;
}