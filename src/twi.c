#include <math.h>
#include <stdlib.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <compat/twi.h>

#include "aurum_private.h"

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif

#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#include "twi.h"

static volatile uint8_t twi_state;
static volatile uint8_t twi_slarw;
static volatile uint8_t twi_send_stop;			// should the transaction end with a stop
static volatile uint8_t twi_in_rep_start;		// in the middle of a repeated start

// twi_timeout_us > 0 prevents the code from getting stuck in various while loops here
// if twi_timeout_us == 0 then timeout checking is disabled (the previous Wire lib behavior)
// at some point in the future, the default twi_timeout_us value could become 25000
// and twi_do_reset_on_timeout could become true
// to conform to the SMBus standard
// http://smbus.org/specs/SMBus_3_1_20180319.pdf
static volatile uint32_t twi_timeout_us = 0ul;
static volatile bool twi_timed_out_flag = false;  // a timeout has been seen
static volatile bool twi_do_reset_on_timeout = false;  // reset the TWI registers on timeout

static void (*twi_on_slave_transmit)(void);
static void (*twi_on_slave_receive)(uint8_t*, int);

static uint8_t twi_master_buffer[AU_TWI_BUFFER_LENGTH];
static volatile uint8_t twi_master_buffer_index;
static volatile uint8_t twi_master_buffer_length;

static uint8_t twi_txBuffer[AU_TWI_BUFFER_LENGTH];
static volatile uint8_t twi_tx_buffer_index;
static volatile uint8_t twi_tx_buffer_length;

static uint8_t twi_rx_buffer[AU_TWI_BUFFER_LENGTH];
static volatile uint8_t twi_rx_buffer_index;

static volatile uint8_t twi_error;

/* 
 * Function au_twi_init
 * Desc     readys twi pins and sets twi bitrate
 * Input    none
 * Output   none
 */
void au_twi_init(void) {
  // initialize state
  twi_state = AU_TWI_READY;
  twi_send_stop = true;		// default value
  twi_in_rep_start = false;
  
  // activate internal pullups for twi.
  au_digital_write(PIN_WIRE_SDA, 1);
  au_digital_write(PIN_WIRE_SCL, 1);

  // initialize twi prescaler and bit rate
  cbi(TWSR, TWPS0);
  cbi(TWSR, TWPS1);
  TWBR = ((F_CPU / AU_TWI_FREQ) - 16) / 2;

  /* twi bit rate formula from atmega128 manual pg 204
  SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR))
  note: TWBR should be 10 or higher for master mode
  It is 72 for a 16mhz Wiring board with 100kHz TWI */

  // enable twi module, acks, and twi interrupt
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
}

/* 
 * Function au_twi_disable
 * Desc     disables twi pins
 * Input    none
 * Output   none
 */
void au_twi_disable(void) {
  // disable twi module, acks, and twi interrupt
  TWCR &= ~(_BV(TWEN) | _BV(TWIE) | _BV(TWEA));

  // deactivate internal pullups for twi.
  au_digital_write(PIN_WIRE_SDA, 0);
  au_digital_write(PIN_WIRE_SCL, 0);
}

/* 
 * Function twi_slaveInit
 * Desc     sets slave address and enables interrupt
 * Input    none
 * Output   none
 */
void au_twi_set_address(uint8_t address) {
  // set twi slave address (skip over TWGCE bit)
  TWAR = address << 1;
}

/* 
 * Function twi_setClock
 * Desc     sets twi bit rate
 * Input    Clock Frequency
 * Output   none
 */
void twi_set_frequency(uint32_t frequency) {
  TWBR = ((F_CPU / frequency) - 16) / 2;
  
  /* twi bit rate formula from atmega128 manual pg 204
  SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR))
  note: TWBR should be 10 or higher for master mode
  It is 72 for a 16mhz Wiring board with 100kHz TWI */
}

/* 
 * Function au_twi_read_from
 * Desc     attempts to become twi bus master and read a
 *          series of bytes from a device on the bus
 * Input    address: 7bit i2c device address
 *          data: pointer to byte array
 *          length: number of bytes to read into array
 *          sendStop: Boolean indicating whether to send a stop at the end
 * Output   number of bytes read
 */
