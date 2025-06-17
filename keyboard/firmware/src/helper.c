/*
 * helper.c
 * Implementation of helper.h functions
 *
 */

#include "helper.h"

#define _AVR_IO_H_ // Need this to use versions of register names and not those
                   // AVR libraries tries to define for 32u4
#include <avr/interrupt.h>

#include <stdint.h>

const uint8_t inPins[NUM_IN_PIN] = {19, 18, 15, 14, 16};
const uint8_t outPins[NUM_OUT_PIN] = {4, 5, 6, 7, 8, 9};

// I/O registers are from 0x0020 to 0x005F
/* Specifically:
 * PIN#  = In port for the pins
 * DDR#  = port setting
 * PORT# = what to set for output
 * */
volatile uint8_t *SPCR = (uint8_t *)(0x4C);
volatile uint8_t *SREG = (uint8_t *)(0x3F);
volatile uint8_t *UCSR1B = (uint8_t *)(0xC9);

/** TWI (aka I2C) */

volatile uint8_t *TWCR = (uint8_t *)(0xBC);

// 2-wire Serial Interface Data Register
volatile uint8_t *TWDR = (uint8_t *)(0xBB);

// Set to determine the address of the slave
volatile uint8_t *TWAR = (uint8_t *)(0xBA);

volatile uint8_t *TWSR = (uint8_t *)(0xB9);

// 2-wire Serial Interface Bit Rate Register
volatile uint8_t *TWBR = (uint8_t *)(0xB8);

/** GPIO port B */
volatile uint8_t *PINB = (uint8_t *)(0x23);
volatile uint8_t *DDRB = (uint8_t *)(0x24);
volatile uint8_t *PORTB = (uint8_t *)(0x25);

uint8_t onBoardLEDRight = 0; // usbc at bottom of board
uint8_t pinB1 = 1;
uint8_t pinB2 = 2;
uint8_t pinB3 = 3;
uint8_t pinB4 = 4;
uint8_t pinB5 = 5;
uint8_t pinB6 = 6;

/** GPIO port C */
volatile uint8_t *PINC = (uint8_t *)(0x26);
volatile uint8_t *DDRC = (uint8_t *)(0x27);
volatile uint8_t *PORTC = (uint8_t *)(0x28);

uint8_t pinC6 = 6;

/** GPIO port D */
volatile uint8_t *PIND = (uint8_t *)(0x29);
volatile uint8_t *DDRD = (uint8_t *)(0x2A);
volatile uint8_t *PORTD = (uint8_t *)(0x2B);

uint8_t pinD0 = 0;
uint8_t pinD1 = 1;
uint8_t pinD4 = 4;
uint8_t onBoardLEDLeft = 5; // usbc at bottom of board
uint8_t pinD7 = 7;

/** GPIO port E */
volatile uint8_t *PINE = (uint8_t *)(0x2C);
volatile uint8_t *DDRE = (uint8_t *)(0x2D);
volatile uint8_t *PORTE = (uint8_t *)(0x2E);

uint8_t pinE6 = 6;

/** GPIO port F */
volatile uint8_t *PINF = (uint8_t *)(0x2F);
volatile uint8_t *DDRF = (uint8_t *)(0x30);
volatile uint8_t *PORTF = (uint8_t *)(0x31);

uint8_t pinF4 = 4;
uint8_t pinF5 = 5;
uint8_t pinF6 = 6;
uint8_t pinF7 = 7;

/** GPIO register 0 */
volatile uint8_t *GPIOR0 = (uint8_t *)(0x3E);

/**
 *  @delay
 *  wait a certain ammount of time
 *  for more accuracy you need to look up clock speed good for testing
 *  @time: the time in "ms" that the board waits (good value= 500,000)
 *
 */
void delay(unsigned long int time) {
  for (volatile unsigned long int i = 0; i < time; i++)
    ;
}

/**
 *  @__no_op
 *  Literally a no operation
 *  used to sync input pins
 *
 */
void __no_op() {}

/**
 *  @togglePin
 *  toggle the current pin
 *  @pinNumber: the pin number on the board that will be toggled
 *
 */
