#ifndef AURUM_PRIVATE_H
#define AURUM_PRIVATE_H

#include "aurum.h"

uint32_t au_count_pulse_asm(volatile uint8_t *port, uint8_t bit, uint8_t stateMask, unsigned long maxloops);

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define EXTERNAL_NUM_INTERRUPTS 2

#define EXTERNAL_INT_0 0
#define EXTERNAL_INT_1 1

typedef void (*voidFuncPtr)(void);

#endif