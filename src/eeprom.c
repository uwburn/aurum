#include "aurum.h"

#include <avr/io.h>

uint8_t au_eeprom_read(uint16_t address) {
  // Wait for completion of previous write
  while (EECR & (1 << EEPE));

  // Set EEPROM address
  EEAR = address;

  // Start EEPROM read
  EECR |= (1 << EERE);

  // Return data
  return EEDR;
}

void au_eeprom_write(uint16_t address, uint8_t value) {
  // Wait for completion of previous write
  while (EECR & (1 << EEPE));

  // Set EEPROM address
  EEAR = address;

  // Set data
  EEDR = value;

  // The EEPROM write sequence must be performed within
  // four clock cycles:
  // - Set EEMPE
  // - Set EEPE
  EECR |= (1 << EEMPE);
  EECR |= (1 << EEPE);
}

void au_eeprom_update(uint16_t address, uint8_t value) {
  if (au_eeprom_read(address) != value) {
    au_eeprom_write(address, value);
  }
}

void au_eeprom_put(uint16_t address, const void *data, uint16_t size) {
  const uint8_t *bytes = (const uint8_t *)data;

  for (uint16_t i = 0; i < size; i++) {
    au_eeprom_update(address + i, bytes[i]);
  }
}

void au_eeprom_get(uint16_t address, void *data, uint16_t size){
  uint8_t *bytes = (uint8_t *)data;

  for (uint16_t i = 0; i < size; i++) {
    bytes[i] = au_eeprom_read(address + i);
  }
}