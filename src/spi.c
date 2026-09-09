#include <avr/io.h>
#include <avr/interrupt.h>

#include "aurum/spi.h"
#include "aurum_private.h"

static uint8_t au_spi_initialized;
static uint8_t au_spi_interrupt_mode; // 0=none, 1=mask, 2=global
static uint8_t au_spi_interrupt_mask; // which interrupts to mask
static uint8_t au_spi_interrupt_save; // temp storage, to restore state

// mapping of interrupt numbers to bits within SPI_AVR_EIMSK
#define SPI_INT0_MASK  (1<<INT0)
#define SPI_INT1_MASK  (1<<INT1)

void au_spi_using_interrupt(uint8_t interrupt_number) {
  uint8_t mask = 0;
  uint8_t sreg = SREG;

  // Protect from a scheduler and prevent transactionBegin
  cli();
  
  switch (interrupt_number) {
  case 0: 
    mask = SPI_INT0_MASK;
    break;
  case 1:
    mask = SPI_INT1_MASK;
    break;
  default:
    au_spi_interrupt_mode = 2;
    break;
  }

  au_spi_interrupt_mask |= mask;

  if (!au_spi_interrupt_mode) {
    au_spi_interrupt_mode = 1;
  }

  SREG = sreg;
}

void au_spi_not_using_interrupt(uint8_t interrupt_number) {
  // Once in mode 2 we can't go back to 0 without a proper reference count
  if (au_spi_interrupt_mode == 2) {
    return;
  }

  uint8_t mask = 0;
  uint8_t sreg = SREG;
  cli(); // Protect from a scheduler and prevent transactionBegin
  switch (interrupt_number) {
  case 0: 
    mask = SPI_INT0_MASK;
    break;
  case 1:
    mask = SPI_INT1_MASK;
    break;
  default:
    break;
    // this case can't be reached
  }
  
  au_spi_interrupt_mask &= ~mask;

  if (!au_spi_interrupt_mask) {
    au_spi_interrupt_mode = 0;
  }

  SREG = sreg;
}

au_spi_settings_t au_spi_settings(uint32_t clock, uint8_t bit_order, uint8_t data_mode) {
  // Clock settings are defined as follows. Note that this shows SPI2X
  // inverted, so the bits form increasing numbers. Also note that
  // fosc/64 appears twice
  // SPR1 SPR0 ~SPI2X Freq
  //   0    0     0   fosc/2
  //   0    0     1   fosc/4
  //   0    1     0   fosc/8
  //   0    1     1   fosc/16
  //   1    0     0   fosc/32
  //   1    0     1   fosc/64
  //   1    1     0   fosc/64
  //   1    1     1   fosc/128

  // We find the fastest clock that is less than or equal to the
  // given clock rate. The clock divider that results in clock_setting
  // is 2 ^^ (clock_div + 1). If nothing is slow enough, we'll use the
  // slowest (128 == 2 ^^ 7, so clock_div = 6).
  uint8_t clock_div;
  uint32_t clock_setting = F_CPU / 2;
  clock_div = 0;
  while (clock_div < 6 && clock < clock_setting) {
    clock_setting /= 2;
    clock_div++;
  }

  // Compensate for the duplicate fosc/64
  if (clock_div == 6)
  clock_div = 7;

  // Invert the SPI2X bit
  clock_div ^= 0x1;

  // Build the SPI settings
  au_spi_settings_t settings = {
    .spcr = _BV(SPE) | 
            _BV(MSTR) | 
            ((bit_order == AU_SPI_LSBFIRST) ? _BV(DORD) : 0) |
            (data_mode & AU_SPI_MODE_MASK) | 
            ((clock_div >> 1) & AU_SPI_CLOCK_MASK),
    .spsr = clock_div & AU_SPI_2XCLOCK_MASK
  };
  
  return settings;
}

