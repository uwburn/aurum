#include "aurum.h"

// OK

uint8_t au_shift_in(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder) {
	uint8_t value = 0;
	uint8_t i;

	for (i = 0; i < 8; ++i) {
		au_digital_write(clockPin, HIGH);
		if (bitOrder == LSBFIRST) {
			value |= au_digital_read(dataPin) << i;
    }
		else {
			value |= au_digital_read(dataPin) << (7 - i);
    }
		au_digital_write(clockPin, LOW);
	}
	return value;
}

void au_shift_out(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val) {
	uint8_t i;

	for (i = 0; i < 8; i++)  {
		if (bitOrder == LSBFIRST) {
			au_digital_write(dataPin, val & 1);
			val >>= 1;
		} else {	
			au_digital_write(dataPin, (val & 128) != 0);
			val <<= 1;
		}
			
		au_digital_write(clockPin, HIGH);
		au_digital_write(clockPin, LOW);		
	}
}