uint8_t au_twi_read_from(uint8_t address, uint8_t* data, uint8_t length, uint8_t sendStop) {
  uint8_t i;

  // ensure data will fit into buffer
  if(AU_TWI_BUFFER_LENGTH < length){
    return 0;
  }

  // wait until twi is ready, become master receiver
  uint32_t start_micros = au_micros();
  while(AU_TWI_READY != twi_state) {
    if((twi_timeout_us > 0ul) && ((au_micros() - start_micros) > twi_timeout_us)) {
      au_twi_handle_timeout(twi_do_reset_on_timeout);
      return 0;
    }
  }
  twi_state = AU_TWI_MRX;
  twi_send_stop = sendStop;
  // reset error state (0xFF.. no error occurred)
  twi_error = 0xFF;

  // initialize buffer iteration vars
  twi_master_buffer_index = 0;
  twi_master_buffer_length = length-1;  // This is not intuitive, read on...
  // On receive, the previously configured ACK/NACK setting is transmitted in
  // response to the received byte before the interrupt is signalled. 
  // Therefore we must actually set NACK when the _next_ to last byte is
  // received, causing that NACK to be sent in response to receiving the last
  // expected byte of data.

  // build sla+w, slave device address + w bit
  twi_slarw = TW_READ;
  twi_slarw |= address << 1;

  if (true == twi_in_rep_start) {
    // if we're in the repeated start state, then we've already sent the start,
    // (@@@ we hope), and the TWI statemachine is just waiting for the address byte.
    // We need to remove ourselves from the repeated start state before we enable interrupts,
    // since the ISR is ASYNC, and we could get confused if we hit the ISR before cleaning
    // up. Also, don't enable the START interrupt. There may be one pending from the 
    // repeated start that we sent ourselves, and that would really confuse things.
    twi_in_rep_start = false;			// remember, we're dealing with an ASYNC ISR
    start_micros = au_micros();
    do {
      TWDR = twi_slarw;
      if((twi_timeout_us > 0ul) && ((au_micros() - start_micros) > twi_timeout_us)) {
        au_twi_handle_timeout(twi_do_reset_on_timeout);
        return 0;
      }
    } 
    while(TWCR & _BV(TWWC));
    TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE);	// enable INTs, but not START
  }
  else {
    // send start condition
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTA);
  }

  // wait for read operation to complete
  start_micros = au_micros();
  while(AU_TWI_MRX == twi_state){
    if((twi_timeout_us > 0ul) && ((au_micros() - start_micros) > twi_timeout_us)) {
      au_twi_handle_timeout(twi_do_reset_on_timeout);
      return 0;
    }
  }

  if (twi_master_buffer_index < length) {
    length = twi_master_buffer_index;
  }

  // copy twi buffer to data
  for(i = 0; i < length; ++i){
    data[i] = twi_master_buffer[i];
  }

  return length;
}

/* 
 * Function au_twi_write_to
 * Desc     attempts to become twi bus master and write a
 *          series of bytes to a device on the bus
 * Input    address: 7bit i2c device address
 *          data: pointer to byte array
 *          length: number of bytes in array
 *          wait: boolean indicating to wait for write or not
 *          sendStop: boolean indicating whether or not to send a stop at the end
 * Output   0 .. success
 *          1 .. length to long for buffer
 *          2 .. address send, NACK received
 *          3 .. data send, NACK received
 *          4 .. other twi error (lost bus arbitration, bus error, ..)
 *          5 .. timeout
 */
