/*
 * i2c.c
 * Implementation of i2c.h functions
 *
 */

#include "i2c.h"
#include "helper.h"
#include "usb.h"
#include <stdint.h>

/**
 *  @i2cInitSlave
 *  sets the addres for TWI communication on the slave board
 *
 */
void i2cInitSlave() {
  cli();
  *TWCR = 0;
  // Set the address of the slave
  *TWAR = (0x45 << 1);
  // expect 1000 1010
  *TWCR = TWEN | TWEA | TWIE | TWINT;
  sei();
}

/**
 *  @i2cReset
 *  Configure SCL and SDA as output pins
 *  Toggle them both, then restart normal config
 *
 */
void i2cReset() {
  // set SCL and SDA pins as outputs
  configureOut(3); // SCL
  configureOut(2); // SDA
  // Sets both SCL and SDA high
  setOutPin(3, 1);
  setOutPin(2, 1);

  for (int i = 0; i < 9; i++) {
    togglePin(3); // SCL low
    delay(10);
    togglePin(3); // SCL high
    delay(10);
  }

  // Simulate a fake stop signal
  setOutPin(2, 0); // SDA low
  delay(10);
  setOutPin(3, 1); // SCL high
  delay(10);
  setOutPin(2, 1); // SDA low

  *TWCR = TWEN;
}

/**
 *  @i2cCheckAndRecover
 *  Check the status of i2c, and reset it if it isn't responsive
 *
 */
void i2cCheckAndRecover() {
  uint8_t status = *TWSR & TWI_TWSR_MASK;
  // shift back the twen bit to have a single 0 or 1
  uint8_t i2cOn = (*TWCR & (TWEN)) >> 2;
  uint8_t fix;

  switch (status) {
  case TW_START:
  case TW_REP_START:
  case TWI_MR_SLA_ACK:
  case TWI_MR_DATA_ACK:
  case TWI_MR_DATA_NACK:
    fix = 0;
  default:
    fix = 1;
  }

  // Fix is not working forsome reason, I believe this is what is causing issues
  // with respetct to the master not starting properly once the slave is reset,
  // until the master itsle fis reset
  if (fix || !i2cOn) {
    i2cReset();

    togglePin(0);
    delay(10000);

    i2cInitMaster();
  }
}

/**
 *  @i2cInitMaster
 *  Configure the TWBR to have appropriate SCL speed
 *  Configure TWSR so there is no prescalar
 *  Set TWCR TWEN bit to enable TWI
 *
 */
void i2cInitMaster() {

  *TWCR = 0;

  *TWSR = ~(TWPS0 | TWPS1);

  *TWBR = (((F_CPU / SCL_CLOCK) - 16) / 2);

  *TWCR = TWEN;
}

/**
 *  @i2cSync
 *  Waits until twint is cleared
 *
 */
void i2cSyncCount(unsigned long int timer) {

  int i = 0;
  while (!(*TWCR & TWINT)) {
    i++;
    togglePin(8);
    delay(timer);
    if (i == 5)
      return;
  }
  setOutPin(8, LOW);
}

/**
 *  @i2cSync
 *  Waits until twint is cleared
 *
 */
void i2cSync() {
  int counter = 0;
  while ((!(*TWCR & TWINT)) && counter < 25000)
    counter++;
}

/**
 *  @i2cStart
 *  Set TWCR TWINT flag
 *  Set TWCR TWSTA bit to send start signal
 *  Set TWCR TWEN bit to enable TWI
 *
 */
void i2cStart() {

  *TWCR = TWINT | TWSTA | TWEN;

  i2cSync();
  // *TWDR = (SLAVE_ADDRESS << 1) | 1;
}

/**
 *  @i2cAddress
 *  Connect to the appropriate address
 *  @address: Address of the slave
 *  @requestWrite: Whether the master is reading or writing
 *
 */
void i2cAddress(const uint8_t slaveAddr, uint8_t requestWrite) {

  *TWDR = ((slaveAddr << 1) | requestWrite);
  *TWCR = TWINT | TWEN;

  i2cSync();
}

/**
 *  @i2cMasterRecieve
 *  A complete master recieve i2c transaction
 *  @slaveAddr:   Address of the slave
 *  @slaveMatrix: The slave side keyboard matrix being filed
 *  @length:      The length of the expected matrix
 *
 */
void i2cMasterRecieve(uint8_t slaveAddr, uint8_t *slaveMatrix, uint8_t length) {

  i2cStart();

  if ((*TWSR & 0xF8) != 0x08) {
    return;
  }

  i2cAddress(slaveAddr, READ);
  if ((*TWSR & 0xF8) != TWI_MR_SLA_ACK) {
    return;
  }

  for (uint8_t i = 0; i < (length - 1); i++) {
    slaveMatrix[i] = i2cReadAck();
  }
  slaveMatrix[length - 1] = i2cReadNack();

  i2cStop();
}

/**
 *  @i2cStop
 *  Set TWCR TWINT flag
 *  Set TWCR TWSTO bit to send stop signal
 *  Set TWCR TWEN bit to enable TWI
 *
 */
void i2cStop() {
  *TWCR = TWINT | TWSTO | TWEN;
  while (*TWCR & TWSTO)
    ;
}

/**
 *  @i2cReadAck
 *  Reads the data coming in and sends an ACK
 *  @Return: returns the recieved byte
 */
uint8_t i2cReadAck() {
  *TWCR = TWINT | TWEN | TWEA;

  i2cSync();

  uint8_t data = *TWDR;

  return data;
}

/**
 *  @i2cReadNack
 *  Reads the data coming in and sends a NACK
 *  @Return: returns the recieved byte
 */
uint8_t i2cReadNack() {
  *TWCR = TWINT | TWEN;

  i2cSync();

  uint8_t data = *TWDR;

  return data;
}
