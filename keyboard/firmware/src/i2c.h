/*
 * i2c.h
 */

#ifndef I2C_H
#define I2C_H

/*
 * TWBR Register controls clock speed for SCL should be higher than 10 (P. 231)
 *
 * Protocol:
 *    1. Application: writes to TWCR register initiating Start
 *    2. Hardware: Sets TWINT flag
 *    3. Application: Check TWSR to see if it was sent
 *    4. Hardware: TWINT set. Status code indicates ACK/NACK
 *    5.
 *    6.
 *    7.
 *
 */

// #define SCL_CLOCK 100000L
#define SCL_CLOCK 50000UL

// I shouldn't have to tell you what number this is
// #define SLAVE_ADDRESS 0b0100 0101
#define SLAVE_ADDRESS 0x45

#define TWI_TWSR_MASK 0xF8

#define TW_START 0x08
#define TW_REP_START 0x10

#define TWI_MR_ARB_LOST 0x38
#define TWI_MR_SLA_ACK 0x40
#define TWI_MR_SLA_NACK 0x48
#define TWI_MR_DATA_ACK 0x50
#define TWI_MR_DATA_NACK 0x58

#define TWI_ST_SLA_ACK 0xA8
#define TWI_ST_ARB_LOST_SLA_ACK 0xB0
#define TWI_ST_DATA_ACK 0xB8
#define TWI_ST_DATA_NACK 0xC0
#define TWI_ST_LAST_DATA 0xC8

#ifndef F_CPU
#define F_CPU 16000000UL // CPU Clock 16Mhz
#endif

// passed along with SLA to indicate read request from master, or write from
// master
#define READ 1
#define WRITE 0

#include <stdint.h>

/**
 *  @i2cInitSlave
 *  sets the addres for TWI communication on the slave board
 *
 */
void i2cInitSlave();

/**
 *  @i2cInitMaster
 *  Configure the TWBR to have appropriate SCL speed
 *  Configure TWSR so there is no prescalar
 *  Set TWCR TWEN bit to enable TWI
 *
 */
void i2cInitMaster();

/**
 *  @i2cStart
 *  Set TWCR TWINT flag
 *  Set TWCR TWSTA bit to send start signal
 *  Set TWCR TWEN bit to enable TWI
 *
 */
void i2cStart();

/**
 *  @i2cSync
 *  Waits until twint is cleared
 *
 */
void i2cSync();

/**
 *  @i2cAddress
 *  Connect to the appropriate address
 *  @address: Address of the slave
 *  @requestWrite: Whether the master is reading or writing
 *
 */
void i2cAddress(uint8_t slaveAddr, uint8_t requestWrite);

/**
 *  @i2cMasterRecieve@
 *  Connect to the appropriate address
 *  @address: Address of the slave
 *  @requestWrite: Whether the master is reading or writing
 *
 */
void i2cMasterRecieve(uint8_t slaveAddr, uint8_t *matrix, uint8_t length);

/**
 *  @i2cStop
 *  Set TWCR TWINT flag
 *  Set TWCR TWSTO bit to send stop signal
 *  Set TWCR TWEN bit to enable TWI
 *
 */
void i2cStop();

/**
 *  @i2cReadAck
 *  Reads the data coming in and sends an ACK
 *  @Return: returns the recieved byte
 */
uint8_t i2cReadAck();

/**
 *  @i2cReadAck
 *  Reads the data coming in and sends a NACK
 *  @Return: returns the recieved byte
 */
uint8_t i2cReadNack();

#endif // I2C_H
