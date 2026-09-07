#ifndef AURUM_TWI_H
#define AURUM_TWI_H

#include <stdint.h>
#include <stdbool.h>

#ifndef AU_TWI_FREQ
#define AU_TWI_FREQ 100000L
#endif

#ifndef AU_TWI_BUFFER_LENGTH
#define AU_TWI_BUFFER_LENGTH 32
#endif

#define AU_TWI_READY 0
#define AU_TWI_MRX   1
#define AU_TWI_MTX   2
#define AU_TWI_SRX   3
#define AU_TWI_STX   4

void au_twi_init();
void au_twi_disable();
void au_twi_set_address(uint8_t);
void twi_set_frequency(uint32_t);
uint8_t au_twi_read_from(uint8_t, uint8_t*, uint8_t, uint8_t);
uint8_t au_twi_write_to(uint8_t, uint8_t*, uint8_t, uint8_t, uint8_t);
uint8_t au_twi_transmit(const uint8_t*, uint8_t);
void au_twi_attach_slave_rx_event( void (*)(uint8_t*, int) );
void au_twi_attach_slave_tx_event( void (*)(void) );
void au_twi_reply(uint8_t);
void au_twi_stop();
void au_twi_release_bus();
void au_twi_set_timeout_in_micros(uint32_t, bool);
void au_twi_handle_timeout(bool);
bool au_twi_manage_timeout_flag(bool);

#endif