uint8_t au_twi_write_to(uint8_t address, uint8_t* data, uint8_t length, uint8_t wait, uint8_t sendStop) {
  uint8_t i;

  // ensure data will fit into buffer
  if(AU_TWI_BUFFER_LENGTH < length){
    return 1;
  }

  // wait until twi is ready, become master transmitter
  uint32_t start_micros = au_micros();
  while(AU_TWI_READY != twi_state) {
    if((twi_timeout_us > 0ul) && ((au_micros() - start_micros) > twi_timeout_us)) {
      au_twi_handle_timeout(twi_do_reset_on_timeout);
      return (5);
    }
  }
  twi_state = AU_TWI_MTX;
  twi_send_stop = sendStop;
  // reset error state (0xFF.. no error occurred)
  twi_error = 0xFF;

  // initialize buffer iteration vars
  twi_master_buffer_index = 0;
  twi_master_buffer_length = length;
  
  // copy data to twi buffer
  for(i = 0; i < length; ++i) {
    twi_master_buffer[i] = data[i];
  }
  
  // build sla+w, slave device address + w bit
  twi_slarw = TW_WRITE;
  twi_slarw |= address << 1;
  
  // if we're in a repeated start, then we've already sent the START
  // in the ISR. Don't do it again.
  //
  if (true == twi_in_rep_start) {
    // if we're in the repeated start state, then we've already sent the start,
    // (@@@ we hope), and the TWI statemachine is just waiting for the address byte.
    // We need to remove ourselves from the repeated start state before we enable interrupts,
    // since the ISR is ASYNC, and we could get confused if we hit the ISR before cleaning
    // up. Also, don't enable the START interrupt. There may be one pending from the 
    // repeated start that we sent ourselves, and that would really confuse things.
    twi_in_rep_start = false;			// remember, we're dealing with an ASYNC ISR
    start_micros = au_micros();
    do {
      TWDR = twi_slarw;
      if((twi_timeout_us > 0ul) && ((au_micros() - start_micros) > twi_timeout_us)) {
        au_twi_handle_timeout(twi_do_reset_on_timeout);
        return (5);
      }
    } while(TWCR & _BV(TWWC));
    TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE);	// enable INTs, but not START
  }
  else {
    // send start condition
    TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTA);	// enable INTs
  }

  // wait for write operation to complete
  start_micros = au_micros();
  while(wait && (AU_TWI_MTX == twi_state)) {
    if((twi_timeout_us > 0ul) && ((au_micros() - start_micros) > twi_timeout_us)) {
      au_twi_handle_timeout(twi_do_reset_on_timeout);
      return (5);
    }
  }
  
  if (twi_error == 0xFF) {
    return 0;	// success
  }
  else if (twi_error == TW_MT_SLA_NACK) {
    return 2;	// error: address send, nack received
  }
  else if (twi_error == TW_MT_DATA_NACK) {
    return 3;	// error: data send, nack received
  }
  else {
    return 4;	// other twi error
  }
}

/* 
 * Function au_twi_transmit
 * Desc     fills slave tx buffer with data
 *          must be called in slave tx event callback
 * Input    data: pointer to byte array
 *          length: number of bytes in array
 * Output   1 length too long for buffer
 *          2 not slave transmitter
 *          0 ok
 */
uint8_t au_twi_transmit(const uint8_t* data, uint8_t length) {
  uint8_t i;

  // ensure data will fit into buffer
  if(AU_TWI_BUFFER_LENGTH < (twi_tx_buffer_length+length)) {
    return 1;
  }
  
  // ensure we are currently a slave transmitter
  if(AU_TWI_STX != twi_state) {
    return 2;
  }
  
  // set length and copy data into tx buffer
  for(i = 0; i < length; ++i) {
    twi_txBuffer[twi_tx_buffer_length+i] = data[i];
  }
  twi_tx_buffer_length += length;
  
  return 0;
}

/* 
 * Function au_twi_attach_slave_rx_event
 * Desc     sets function called before a slave read operation
 * Input    function: callback function to use
 * Output   none
 */
void au_twi_attach_slave_rx_event(void (*function)(uint8_t*, int)) {
  twi_on_slave_receive = function;
}

/* 
 * Function au_twi_attach_slave_tx_event
 * Desc     sets function called before a slave write operation
 * Input    function: callback function to use
 * Output   none
 */
void au_twi_attach_slave_tx_event(void (*function)(void)) {
  twi_on_slave_transmit = function;
}

/* 
 * Function au_twi_reply
 * Desc     sends byte or readys receive line
 * Input    ack: byte indicating to ack or to nack
 * Output   none
 */
void au_twi_reply(uint8_t ack) {
  // transmit master read ready signal, with or without ack
  if(ack) {
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);
  }
  else {
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT);
  }
}

/* 
 * Function au_twi_stop
 * Desc     relinquishes bus master status
 * Input    none
 * Output   none
 */
void au_twi_stop(void) {
  // send stop condition
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTO);

  // wait for stop condition to be executed on bus
  // TWINT is not set after a stop condition!
  // We cannot use au_micros() from an ISR, so approximate the timeout with cycle-counted delays
  const uint8_t us_per_loop = 8;
  uint32_t counter = (twi_timeout_us + us_per_loop - 1)/us_per_loop; // Round up
  while(TWCR & _BV(TWSTO)) {
    if(twi_timeout_us > 0ul){
      if (counter > 0ul){
        _delay_us(us_per_loop);
        counter--;
      }
      else {
        au_twi_handle_timeout(twi_do_reset_on_timeout);
        return;
      }
    }
  }

  // update twi state
  twi_state = AU_TWI_READY;
}

