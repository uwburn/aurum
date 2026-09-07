#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "aurum/wire.h"
#include "twi.h"

static uint8_t rx_buffer[AU_TWI_BUFFER_LENGTH];
static uint8_t rx_buffer_index = 0;
static uint8_t rx_buffer_length = 0;

static uint8_t tx_address = 0;
static uint8_t tx_buffer[AU_TWI_BUFFER_LENGTH];
static uint8_t tx_buffer_index = 0;
static uint8_t tx_buffer_length = 0;

static uint8_t transmitting = 0;

static void (*user_on_request)(void) = NULL;
static void (*user_on_receive)(int) = NULL;

static void au_wire_on_request_service(void) {
  if (user_on_request == NULL) {
      return;
  }

  // Reset TX buffer iterator variables.
  // This matches the behaviour of the original Arduino implementation.
  tx_buffer_index = 0;
  tx_buffer_length = 0;

  user_on_request();
}

static void au_wire_on_receive_service(uint8_t *in_bytes, int num_bytes) {
  // Don't bother if the user hasn't registered a callback.
  if (user_on_receive == NULL) {
    return;
  }

  // Don't overwrite data that is still waiting to be consumed.
  if (rx_buffer_index < rx_buffer_length) {
    return;
  }

  // Copy the TWI RX buffer into the Wire RX buffer.
  for (int i = 0; i < num_bytes; ++i) {
    rx_buffer[i] = in_bytes[i];
  }

  rx_buffer_index = 0;
  rx_buffer_length = (uint8_t)num_bytes;

  // Notify the application.
  user_on_receive(num_bytes);
}

void au_wire_begin(void) {
  rx_buffer_index = 0;
  rx_buffer_length = 0;

  tx_buffer_index = 0;
  tx_buffer_length = 0;

  transmitting = 0;

  au_twi_init();

  // Default callbacks must always be installed because the TWI
  // implementation can operate as a slave.
  au_twi_attach_slave_tx_event(au_wire_on_request_service);
  au_twi_attach_slave_rx_event(au_wire_on_receive_service);
}

void au_wire_begin_address(uint8_t address) {
  au_wire_begin();

  au_twi_set_address(address);
}

void au_wire_end() {
  au_twi_disable();
}

void au_wire_set_clock(uint32_t clock) {
  twi_set_frequency(clock);
}

void au_wire_set_timeout(uint32_t timeout_us, uint8_t reset_with_timeout) {
  au_twi_set_timeout_in_micros(timeout_us, reset_with_timeout != 0);
}

uint8_t au_wire_get_timeout_flag() {
  return au_twi_manage_timeout_flag(false) ? 1 : 0;
}

void au_wire_clear_timeout_flag() {
  au_twi_manage_timeout_flag(true);
}

uint8_t au_wire_request_from_register(uint8_t address, uint8_t quantity, uint32_t internal_address, uint8_t internal_address_size, uint8_t send_stop) {
  // Send internal register address first.
  // This allows access to devices with internal registers while
  // preserving the repeated START between the write and read.
  if (internal_address_size > 0) {
    au_wire_begin_transmission(address);

    // Maximum internal address size is 3 bytes.
    if (internal_address_size > 3) {
      internal_address_size = 3;
    }

    // Send most significant byte first.
    while (internal_address_size-- > 0) {
      au_wire_write((uint8_t)(internal_address >> (internal_address_size * 8)));
    }

    // No STOP: the following read starts with a repeated START.
    au_wire_end_transmission_stop(0);
  }

  // Clamp quantity to the RX buffer size.
  if (quantity > AU_TWI_BUFFER_LENGTH) {
    quantity = AU_TWI_BUFFER_LENGTH;
  }

  // Perform blocking read.
  uint8_t read = au_twi_read_from(address, rx_buffer, quantity, send_stop);

  rx_buffer_index = 0;
  rx_buffer_length = read;

  return read;
}

uint8_t au_wire_request_from(uint8_t address, uint8_t quantity) {
  return au_wire_request_from_stop(address, quantity, 1);
}

uint8_t au_wire_request_from_stop(uint8_t address, uint8_t quantity, uint8_t send_stop) {
  return au_wire_request_from_register(address, quantity, 0, 0, send_stop);
}

void au_wire_begin_transmission(uint8_t address) {
  // Indicate that we are transmitting.
  transmitting = 1;

  // Set address of targeted slave.
  tx_address = address;

  // Reset TX buffer iterator variables.
  tx_buffer_index = 0;
  tx_buffer_length = 0;
}

uint8_t au_wire_end_transmission(void) {
  return au_wire_end_transmission_stop(1);
}

uint8_t au_wire_end_transmission_stop(uint8_t send_stop) {
  // Transmit buffer (blocking).
  uint8_t ret = au_twi_write_to( tx_address, tx_buffer, tx_buffer_length, 1, send_stop);

  // Reset TX buffer iterator variables.
  tx_buffer_index = 0;
  tx_buffer_length = 0;

  // Indicate that we are done transmitting.
  transmitting = 0;

  return ret;
}

size_t au_wire_write(uint8_t data) {
  if (transmitting) {
    // Master transmitter mode.
    // Don't bother if buffer is full.
    if (tx_buffer_length >= AU_TWI_BUFFER_LENGTH) {
      return 0;
    }

    // Put byte in TX buffer.
    tx_buffer[tx_buffer_index] = data;
    ++tx_buffer_index;

    // Update amount in buffer.
    tx_buffer_length = tx_buffer_index;
  }
  else {
    // Slave transmitter mode.
    // Reply to master.
    au_twi_transmit(&data, 1);
  }

  return 1;
}

size_t au_wire_write_buffer(const uint8_t *data, size_t quantity) {
  if (transmitting) {
    // Master transmitter mode.
    size_t written = 0;

    for (size_t i = 0; i < quantity; ++i) {
      if (au_wire_write(data[i]) == 0) {
        break;
      }

      ++written;
    }

    return written;
  }
  else {
    // Slave transmitter mode.
    au_twi_transmit(data, quantity);

    return quantity;
  }
}

// RX buffer
int au_wire_available(void) {
  return rx_buffer_length - rx_buffer_index;
}

int au_wire_read(void) {
  int value = -1;

  // Get each successive byte on each call.
  if (rx_buffer_index < rx_buffer_length) {
    value = rx_buffer[rx_buffer_index];
    ++rx_buffer_index;
  }

  return value;
}

int au_wire_peek(void) {
  int value = -1;

  if (rx_buffer_index < rx_buffer_length) {
    value = rx_buffer[rx_buffer_index];
  }

  return value;
}

void au_wire_flush(void) {
}

void au_wire_on_receive(void (*callback)(int)) {
  user_on_receive = callback;
}

void au_wire_on_request(void (*callback)(void)) {
  user_on_request = callback;
}