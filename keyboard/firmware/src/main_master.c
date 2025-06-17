/*
 * demo_master.c
 *
 */

#include "helper.h"
#include "i2c.h"
#include "usb.h"
#include <stdint.h>

/**
 *  @initOutputPins
 *  calls the functions that configures output pins
 *
 */
void initOutputPins() {
  configureOut(0);
  configureOut(1);

  configureOut(4);
  configureOut(5);
  configureOut(6);
  configureOut(7);
  configureOut(8);
  configureOut(9);

  setOutPin(1, 1);

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

/**
 *  @combine
 *  function that scans through the matrix changing what pins are low to enable
 *  @keyMatrix: pointer to the matrix
 *              uses in_pin so that there are less bytes to send (5 < 6)
 *
 */
void combine(uint8_t *keyMatrixMaster, uint8_t *keyMatrixSlave,
             uint8_t *keyMatrix) {
    for (uint8_t i = 0; i < (NUM_IN_PIN); i++) {
        keyMatrix[i] = keyMatrixMaster[i];
        }
    for (uint8_t i = 0; i < (NUM_IN_PIN); i++) {
            keyMatrix[i+5] = keyMatrixSlave[i];
          }
}

int main(void) {

  initOutputPins();
  initInputPins();

  configureUSBInterface();
  configureEndpointZero();

  i2cInitMaster();

  __no_op();

  uint8_t keyMatrixMaster[NUM_IN_PIN];
  uint8_t keyMatrixSlave[NUM_IN_PIN];
  uint8_t keyMatrix[NUM_IN_PIN * 2];

  uint8_t new_keyboard_pressed_keys[6] = {0, 0, 0, 0, 0, 0};
  uint8_t new_keyboard_modifier = 0;

  // Is the current HID report empty (0) or non-empty (not 0)
  uint8_t current_status = 0;

  // Is the newly scanned HID report empty (0) or non-empty (not 0)
  uint8_t zero_status = 0;

  for (uint8_t i = 0; i < NUM_IN_PIN; i++) {
    // Fills matrix with values 0x01 -> 0x05
    keyMatrixMaster[i] = 0x00;
  }

  for (uint8_t i = 0; i < NUM_IN_PIN; i++) {
    // Fills matrix with values 0x01 -> 0x05
    keyMatrixSlave[i] = 0x00;
  }

  for (uint8_t i = 0; i < NUM_IN_PIN * 2; i++) {
    // Fills matrix with values 0x01 -> 0x05
    keyMatrix[i] = 0x00;
  }

  while (1) {

    // Scan master's matrix then receive slave's matrix
    matrixScan(keyMatrixMaster);
    i2cMasterRecieve(SLAVE_ADDRESS, keyMatrixSlave, NUM_IN_PIN);

    // Combine master's matrix and slave's matrix
    combine(keyMatrixMaster, keyMatrixSlave, keyMatrix);

    // Deal with sending any macros first, then move onto regular keypresses
    send_macros(keyMatrix, new_keyboard_pressed_keys,
        &new_keyboard_modifier);

    // Encode contents of combined matrix into USB-formmated modifier and
    // keypresses
    encode_keypresses(keyMatrix, new_keyboard_pressed_keys,
                      &new_keyboard_modifier);

    // Check if new report is empty
    zero_status = new_keyboard_modifier;
    for (int i = 0; i < 6; i++) {
      zero_status += new_keyboard_pressed_keys[i];
    }

    if (!zero_status && !current_status) { // Both empty, do nothing
      continue;

    } else if (zero_status && !current_status) { // New keypresses (must send)
      for (int i = 0; i < 6; i++) {
        keyboard_pressed_keys[i] = new_keyboard_pressed_keys[i];
      }
      keyboard_modifier = new_keyboard_modifier;
      usb_send_keypress();
      current_status = 1;
      continue;

    } else if (!zero_status &&
               current_status) { // No longer any keypresses (must send blank)
      for (int i = 0; i < 6; i++) {
        keyboard_pressed_keys[i] = 0x00;
      }
      keyboard_modifier = 0x00;
      usb_send_keypress();
      current_status = 0;
      continue;

    } else if (zero_status &&
               current_status) { // Replace keypresses (and check if held down)
      int identical = 1;
      for (int i = 0; i < 6; i++) {
        if (keyboard_pressed_keys[i] != new_keyboard_pressed_keys[i]) {
          identical = 0;
        }
      }
      if (identical) {
        continue;
      } else {
        for (int i = 0; i < 6; i++) {
          keyboard_pressed_keys[i] = new_keyboard_pressed_keys[i];
        }
        keyboard_modifier = new_keyboard_modifier;
        usb_send_keypress();
        current_status = 1;
        continue;
      }
    }
  }
}