void togglePin(uint8_t pinNumber) {
  switch (pinNumber) {
  case 0: // B0
    *PORTB ^= (1 << onBoardLEDRight);
    break;
  case 1: // D5
    *PORTD ^= (1 << onBoardLEDLeft);
    break;
  case 2: // D1
    *PORTD ^= (1 << pinD1);
    break;
  case 3: // D0
    *PORTD ^= (1 << pinD0);
    break;
  case 4: // D4
    *PORTD ^= (1 << pinD4);
    break;
  case 5: // C6
    *PORTC ^= (1 << pinC6);
    break;
  case 6: // D7
    *PORTD ^= (1 << pinD7);
    break;
  case 7: // E6
    *PORTE ^= (1 << pinE6);
    break;
  case 8: // B4
    *PORTB ^= (1 << pinB4);
    break;
  case 9: // B5
    *PORTB ^= (1 << pinB5);
    break;
  case 10: // B6
    *PORTB ^= (1 << pinB6);
    break;
  case 14: // B3
    *PORTB ^= (1 << pinB3);
    break;
  case 15: // B1
    *PORTB ^= (1 << pinB1);
    break;
  case 16: // B2
    *PORTB ^= (1 << pinB2);
    break;
  case 18: // F7
    *PORTF ^= (1 << pinF7);
    break;
  case 19: // F6
    *PORTF ^= (1 << pinF6);
    break;
  case 20: // F5
    *PORTF ^= (1 << pinF5);
    break;
  case 21: // F4
    *PORTF ^= (1 << pinF4);
    break;
  default:
    return;
  }
  return;
}

/**
 *  @setOut
 *  sets the output of a pin
 *  @pinNumber: the pin number on the board that will have it's state set
 *  @highLow:   whether the pin is 0 or 1
 *
 */
void setOutPin(uint8_t pinNumber, uint8_t highLow) {
  switch (pinNumber) {
  case 0: // B0 NOTE: Must invert (not / !) highLow value as the pin is
          // active-low
    *PORTB = (*PORTB & ~(1 << onBoardLEDRight)) | (!highLow << onBoardLEDRight);
    break;
  case 1: // D5
    *PORTD = (*PORTD & ~(1 << onBoardLEDLeft)) | (highLow << onBoardLEDLeft);
    break;
  case 2: // D1
    *PORTD = (*PORTD & ~(1 << pinD1)) | (highLow << pinD1);
    break;
  case 3: // D0
    *PORTD = (*PORTD & ~(1 << pinD0)) | (highLow << pinD0);
    break;
  case 4: // D4
    *PORTD = (*PORTD & ~(1 << pinD4)) | (highLow << pinD4);
    break;
  case 5: // C6
    *PORTC = (*PORTC & ~(1 << pinC6)) | (highLow << pinC6);
    break;
  case 6: // D7
    *PORTD = (*PORTD & ~(1 << pinD7)) | (highLow << pinD7);
    break;
  case 7: // E6
    *PORTE = (*PORTE & ~(1 << pinE6)) | (highLow << pinE6);
    break;
  case 8: // B4
    *PORTB = (*PORTB & ~(1 << pinB4)) | (highLow << pinB4);
    break;
  case 9: // B5
    *PORTB = (*PORTB & ~(1 << pinB5)) | (highLow << pinB5);
    break;
  case 10: // B6
    *PORTB = (*PORTB & ~(1 << pinB6)) | (highLow << pinB6);
    break;
  case 14: // B3
    *PORTB = (*PORTB & ~(1 << pinB3)) | (highLow << pinB3);
    break;
  case 15: // B1
    *PORTB = (*PORTB & ~(1 << pinB1)) | (highLow << pinB1);
    break;
  case 16: // B2
    *PORTB = (*PORTB & ~(1 << pinB2)) | (highLow << pinB2);
    break;
  case 18: // F7
    *PORTF = (*PORTF & ~(1 << pinF7)) | (highLow << pinF7);
    break;
  case 19: // F6
    *PORTF = (*PORTF & ~(1 << pinF6)) | (highLow << pinF6);
    break;
  case 20: // F5
    *PORTF = (*PORTF & ~(1 << pinF5)) | (highLow << pinF5);
    break;
  case 21: // F4
    *PORTF = (*PORTF & ~(1 << pinF4)) | (highLow << pinF4);
    break;
  default:
    return;
  }
  return;
}

/**
 *  @checkPinIn
 *  reads the input of the selected pin
 *  not super intuitive, it sets it to 0 by default as a 1 is the expected
 *  default for the pull-down pins
 *  @pinNumber: the pin number on the board that will be read
 *  @Return:    returns an int (0 or 1) acting as a bool for the pin
 *
 */
