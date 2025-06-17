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
  // Set the address of the slave
  *TWAR = (0x45 << 1);
  // expect 1000 1010
  *TWCR = TWEN | TWEA | TWIE | TWINT;
  sei();
}

/**
 *  @i2cInitMaster
 *  Configure the TWBR to have appropriate SCL speed
 *  Configure TWSR so there is no prescalar
 *  Set TWCR TWEN bit to enable TWI
 *
 */
void i2cInitMaster() {
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
