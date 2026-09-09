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


#ifndef AURUM_TONE_H
#define AURUM_TONE_H

/*
* TONE
*
* This header contains functions for driving a pizeo speaker from digital pins.
*
* Unlike the original Arduino API that directly includes this in the Arduino.h,
* thoose functions have been moved to a dedicated header.
*/

#include <stdint.h>

#include "core.h"

void au_tone(uint8_t pin, unsigned int frequency, unsigned long duration);
void au_no_tone(uint8_t pin);

#endif