/* 
 * Function au_twi_release_bus
 * Desc     releases bus control
 * Input    none
 * Output   none
 */
void au_twi_release_bus(void) {
  // release bus
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT);

  // update twi state
  twi_state = AU_TWI_READY;
}

/* 
 * Function au_twi_set_timeout_in_micros
 * Desc     set a timeout for while loops that twi might get stuck in
 * Input    timeout value in microseconds (0 means never time out)
 * Input    reset_with_timeout: true causes timeout events to reset twi
 * Output   none
 */
void au_twi_set_timeout_in_micros(uint32_t timeout, bool reset_with_timeout) {
  twi_timed_out_flag = false;
  twi_timeout_us = timeout;
  twi_do_reset_on_timeout = reset_with_timeout;
}

/* 
 * Function twi_handleTimeout
 * Desc     this gets called whenever a while loop here has lasted longer than
 *          twi_timeout_us microseconds. always sets twi_timed_out_flag
 * Input    reset: true causes this function to reset the twi hardware interface
 * Output   none
 */
void au_twi_handle_timeout(bool reset) {
  twi_timed_out_flag = true;

  if (reset) {
    // remember bitrate and address settings
    uint8_t previous_TWBR = TWBR;
    uint8_t previous_TWAR = TWAR;

    // reset the interface
    au_twi_disable();
    au_twi_init();

    // reapply the previous register values
    TWAR = previous_TWAR;
    TWBR = previous_TWBR;
  }
}

/*
 * Function au_twi_manage_timeout_flag
 * Desc     returns true if twi has seen a timeout
 *          optionally clears the timeout flag
 * Input    clear_flag: true if we should reset the hardware
 * Output   the value of twi_timed_out_flag when the function was called
 */
bool au_twi_manage_timeout_flag(bool clear_flag) {
  bool flag = twi_timed_out_flag;
  if (clear_flag) {
    twi_timed_out_flag = false;
  }
  return(flag);
}

