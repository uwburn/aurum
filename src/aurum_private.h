#ifndef AURUM_PRIVATE_H
#define AURUM_PRIVATE_H

#include <avr/pgmspace.h>

#include "aurum/core.h"

#ifndef F_CPU
# warning "F_CPU not defined for <aurum.h>"
# define F_CPU 16000000UL
#endif

#define au_clock_cycles_per_microsecond() ( F_CPU / 1000000L )
#define au_clock_cycles_to_microseconds(a) ( (a) / au_clock_cycles_per_microsecond() )
#define au_microseconds_to_clock_cycles(a) ( (a) * au_clock_cycles_per_microsecond() )

#define AU_NOT_A_PIN    0
#define AU_NOT_A_PORT   0
#define AU_NOT_ON_TIMER 0
#define AU_TIMER0A      1
#define AU_TIMER0B      2
#define AU_TIMER1A      3
#define AU_TIMER1B      4
#define AU_TIMER1C      5
#define AU_TIMER2       6
#define AU_TIMER2A      7
#define AU_TIMER2B      8
#define AU_PORT_B       2
#define AU_PORT_C       3
#define AU_PORT_D       4

extern const uint16_t PROGMEM au_port_to_mode_PGM[];
extern const uint16_t PROGMEM au_port_to_input_PGM[];
extern const uint16_t PROGMEM au_port_to_output_PGM[];
extern const uint8_t PROGMEM au_digital_pin_to_port_PGM[];
extern const uint8_t PROGMEM au_digital_pin_to_bit_mask_PGM[];
extern const uint8_t PROGMEM au_digital_pin_to_timer_PGM[];

#define au_digital_pin_to_port(P) ( pgm_read_byte( au_digital_pin_to_port_PGM + (P) ) )
#define au_digital_pin_to_bit_mask(P) ( pgm_read_byte( au_digital_pin_to_bit_mask_PGM + (P) ) )
#define au_digital_pin_to_timer(P) ( pgm_read_byte( au_digital_pin_to_timer_PGM + (P) ) )
#define au_analog_in_pin_to_bit(P) (P)
#define au_port_output_register(P) ( (volatile uint8_t *)( pgm_read_word( au_port_to_output_PGM + (P))) )
#define au_port_input_register(P) ( (volatile uint8_t *)( pgm_read_word( au_port_to_input_PGM + (P))) )
#define au_port_mode_register(P) ( (volatile uint8_t *)( pgm_read_word( au_port_to_mode_PGM + (P))) )

#define au_bit_read(value, bit) (((value) >> (bit)) & 0x01)
#define au_bit_set(value, bit) ((value) |= (1UL << (bit)))
#define au_bit_clear(value, bit) ((value) &= ~(1UL << (bit)))
#define au_bit_toggle(value, bit) ((value) ^= (1UL << (bit)))
#define au_bit_write(value, bit, bitvalue) ((bitvalue) ? au_bit_set(value, bit) : au_bit_clear(value, bit))

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

#define PIN_WIRE_SDA    (20)
#define PIN_WIRE_SCL    (21)

#define PIN_SPI_SS    (10)
#define PIN_SPI_MOSI  (11)
#define PIN_SPI_MISO  (12)
#define PIN_SPI_SCK   (13)

#endif