void au_spi_begin() {
  uint8_t sreg = SREG;
  cli(); // Protect from a scheduler and prevent transactionBegin
  if (!au_spi_initialized) {
    // Set SS to high so a connected chip will be "deselected" by default
    uint8_t port = au_digital_pin_to_port(PIN_SPI_SS);
    uint8_t bit = au_digital_pin_to_bit_mask(PIN_SPI_SS);
    volatile uint8_t *reg = au_port_mode_register(port);

    // if the SS pin is not already configured as an output
    // then set it high (to enable the internal pull-up resistor)
    if(!(*reg & bit)){
      au_digital_write(PIN_SPI_SS, AU_HIGH);
    }

    // When the SS pin is set as OUTPUT, it can be used as
    // a general purpose output port (it doesn't influence
    // SPI operations).
    au_pin_mode(PIN_SPI_SS, AU_OUTPUT);

    // Warning: if the SS pin ever becomes a LOW INPUT then SPI
    // automatically switches to Slave, so the data direction of
    // the SS pin MUST be kept as OUTPUT.
    SPCR |= _BV(MSTR);
    SPCR |= _BV(SPE);

    // Set direction register for SCK and MOSI pin.
    // MISO pin automatically overrides to INPUT.
    // By doing this AFTER enabling SPI, we avoid accidentally
    // clocking in a single bit since the lines go directly
    // from "input" to SPI control.
    // http://code.google.com/p/arduino/issues/detail?id=888
    au_pin_mode(PIN_SPI_SCK, AU_OUTPUT);
    au_pin_mode(PIN_SPI_MOSI, AU_OUTPUT);
  }
  au_spi_initialized++; // reference count
  SREG = sreg;
}

void au_spi_end() {
  uint8_t sreg = SREG;
  cli(); // Protect from a scheduler and prevent transactionBegin
  // Decrease the reference counter
  if (au_spi_initialized)
    au_spi_initialized--;
  // If there are no more references disable SPI
  if (!au_spi_initialized) {
    SPCR &= ~_BV(SPE);
    au_spi_interrupt_mode = 0;
  }
  SREG = sreg;
}

void au_spi_begin_transaction(au_spi_settings_t settings) {
  if (au_spi_interrupt_mode > 0) {
    uint8_t sreg = SREG;
    cli();

    au_spi_interrupt_save = sreg;
  }

  SPCR = settings.spcr;
  SPSR = settings.spsr;
}

void au_spi_end_transaction() {
  if (au_spi_interrupt_mode > 0) {
    cli();
    SREG = au_spi_interrupt_save;
  }
}

uint8_t au_spi_transfer(uint8_t data) {
  SPDR = data;
  /*
    * The following NOP introduces a small delay that can prevent the wait
    * loop form iterating when running at the maximum speed. This gives
    * about 10% more speed, even if it seems counter-intuitive. At lower
    * speeds it is unnoticed.
    */
  asm volatile("nop");
  while (!(SPSR & _BV(SPIF))); // wait
  return SPDR;
}

uint16_t au_spi_transfer16(uint16_t data) {
  union { 
    uint16_t val; 
    struct { 
      uint8_t lsb;
      uint8_t msb;
    }; 
  } in, out;

  in.val = data;
  if (!(SPCR & _BV(DORD))) {
    SPDR = in.msb;
    asm volatile("nop"); // See transfer(uint8_t) function
    while (!(SPSR & _BV(SPIF)));
    out.msb = SPDR;
    SPDR = in.lsb;
    asm volatile("nop");
    while (!(SPSR & _BV(SPIF)));
    out.lsb = SPDR;
  } else {
    SPDR = in.lsb;
    asm volatile("nop");
    while (!(SPSR & _BV(SPIF)));
    out.lsb = SPDR;
    SPDR = in.msb;
    asm volatile("nop");
    while (!(SPSR & _BV(SPIF)));
    out.msb = SPDR;
  }
  return out.val;
}

void au_spi_transfer_buffer(void *buffer, size_t count) {
  if (count == 0) {
    return;
  }

  uint8_t *p = (uint8_t *)buffer;
  SPDR = *p;
  while (--count > 0) {
    uint8_t out = *(p + 1);
    while (!(SPSR & _BV(SPIF)));
    uint8_t in = SPDR;
    SPDR = out;
    *p++ = in;
  }
  while (!(SPSR & _BV(SPIF)));
  *p = SPDR;
}