ISR(TWI_vect) {
  switch(TW_STATUS) {
  // All Master
  case TW_START:     // sent start condition
  case TW_REP_START: // sent repeated start condition
    // copy device address and r/w bit to output register and ack
    TWDR = twi_slarw;
    au_twi_reply(1);
    break;
  // Master Transmitter
  case TW_MT_SLA_ACK:  // slave receiver acked address
  case TW_MT_DATA_ACK: // slave receiver acked data
    // if there is data to send, send it, otherwise stop 
    if(twi_master_buffer_index < twi_master_buffer_length){
      // copy data to output register and ack
      TWDR = twi_master_buffer[twi_master_buffer_index++];
      au_twi_reply(1);
    }
    else{
      if (twi_send_stop){
        au_twi_stop();
      }
      else {
        twi_in_rep_start = true;	// we're gonna send the START
        // don't enable the interrupt. We'll generate the start, but we
        // avoid handling the interrupt until we're in the next transaction,
        // at the point where we would normally issue the start.
        TWCR = _BV(TWINT) | _BV(TWSTA)| _BV(TWEN) ;
        twi_state = AU_TWI_READY;
      }
    }
    break;
  case TW_MT_SLA_NACK:  // address sent, nack received
    twi_error = TW_MT_SLA_NACK;
    au_twi_stop();
    break;
  case TW_MT_DATA_NACK: // data sent, nack received
    twi_error = TW_MT_DATA_NACK;
    au_twi_stop();
    break;
  case TW_MT_ARB_LOST: // lost bus arbitration
    twi_error = TW_MT_ARB_LOST;
    au_twi_release_bus();
    break;
  // Master Receiver
  case TW_MR_DATA_ACK: // data received, ack sent
    // put byte into buffer
    twi_master_buffer[twi_master_buffer_index++] = TWDR;
    #if __GNUC__ >= 7
    __attribute__ ((fallthrough));
    #endif
  case TW_MR_SLA_ACK:  // address sent, ack received
    // ack if more bytes are expected, otherwise nack
    if(twi_master_buffer_index < twi_master_buffer_length){
      au_twi_reply(1);
    }
    else{
      au_twi_reply(0);
    }
    break;
  case TW_MR_DATA_NACK: // data received, nack sent
    // put final byte into buffer
    twi_master_buffer[twi_master_buffer_index++] = TWDR;
    if (twi_send_stop){
      au_twi_stop();
    }
    else {
      twi_in_rep_start = true;	// we're gonna send the START
      // don't enable the interrupt. We'll generate the start, but we
      // avoid handling the interrupt until we're in the next transaction,
      // at the point where we would normally issue the start.
      TWCR = _BV(TWINT) | _BV(TWSTA)| _BV(TWEN) ;
      twi_state = AU_TWI_READY;
    }
    break;
  case TW_MR_SLA_NACK: // address sent, nack received
    au_twi_stop();
    break;
  // TW_MR_ARB_LOST handled by TW_MT_ARB_LOST case

  // Slave Receiver
  case TW_SR_SLA_ACK:   // addressed, returned ack
  case TW_SR_GCALL_ACK: // addressed generally, returned ack
  case TW_SR_ARB_LOST_SLA_ACK:   // lost arbitration, returned ack
  case TW_SR_ARB_LOST_GCALL_ACK: // lost arbitration, returned ack
    // enter slave receiver mode
    twi_state = AU_TWI_SRX;
    // indicate that rx buffer can be overwritten and ack
    twi_rx_buffer_index = 0;
    au_twi_reply(1);
    break;
  case TW_SR_DATA_ACK:       // data received, returned ack
  case TW_SR_GCALL_DATA_ACK: // data received generally, returned ack
    // if there is still room in the rx buffer
    if(twi_rx_buffer_index < AU_TWI_BUFFER_LENGTH) {
      // put byte in buffer and ack
      twi_rx_buffer[twi_rx_buffer_index++] = TWDR;
      au_twi_reply(1);
    }
    else{
      // otherwise nack
      au_twi_reply(0);
    }
    break;
  case TW_SR_STOP: // stop or repeated start condition received
    // ack future responses and leave slave receiver state
    au_twi_release_bus();
    // put a null char after data if there's room
    if(twi_rx_buffer_index < AU_TWI_BUFFER_LENGTH) {
      twi_rx_buffer[twi_rx_buffer_index] = '\0';
    }
    // callback to user defined callback
    twi_on_slave_receive(twi_rx_buffer, twi_rx_buffer_index);
    // since we submit rx buffer to "wire" library, we can reset it
    twi_rx_buffer_index = 0;
    break;
  case TW_SR_DATA_NACK:       // data received, returned nack
  case TW_SR_GCALL_DATA_NACK: // data received generally, returned nack
    // nack back at master
    au_twi_reply(0);
    break;
  
  // Slave Transmitter
  case TW_ST_SLA_ACK:          // addressed, returned ack
  case TW_ST_ARB_LOST_SLA_ACK: // arbitration lost, returned ack
    // enter slave transmitter mode
    twi_state = AU_TWI_STX;
    // ready the tx buffer index for iteration
    twi_tx_buffer_index = 0;
    // set tx buffer length to be zero, to verify if user changes it
    twi_tx_buffer_length = 0;
    // request for txBuffer to be filled and length to be set
    // note: user must call au_twi_transmit(bytes, length) to do this
    twi_on_slave_transmit();
    // if they didn't change buffer & length, initialize it
    if(0 == twi_tx_buffer_length) {
      twi_tx_buffer_length = 1;
      twi_txBuffer[0] = 0x00;
    }
    #if __GNUC__ >= 7
    __attribute__ ((fallthrough));		  
    #endif
    // transmit first byte from buffer, fall
  case TW_ST_DATA_ACK: // byte sent, ack returned
    // copy data to output register
    TWDR = twi_txBuffer[twi_tx_buffer_index++];
    // if there is more to send, ack, otherwise nack
    if(twi_tx_buffer_index < twi_tx_buffer_length){
      au_twi_reply(1);
    }
    else {
      au_twi_reply(0);
    }
    break;
  case TW_ST_DATA_NACK: // received nack, we are done 
  case TW_ST_LAST_DATA: // received ack, but we are done already!
    // ack future responses
    au_twi_reply(1);
    // leave slave receiver state
    twi_state = AU_TWI_READY;
    break;
  // All
  case TW_NO_INFO:   // no state information
    break;
  case TW_BUS_ERROR: // bus error, illegal stop/start
    twi_error = TW_BUS_ERROR;
    au_twi_stop();
    break;
  }
}