uint8_t checkPinIn(uint8_t pinNumber) {
  switch (pinNumber) {
  case 2: // D1
    return ((((*PIND & (1 << pinD1)) >> pinD1) == 0x0001) ? 0 : 1);
    break;
  case 3: // D0
    return ((((*PIND & (1 << pinD0)) >> pinD0) == 0x0001) ? 0 : 1);
    break;
  case 4: // D4
    return ((((*PIND & (1 << pinD4)) >> pinD4) == 0x0001) ? 0 : 1);
    break;
  case 5: // C6
    return ((((*PINC & (1 << pinC6)) >> pinC6) == 0x0001) ? 0 : 1);
    break;
  case 6: // D7
    return ((((*PIND & (1 << pinD7)) >> pinD7) == 0x0001) ? 0 : 1);
    break;
  case 7: // E6
    return ((((*PINE & (1 << pinE6)) >> pinE6) == 0x0001) ? 0 : 1);
    break;
  case 8: // B4
    return ((((*PINB & (1 << pinB4)) >> pinB4) == 0x0001) ? 0 : 1);
    break;
  case 9: // B5
    return ((((*PINB & (1 << pinB5)) >> pinB5) == 0x0001) ? 0 : 1);
    break;
  case 10: // B6
    return ((((*PINB & (1 << pinB6)) >> pinB6) == 0x0001) ? 0 : 1);
    break;
  case 14: // B3
    return ((((*PINB & (1 << pinB3)) >> pinB3) == 0x0001) ? 0 : 1);
    break;
  case 15: // B1
    return ((((*PINB & (1 << pinB1)) >> pinB1) == 0x0001) ? 0 : 1);
    break;
  case 16: // B2
    return ((((*PINB & (1 << pinB2)) >> pinB2) == 0x0001) ? 0 : 1);
    break;
  case 18: // F7
    return ((((*PINF & (1 << pinF7)) >> pinF7) == 0x0001) ? 0 : 1);
    break;
  case 19: // F6
    return ((((*PINF & (1 << pinF6)) >> pinF6) == 0x0001) ? 0 : 1);
    break;
  case 20: // F5
    return ((((*PINF & (1 << pinF5)) >> pinF5) == 0x0001) ? 0 : 1);
    break;
  case 21: // F4
    return ((((*PINF & (1 << pinF4)) >> pinF4) == 0x0001) ? 0 : 1);
    break;
  default:
    return (0);
  }
}

/**
 *  @configureOut
 *  sets the DDRx port high to use PORTx as output high/low
 *  @pinNumber: the pin number on the board that will be set to output mode
 *
 */
void configureOut(uint8_t pinNumber) {
  switch (pinNumber) {
  case 0: // B0
    *DDRB = (*DDRB & ~(1 << onBoardLEDRight)) | (1 << onBoardLEDRight);
    break;
  case 1: // D5
    *DDRD = (*DDRD & ~(1 << onBoardLEDLeft)) | (1 << onBoardLEDLeft);
    break;
  case 2: // D1
    *DDRD = (*DDRD & ~(1 << pinD1)) | (1 << pinD1);
    break;
  case 3: // D0
    *DDRD = (*DDRD & ~(1 << pinD0)) | (1 << pinD0);
    break;
  case 4: // D4
    *DDRD = (*DDRD & ~(1 << pinD4)) | (1 << pinD4);
    break;
  case 5: // C6
    *DDRC = (*DDRC & ~(1 << pinC6)) | (1 << pinC6);
    break;
  case 6: // D7
    *DDRD = (*DDRD & ~(1 << pinD7)) | (1 << pinD7);
    break;
  case 7: // E6
    *DDRE = (*DDRE & ~(1 << pinE6)) | (1 << pinE6);
    break;
  case 8: // B4
    *DDRB = (*DDRB & ~(1 << pinB4)) | (1 << pinB4);
    break;
  case 9: // B5
    *DDRB = (*DDRB & ~(1 << pinB5)) | (1 << pinB5);
    break;
  case 10: // B6
    *DDRB = (*DDRB & ~(1 << pinB6)) | (1 << pinB6);
    break;
  case 14: // B3
    *DDRB = (*DDRB & ~(1 << pinB3)) | (1 << pinB3);
    break;
  case 15: // B1
    *DDRB = (*DDRB & ~(1 << pinB1)) | (1 << pinB1);
    break;
  case 16: // B2
    *DDRB = (*DDRB & ~(1 << pinB2)) | (1 << pinB2);
    break;
  case 18: // F7
    *DDRF = (*DDRF & ~(1 << pinF7)) | (1 << pinF7);
    break;
  case 19: // F6
    *DDRF = (*DDRF & ~(1 << pinF6)) | (1 << pinF6);
    break;
  case 20: // F5
    *DDRF = (*DDRF & ~(1 << pinF5)) | (1 << pinF5);
    break;
  case 21: // F4
    *DDRF = (*DDRF & ~(1 << pinF4)) | (1 << pinF4);
    break;
  default:
    return;
  }
  return;
}

