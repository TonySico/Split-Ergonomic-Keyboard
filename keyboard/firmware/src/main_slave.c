/*
 * demo_slave.c
 *
 */

#include "helper.h"
#include "i2c.h"
#include "usb.h"

// Allows for placing of ISRs at correct addresses, specific to each device
// (32u4 here), using ISR(<VECTOR>) macro
#define _AVR_IO_H_ // Need this to use versions of register names and not those
                   // AVR libraries tries to define for 32u4
#include <avr/interrupt.h>

#define LOW 0
#define HIGH 1

#define NUM_IN_PIN 5
#define NUM_OUT_PIN 6

uint8_t keyMatrixSlave[NUM_IN_PIN];
volatile uint8_t matrixIndex;

#define TWI_VECT __vector_36 /* TWI two wire serial interface */

/**
 *  @initOutputPins
 *  calls the functions that configures output pins
 *
 */
void initOutputPins() {
  configureOut(4);
  configureOut(5);
  configureOut(6);
  configureOut(7);
  configureOut(8);
  configureOut(9);

  setOutPin(4, 1);
  setOutPin(5, 1);
  setOutPin(6, 1);
  setOutPin(7, 1);
  setOutPin(8, 1);
  setOutPin(9, 1);
}

/**
 *  @initInputPins
 *  calls the functions that configures input pins
 *
 */
void initInputPins() {
  configureIn(10, 1); // Button

  configureIn(14, 1);
  configureIn(15, 1);
  configureIn(16, 1);
  configureIn(18, 1);
  configureIn(19, 1);
}

int main(void) {

  initOutputPins();
  initInputPins();

  // configureUSBInterface();
  // configureEndpointZero();

  i2cInitSlave();

  __no_op();

  // outputRegister(TWAR, 9);

  // for (uint8_t i = 0; i < NUM_IN_PIN; i++) {
  //   // Fills matrix with values 0x01 -> 0x05
  //   keyMatrixSlave[i] = 0x00;
  // }

  for (uint8_t i = 0; i < NUM_IN_PIN; i++) {
    // Fills matrix with values 0x01 -> 0x05
    keyMatrixSlave[i] = 0;
  }

  matrixIndex = 0;
  // setOutPin(0, HIGH);

  while (1) {
    matrixScan(keyMatrixSlave);
    // togglePin(0);
    // delay(100000);

    // send_message("Y!");
    // for (int i = 0; i < NUM_IN_PIN; i++) {
    //   for (int j = 0; j < 8; j++) {
    //     send_int((keyMatrixSlave[i] >> j) & 0x1);
    //   }
    //   send_message(" !");
    // }
    // stepButton(10);
  }
}

ISR(TWI_VECT) {

  switch (*TWSR & TWI_TWSR_MASK) {

  case TWI_ST_SLA_ACK:          // SLA+R received, ACK sent
  case TWI_ST_ARB_LOST_SLA_ACK: // Arbitration lost, SLA+R received
    // setOutPin(1, LOW);
    *TWDR = keyMatrixSlave[matrixIndex]; // Load data to send
    *TWCR = TWINT | TWEN | TWEA;         // Transmit first byte
    matrixIndex++;
    break;

  case TWI_ST_DATA_ACK:                  // Data transmitted, ACK received
    *TWDR = keyMatrixSlave[matrixIndex]; // Load data to send
    *TWCR = TWINT | TWEN | TWEA;         // Send byte
    matrixIndex++;
    break;

  case TWI_ST_DATA_NACK:         // Data transmitted, NACK received (last byte)
  case TWI_ST_LAST_DATA:         // Last data byte transmitted, ACK received
    *TWCR = TWINT | TWEN | TWEA; // Release bus
    matrixIndex = 0;
    break;

  case 0xA0:                     // STOP or repeated START received
    *TWCR = TWINT | TWEN | TWEA; // ACK and wait
    matrixIndex = 0;
    break;

  default:
    matrixIndex = 0;
    *TWCR = 0;
    delay(4000);
    *TWCR = TWINT | TWEN | TWEA;
    // setOutPin(1, HIGH);
    break;
  }
  *TWCR |= TWIE | TWINT | TWEA | TWEN;
}
