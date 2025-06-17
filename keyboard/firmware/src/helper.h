/*
 * helper.h
 *
 */

#ifndef HELPER_H
#define HELPER_H

#include <stdint.h>

#define LOW 0
#define HIGH 1

#define NUM_IN_PIN 5
#define NUM_OUT_PIN 6

extern volatile uint8_t *SPCR;
extern volatile uint8_t *SREG;
extern volatile uint8_t *UCSR1B;

// 2-wire Serial Interface Bit Rate Register
// SCL frequency = (CPU Clock Frequency)/(16 + 2 * TWBR * 4^(TWPS))
// Should be higher than 10
extern volatile uint8_t *TWBR;

extern volatile uint8_t *TWCR;
#define TWINT (1 << 7)
#define TWEA (1 << 6)
#define TWSTA (1 << 5)
#define TWSTO (1 << 4)
#define TWWC (1 << 3)
#define TWEN (1 << 2)
#define TWIE (1 << 0)

// 2-wire Serial Interface Data Register
extern volatile uint8_t *TWDR;

// Set to determine the address of the slave
extern volatile uint8_t *TWAR;

extern volatile uint8_t *TWSR;
#define TWPS0 (1 << 0)
#define TWPS1 (1 << 1)

/** GPIO port B */
extern volatile uint8_t *PINB;
extern volatile uint8_t *DDRB;
extern volatile uint8_t *PORTB;

extern uint8_t onBoardLEDRight; // usbc at bottom of board
extern uint8_t pinB1;
extern uint8_t pinB2;
extern uint8_t pinB3;
extern uint8_t pinB4;
extern uint8_t pinB5;
extern uint8_t pinB6;

/** GPIO port C */
extern volatile uint8_t *PINC;
extern volatile uint8_t *DDRC;
extern volatile uint8_t *PORTC;

extern uint8_t pinC6;

/** GPIO port D */
extern volatile uint8_t *PIND;
extern volatile uint8_t *DDRD;
extern volatile uint8_t *PORTD;

extern uint8_t pinD0;
extern uint8_t pinD1;
extern uint8_t pinD4;
extern uint8_t onBoardLEDLeft; // usbc at bottom of board
extern uint8_t pinD7;

/** GPIO port E */
extern volatile uint8_t *PINE;
extern volatile uint8_t *DDRE;
extern volatile uint8_t *PORTE;

extern uint8_t pinE6;

/** GPIO port F */
extern volatile uint8_t *PINF;
extern volatile uint8_t *DDRF;
extern volatile uint8_t *PORTF;

extern uint8_t pinF4;
extern uint8_t pinF5;
extern uint8_t pinF6;
extern uint8_t pinF7;

/** GPIO register 0 */
extern volatile uint8_t *GPIOR0;

/**
 *  @delay
 *  wait a certain ammount of time
 *  for more accuracy you need to look up clock speed good for testing
 *  @time: the time in "ms" that the board waits (good value= 500,000)
 *
 */
void delay(unsigned long int time);

/**
 *  @__no_op
 *  Literally a no operation
 *  used to sync input pins
 *
 */
void __no_op();

/**
 *  @togglePin
 *  toggle the current pin
 *  @pinNumber: the pin number on the board that will be toggled
 *
 */
void togglePin(uint8_t pinNumber);

/**
 *  @setOut
 *  sets the output of a pin
 *  @pinNumber: the pin number on the board that will have it's state set
 *  @highLow:   whether the pin is 0 or 1
 *
 */
void setOutPin(uint8_t pinNumber, uint8_t highLow);

/**
 *  @checkPinIn
 *  reads the input of the selected pin
 *  not super intuitive, it sets it to 0 by default as a 1 is the expected
 *  default for the pull-down pins
 *  @pinNumber: the pin number on the board that will be read
 *  @Return: returns an int (0 or 1) acting as a bool for the pin
 *
 */
uint8_t checkPinIn(uint8_t pinNumber);

/**
 *  @configureOut
 *  sets the DDRx port high to use PORTx as output high/low
 *  @pinNumber: the pin number on the board that will be set to output mode
 *
 */
void configureOut(uint8_t pinNumber);

/**
 *  @configureIn
 *  sets the DDRx port low and PORTx to determing input behaviour
 *  @pinNumber: the pin number on the board that will be set to input mode
 *  @setting:   1 = pull-down, 0 = tri-state (pull-up)
 *              currently only pull-down seems to work
 *
 */
void configureIn(uint8_t pinNumber, uint8_t setting);

void sanity(int loop);

/**
 *  @flashRegister
 *  Flash the contents of a given register using the onboard LED (pin 0)
 *  @regPointer: the address of the register you wish to flash [int*]
 *
 */
void flashRegister(volatile uint8_t *regPointer);

/**
 *  @matrixScan
 *  function that scans through the matrix changing what pins are low to enable
 *  @keyMatrix: pointer to the matrix
 *              uses in_pin so that there are less bytes to send (5 < 6)
 *
 */
void matrixScan(uint8_t *keyMatrix);

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
void outputRegister(volatile uint8_t *regPointer, int pinNumber);

/**
 *  @print8Bit
 *  @val:       the 8 bit value being printed
 *  @pinNumber: the pin number on the board that will be read
 *
 */
void print8Bit(uint8_t val, int pinNumber);

/**
 *  @stepButton
 *  an infinite loop that ends when a button is pressed
 *  intented for use with outputRegister() to allow for easier reading of
 *  registers
 *  @pinNumber: the pin number on the board that will be read
 *
 */
void stepButton(uint8_t pinNumber);

#endif // HELPER_H