/**
 *  @configureIn
 *  sets the DDRx port low and PORTx to determing input behaviour
 *  @pinNumber: the pin number on the board that will be set to input mode
 *  @setting:   1 = pull-down, 0 = tri-state (pull-up)
 *              currently only pull-down seems to work
 *              this means you need to use a button that grounds the signal
 *              setting it low
 *
 */

void configureIn(uint8_t pinNumber, uint8_t setting) {
  switch (pinNumber) {
  case 2: // D1
    *DDRD = (*DDRD & ~(1 << pinD1));
    *PORTD = (*PORTD & ~(1 << pinD1)) | (setting << pinD1);
    break;
  case 3: // D0
    *DDRD = (*DDRD & ~(1 << pinD0));
    *PORTD = (*PORTD & ~(1 << pinD0)) | (setting << pinD0);
    break;
  case 4: // D4
    *DDRD = (*DDRD & ~(1 << pinD4));
    *PORTD = (*PORTD & ~(1 << pinD4)) | (setting << pinD4);
    break;
  case 5: // C6
    *DDRC = (*DDRC & ~(1 << pinC6));
    *PORTC = (*PORTC & ~(1 << pinC6)) | (setting << pinC6);
    break;
  case 6: // D7
    *DDRD = (*DDRD & ~(1 << pinD7));
    *PORTD = (*PORTD & ~(1 << pinD7)) | (setting << pinD7);
    break;
  case 7: // E6
    *DDRE = (*DDRE & ~(1 << pinE6));
    *PORTE = (*PORTE & ~(1 << pinE6)) | (setting << pinE6);
    break;
  case 8: // B4
    *DDRB = (*DDRB & ~(1 << pinB4));
    *PORTB = (*PORTB & ~(1 << pinB4)) | (setting << pinB4);
    break;
  case 9: // B5
    *DDRB = (*DDRB & ~(1 << pinB5));
    *PORTB = (*PORTB & ~(1 << pinB5)) | (setting << pinB5);
    break;
  case 10: // B6
    *DDRB = (*DDRB & ~(1 << pinB6));
    *PORTB = (*PORTB & ~(1 << pinB6)) | (setting << pinB6);
    break;
  case 14: // B3
    *DDRB = (*DDRB & ~(1 << pinB3));
    *PORTB = (*PORTB & ~(1 << pinB3)) | (setting << pinB3);
    break;
  case 15: // B1
    *DDRB = (*DDRB & ~(1 << pinB1));
    *PORTB = (*PORTB & ~(1 << pinB1)) | (setting << pinB1);
    break;
  case 16: // B2
    *DDRB = (*DDRB & ~(1 << pinB2));
    *PORTB = (*PORTB & ~(1 << pinB2)) | (setting << pinB2);
    break;
  case 18: // F7
    *DDRF = (*DDRF & ~(1 << pinF7));
    *PORTF = (*PORTF & ~(1 << pinF7)) | (setting << pinF7);
    break;
  case 19: // F6
    *DDRF = (*DDRF & ~(1 << pinF6));
    *PORTF = (*PORTF & ~(1 << pinF6)) | (setting << pinF6);
    break;
  case 20: // F5
    *DDRF = (*DDRF & ~(1 << pinF5));
    *PORTF = (*PORTF & ~(1 << pinF5)) | (setting << pinF5);
    break;
  case 21: // F4
    *DDRF = (*DDRF & ~(1 << pinF4));
    *PORTF = (*PORTF & ~(1 << pinF4)) | (setting << pinF4);
    break;
  default:
    return;
  }
  return;
}

void sanity(int loop) {
  for (int i = 0; i < loop; i++) {
    togglePin(0);
    delay(500000);
    togglePin(0);
    delay(500000);
  }
  togglePin(1);
  delay(250000);
  togglePin(1);
}

/**
 *  @flashRegister
 *  Flash the contents of a given register using the onboard LED (pin 0)
 *  @regPointer: the address of the register you wish to flash [int*]
 *
 */
void flashRegister(volatile uint8_t *regPointer) {
  for (uint8_t i = 0; i < 8; i++) {
    setOutPin(0, ((*regPointer) & (1 << i)));
    delay(250000);
  }

  return;
}

/**
 *  @matrixScan
 *  function that scans through the matrix changing what pins are low to enable
 *  @keyMatrix: pointer to the matrix
 *              uses in_pin so that there are less bytes to send (5 < 6)
 *
 */
void matrixScan(uint8_t *keyMatrix) {
  cli();
  for (int currentOutPin = 0; currentOutPin < NUM_OUT_PIN; currentOutPin++) {
    setOutPin(outPins[currentOutPin], 0);

    for (int currentInPin = 0; currentInPin < NUM_IN_PIN; currentInPin++) {
      if (checkPinIn(inPins[currentInPin]) == 1) {
        keyMatrix[currentInPin] |= (1 << currentOutPin);
      } else {
        keyMatrix[currentInPin] &= ~(1 << currentOutPin);
      }
    }

    setOutPin(outPins[currentOutPin], 1);
  }
  sei();
}

/**
 *  @outputRegister
 *  Output the contents of a given register using pin 10 (most significant bit)
 *  up to pin 21 (A3 on silkscreen) (least significant bit)
 *  the LEDs will be lit until the @pinNumber is pressed
 *  at which point the LEDs will all be turned off and code will proceed
 *  @regPointer: the address of the register you wish to output [int*]
 *  @pinNumber: the pin number on the board that will be read
 *
 */
void outputRegister(volatile uint8_t *regPointer, int pinNumber) {
  setOutPin(10, (((*regPointer) & (0x01 << 7)) >> 7));
  setOutPin(16, (((*regPointer) & (0x01 << 6)) >> 6));
  setOutPin(14, (((*regPointer) & (0x01 << 5)) >> 5));
  setOutPin(15, (((*regPointer) & (0x01 << 4)) >> 4));
  setOutPin(18, (((*regPointer) & (0x01 << 3)) >> 3));
  setOutPin(19, (((*regPointer) & (0x01 << 2)) >> 2));
  setOutPin(20, (((*regPointer) & (0x01 << 1)) >> 1));
  setOutPin(21, (((*regPointer) & (0x01 << 0)) >> 0));
  stepButton(pinNumber);
  setOutPin(10, LOW);
  setOutPin(16, LOW);
  setOutPin(14, LOW);
  setOutPin(15, LOW);
  setOutPin(18, LOW);
  setOutPin(19, LOW);
  setOutPin(20, LOW);
  setOutPin(21, LOW);
  __no_op();
  togglePin(10);
  togglePin(16);
  togglePin(14);
  togglePin(15);
  togglePin(18);
  togglePin(19);
  togglePin(20);
  togglePin(21);
  delay(100000);
  setOutPin(10, LOW);
  setOutPin(16, LOW);
  setOutPin(14, LOW);
  setOutPin(15, LOW);
  setOutPin(18, LOW);
  setOutPin(19, LOW);
  setOutPin(20, LOW);
  setOutPin(21, LOW);
  delay(100000);
}

/**
 *  @print8Bit
 *  displays the value provided accross the 8 LEDS
 *  @val:       the 8 bit value being printed
 *  @pinNumber: the pin number on the board that will be read
 *
 */
void print8Bit(uint8_t val, int pinNumber) {
  setOutPin(10, (((val) & (0x01 << 7)) >> 7));
  setOutPin(16, (((val) & (0x01 << 6)) >> 6));
  setOutPin(14, (((val) & (0x01 << 5)) >> 5));
  setOutPin(15, (((val) & (0x01 << 4)) >> 4));
  setOutPin(18, (((val) & (0x01 << 3)) >> 3));
  setOutPin(19, (((val) & (0x01 << 2)) >> 2));
  setOutPin(20, (((val) & (0x01 << 1)) >> 1));
  setOutPin(21, (((val) & (0x01 << 0)) >> 0));
  stepButton(pinNumber);
  setOutPin(10, LOW);
  setOutPin(16, LOW);
  setOutPin(14, LOW);
  setOutPin(15, LOW);
  setOutPin(18, LOW);
  setOutPin(19, LOW);
  setOutPin(20, LOW);
  setOutPin(21, LOW);
  __no_op();
  togglePin(10);
  togglePin(16);
  togglePin(14);
  togglePin(15);
  togglePin(18);
  togglePin(19);
  togglePin(20);
  togglePin(21);
  delay(100000);
  setOutPin(10, LOW);
  setOutPin(16, LOW);
  setOutPin(14, LOW);
  setOutPin(15, LOW);
  setOutPin(18, LOW);
  setOutPin(19, LOW);
  setOutPin(20, LOW);
  setOutPin(21, LOW);
  delay(100000);
}

/**
 *  @stepButton
 *  an infinite loop that ends when a button is pressed
 *  intented for use with outputRegister() to allow for easier reading of
 *  registers
 *  @pinNumber: the pin number on the board that will be read
 *
 */
void stepButton(uint8_t pinNumber) {
  while (1) {
    if (checkPinIn(pinNumber)) {
      delay(100000);
      break;
    }
  }
}
