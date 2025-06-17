/**
 * usb.c
 * Implementation of usb.h functions
*/

#include "usb.h"

// Global array to hold current HID report and modifier value.
volatile uint8_t keyboard_pressed_keys[6] = {0, 0, 0, 0, 0, 0};
volatile uint8_t keyboard_modifier = 0;

// Key mapping
// In pins are rows, out pins are columns
// First 5 rows are master, second 5 rows are slave
// const uint8_t mapping[2*NUM_IN_PIN][NUM_OUT_PIN] = 
// {   {0x04, 0x05, 0x04, 0x05, 0x04, 0x05}, {0x04, 0x05, 0x04, 0x05, 0x04, 0x05}, {0x04, 0x05, 0x04, 0x05, 0x04, 0x05}, {0x04, 0x05, 0x04, 0x05, 0x04, 0x05}, {0x04, 0x05, 0x04, 0x05, 0x04, 0x05}, 
//     {0x04, 0x05, 0x04, 0x05, 0x04, 0x05}, {0x04, 0x05, 0x04, 0x05, 0x04, 0x05}, {0x04, 0x05, 0x04, 0x05, 0x04, 0x05}, {0x04, 0x05, 0x04, 0x05, 0x04, 0x05}, {0x04, 0x05, 0x04, 0x05, 0x04, 0x05}};

// PLL / Clock
volatile unsigned int *CLKSEL0 = (unsigned int *)(0x00C5);
unsigned int EXTE = 2;
unsigned int CLKS = 0;

volatile unsigned int *CLKSTA = (unsigned int *)(0x00C7);

volatile unsigned int *PLLCSR = (unsigned int *)(0x0049);
unsigned int PINDIV = 4;
unsigned int PLLE = 1;
unsigned int PLOCK = 0;

volatile unsigned int *PLLFRQ = (unsigned int *)(0x52);
unsigned int PINMUX = 7;
unsigned int PLLUSB = 6;
unsigned int PDIV0 = 0;
unsigned int PDIV1 = 1;
unsigned int PDIV2 = 2;
unsigned int PDIV3 = 3;

volatile unsigned int *CLKPR = (unsigned int *)(0x61);
unsigned int CLKPCE = 7;

// USB Endpoint
volatile unsigned int *UESTA0X = (unsigned int *)(0x00EE);

volatile unsigned int *UERST = (unsigned int *)(0x00EA);

volatile unsigned int *UENUM = (unsigned int *)(0x00E9);

volatile unsigned int *UECONX = (unsigned int *)(0x00EB);
unsigned int STALLRQ = 5;

volatile unsigned int *UECFG0X = (unsigned int *)(0x00EC);
unsigned int EPDIR = 0;
unsigned int EPTYPE0 = 6;
unsigned int EPTYPE1 = 7;

volatile unsigned int *UECFG1X = (unsigned int *)(0x00ED);
unsigned int EPSIZE0 = 4;
unsigned int EPSIZE1 = 5;
unsigned int EPSIZE2 = 6;
unsigned int EPBK0 = 2;
unsigned int EPBK1 = 3;
unsigned int ALLOC = 1;

volatile unsigned int *UEIENX = (unsigned int *)(0x00F0);
unsigned int RXSTPE = 3;
unsigned int TXINE = 0;
unsigned int RXOUTE = 2;

volatile unsigned int *UEINT = (unsigned int *)(0x00F4);

volatile unsigned int *UEINTX = (unsigned int *)(0x00E8);
unsigned int RXSTPI = 3;
unsigned int TXINI = 0;
unsigned int RXOUTI = 2;
unsigned int RWAL = 5;

volatile unsigned int *UEDATX = (unsigned int *)(0x00F1);

// USB Interface
volatile unsigned int *UHWCON = (unsigned int *)(0x00D7);
unsigned int UVREGE = 0;

volatile unsigned int *USBCON = (unsigned int *)(0x00D8);
unsigned int USBE = 7;
unsigned int FRZCLK = 5;
unsigned int OTGPADE = 4;
unsigned int VBUSTE = 0;

volatile unsigned int *USBSTA = (unsigned int *)(0x00D9);
unsigned int VBUS = 0;

volatile unsigned int *USBINT = (unsigned int *)(0x00DA);
unsigned int VBUSTI = 0;

volatile unsigned int *UDCON = (unsigned int *)(0x00E0);
unsigned int RSTCPU = 3;
unsigned int LSM = 2;
unsigned int RMWKUP = 1;
unsigned int DETACH = 0;

volatile unsigned int *UDIEN = (unsigned int *)(0x00E2);
unsigned int EORSTE = 3;
unsigned int SOFE = 2;

volatile unsigned int *UDINT = (unsigned int *)(0x00E1);
unsigned int EORSTI = 3;
unsigned int SOFI = 2;

volatile unsigned int *UDADDR = (unsigned int *)(0x00E3);
unsigned int ADDEN = 7;

void configureUSBInterface() {
  *UHWCON |= (1 << UVREGE); // Enable controller's voltage regulator

  *USBCON |= (1 << USBE);    // Enable USB controller
  *UHWCON |= (1 << OTGPADE); // Enable VBUS pad
  *USBCON |= (1 << FRZCLK);  // Freeze USB clock

  *USBCON &= ~(1 << FRZCLK); // Toggle FRZCLK to get WAKEUP IRQ
  *USBCON |= (1 << FRZCLK);

  *PLLCSR = 0x12;
  while (!(*PLLCSR & (1 << PLOCK)));

  *USBCON &= ~(1 << FRZCLK); // Leave power saving mode / unfreeze usb clock

  *UDCON = 0; // "Attach" which is really un-detach
  *UDIEN |=
      (1 << EORSTE); // Enable end-of-reset interrupts. On receipt of interrupt,
                     // sent to usb general interrupt vector and must clear,
                     // then setup endpoint and enable RXSTPE (received setup)
                     // interrupts for ep0 to process setup packets from host.

  sei(); // Enables interrupts by setting the global interrupt mask (from
         // interrupt.h) (cli() clears it)
}

void configureEndpointZero(){
    // This endpoint actually gets configured by default (with values seen below), but this code could be used to configure other endpoints

  *UENUM = 0; // Select USB endpoint

  *UECONX = 1; // Set EPEN to enable endpoint
  *UECFG0X = *UECFG0X &
             (~(1 << EPTYPE0) & ~(1 << EPTYPE1) &
              ~(1 << EPDIR)); // Set type (00xxxxxx) and direction (xxxxxxx0)
  *UECFG1X =
      *UECFG1X &
      (~(1 << EPSIZE0) & ~(1 << EPSIZE1) & ~(1 << EPSIZE2) & ~(1 << EPBK0) &
       ~(1 << EPBK1)); // Set size (x000xxxx) and number of banks (xxxx00xx)
  *UECFG1X |= (1 << ALLOC); // Allocate memory for endpoint (xxxxxx1x)
}

void usb_send_descriptor(uint8_t descriptor[], uint8_t desc_bytes) { // See page 275 for rough outline of algorithm
  for (uint16_t i = 1; i <= desc_bytes; i++) {
    if (*UEINTX & (1 << RXOUTI)) { // We've received a new OUT packet from host,
                                   // stop sending data because he wants to stop
      return;
    }

    *UEDATX = pgm_read_byte(&descriptor[i - 1]);  // write one byte
    if ((i % 8) == 0) {         // FIFO is only 8 bytes wide
                                // FIFO is full, send data
      *UEINTX &= ~(1 << TXINI); // Send IN packet //here
      while (!(*UEINTX & ((1 << RXOUTI) | (1 << TXINI))))
        ; // Wait for ACK (either received OUT packet [RXOUTI] or ready to send
          // new data [TXINI])
    }
  }

  if (!(*UEINTX &
        (1 << RXOUTI))) { // Haven't received final ACK, so must not have sent
                          // remaining data in almost-full bank
    *UEINTX &= ~(1 << TXINI); // Send IN package 
    while (!(*UEINTX & (1 << RXOUTI)))
      ; // Wait for (ZLP) ACK from host 
  }
  // Handshake to acknowledge IRQ 
  *UEINTX &= ~(1 << RXOUTI); 
}

void usb_ep0_setup() {
  // When we receive data, we will read one byte at a time from data register
  // into these variables
  uint8_t bmRequestType;
  uint8_t bRequest;
  uint8_t wValue_l;
  uint8_t wValue_h;
  uint8_t wIndex_l;
  uint8_t wIndex_h;
  uint8_t wLength_l;
  uint8_t wLength_h;
  uint8_t des_bytes;
  uint16_t length;

  // Now read the packet received from fifo into variables
  bmRequestType = *UEDATX;
  bRequest = *UEDATX;
  wValue_l = *UEDATX;
  wValue_h = *UEDATX;
  wIndex_l = *UEDATX;
  wIndex_h = *UEDATX;
  wLength_l = *UEDATX;
  wLength_h = *UEDATX;
  length = wLength_l + (wLength_h << 8);

  // ACK receipt by clearing setup packet interrupt flag in endpoint interrupt
  // register
  *UEINTX &= ~(1 << RXSTPI);

  if ((bmRequestType & 0x60) == 0) {
    // Standard Device Request

    switch (bRequest) {
    case 0x00:     // GET_STATUS/
      *UEDATX = 0; // Send back 16 Bit for status (Not self powered, nowakeup, not halted)
      *UEDATX = 0;
      *UEINTX &= ~(1 << TXINI); // Send data (ACK) and clear FIFO
      while (!(*UEINTX & (1 << RXOUTI)));            // wait for ZLP packet from host (ack)
      *UEINTX &= ~(1 << RXOUTI); // clear interrupt
      break;
    case 0x05:                     // SET_ADDRESS (procedure laid out in datasheet)
      *UDADDR = (wValue_l & 0x7F); // Save assigned address at UADD  (ADDEN = 0 still)
      *UEINTX &= ~(1 << TXINI);    // Send OUT packet (ZLP) / ACK and clear bank
      while (!(*UEINTX & (1 << TXINI)))
        ;                      // wait for bank to clear
      *UDADDR |= (1 << ADDEN); //enable address
      break;
    case 0x06: {
      // GET_DESCRIPTOR
      switch (wValue_h) {
      case 1: // Device Descriptor
        des_bytes = pgm_read_byte(&dev_des[0]);
        usb_send_descriptor((uint8_t *)dev_des, des_bytes);
        break;
      case 2: // Configuration Descriptor
        des_bytes = wLength_l;
        if (wLength_h || (wLength_l > wTotalLength) || (wLength_l == 0))
          des_bytes = wTotalLength;
        usb_send_descriptor((uint8_t *)conf_des, des_bytes);
        break;
      case 3: // A specific string descriptor
        switch (wValue_l) {
        case Lang_idx:
          des_bytes = pgm_read_byte(&lang_des[0]);
          usb_send_descriptor((uint8_t *)lang_des, des_bytes);
          break;
        case Manu_idx:
          des_bytes = pgm_read_byte(&manu_des[0]);
          usb_send_descriptor((uint8_t *)manu_des, des_bytes);
          break;
        case Prod_idx:
          des_bytes = pgm_read_byte(&prod_des[0]);
          usb_send_descriptor((uint8_t *)prod_des, des_bytes);
          break;
        case Seri_idx:
          des_bytes = pgm_read_byte(&seri_des[0]);
          usb_send_descriptor((uint8_t *)seri_des, des_bytes);
          break;
        default:
          break;
        }
        break;
      case 0x21: // HID Device descriptor (18 bytes offset inside of
                 // configuration descriptor)
        des_bytes = pgm_read_byte(&conf_des[18]);
        usb_send_descriptor((uint8_t *)&conf_des[18], des_bytes);
        break;
      case 0x22: // HID report descriptor (HID_report_descriptor)
        des_bytes = HID_report_descriptor_len;
        usb_send_descriptor((uint8_t *)HID_report_descriptor, des_bytes);
        break;

      default:
        break;
      }
    } break;

            case 0x09: // SET_CONFIGURATION
                *UENUM = keyboard_ep; // Select USB endpoint

                *UECONX = 1; // Set EPEN to enable endpoint
                *UECFG0X = 0xC1; // Set type (11xxxxxx) and direction (xxxxxxx1) 
                *UECFG1X = 0x04; // Set size (x000xxxx) and number of banks (xxxx01xx)
                *UECFG1X |= (1 << ALLOC); // Allocate memory for endpoint (xxxxxx1x)

                *UENUM = 0; // select ep0

                *UEINTX &= ~(1<< TXINI); // send ZLP (ack)
                while (!(*UEINTX & (1<<TXINI))); // wait for ZLP back (ack)
                break;

    default:
      // Should set stall request here (STALRQ)
      *UECONX |= (1 << STALLRQ);
      break;
    }
  } else {
    *UECONX |= (1 << STALLRQ);
  }
}

void encode(char message[], uint8_t chars[], uint8_t modifiers[], int* length){
    // Pass message of length <orig_length>. 
    // Also pass chars and modifiers arrays of same length which will contain chars and modifier bytes. 
    // Length is changed to reflect new length
    
    char special_buffer[21]; // Max length permitted for "special" length, used elsewhere (really it's 20, but add one space for the null terminator I'll need elsewhere)
    uint8_t special_buffer_length = 0; 
    uint8_t new_length = 0; // New length temp variable
    uint8_t special_modifier = 0x00; // used to apply special keys to all entries within tag (<Shift> ... </Shift>)
    // Cases: 
    //  Uppercase Letter
    //  Lowercase Letter
    //  Number
    //  Symbol (shift + key)
    //  Special (<HOME>, <INSERT>, etc.) or Modifier (<Shift> ... </Shift>)

    // "abc<Shift>a</Shift>c"
    
    for (int i = 0; i < *length; i++){
        modifiers[new_length] = special_modifier;

        // Case: upper- or lower-case letters and numbers
        if(message[i] >= 'A' && message[i] <= 'Z'){ // A-Z
            message[i] += 32; // .lower()
            modifiers[new_length] |= (1 << 1); // left shift
        }
        if(message[i] >= 'a' && message[i] <= 'z'){ // a-z
            chars[new_length] = message[i]-0x5D; // Substract 0x61 to get from ASCII code to (# in the alphabet), then add 0x04 to get into letter range in USB table
            new_length += 1;
            continue;
        }
        if(message[i] >= '1' && message[i] <= '9'){ // 1-9
            chars[new_length] = message[i]-0x13; // Substract 0x31 to remove ASCII code, then add 0x1E to get into USB number range
            new_length += 1;
            continue;
        }
        if(message[i] == '0') { // 0
            chars[new_length] = 0x27; 
            new_length += 1;
            continue;}
        
        // Case: Symbol (special doubles)
        if (message[i] == '-') {chars[new_length] = 0x2D; new_length += 1; continue;}
        if (message[i] == '_') {chars[new_length] = 0x2D; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == '=') {chars[new_length] = 0x2E; new_length += 1; continue;}
        if (message[i] == '+') {chars[new_length] = 0x2E; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == '[') {chars[new_length] = 0x2F; new_length += 1; continue;}
        if (message[i] == '{') {chars[new_length] = 0x2F; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == ']') {chars[new_length] = 0x30; new_length += 1; continue;}
        if (message[i] == '}') {chars[new_length] = 0x30; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == '\\') {chars[new_length] = 0x31; new_length += 1; continue;}
        if (message[i] == '|') {chars[new_length] = 0x31; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == '#') {chars[new_length] = 0x32; new_length += 1; continue;}
        if (message[i] == '~') {chars[new_length] = 0x32; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == ';') {chars[new_length] = 0x33; new_length += 1; continue;}
        if (message[i] == ':') {chars[new_length] = 0x33; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == '\'') {chars[new_length] = 0x34; new_length += 1; continue;}
        if (message[i] == '\"') {chars[new_length] = 0x34; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == '`') {chars[new_length] = 0x35; new_length += 1; continue;}
        if (message[i] == '~') {chars[new_length] = 0x35; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == ',') {chars[new_length] = 0x36; new_length += 1; continue;}
        // if (message[i] == '<') {chars[new_length] = 0x36; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == '.') {chars[new_length] = 0x37; new_length += 1; continue;}
        if (message[i] == '>') {chars[new_length] = 0x37; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == '/') {chars[new_length] = 0x38; new_length += 1; continue;}
        if (message[i] == '?') {chars[new_length] = 0x38; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == '!') {chars[new_length] = 0x1E; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == '@') {chars[new_length] = 0x1F; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == '#') {chars[new_length] = 0x20; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == '$') {chars[new_length] = 0x21; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == '%') {chars[new_length] = 0x22; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == '^') {chars[new_length] = 0x23; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == '&') {chars[new_length] = 0x24; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == '*') {chars[new_length] = 0x25; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == '(') {chars[new_length] = 0x26; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}
        if (message[i] == ')') {chars[new_length] = 0x27; modifiers[new_length] |= (1 << 1); new_length += 1; continue;}

        // Case: Special and modifiers
        // When we get to a special, we will enter a for loop which picks up at next char and continues until we reach '>'.
        // Then we will adjust the *length variable of the for loop to reflect the now shorter message 
        if (message[i] == '<') {
            for (int j = 1;;j++){
                if(message[i+j] == '>'){special_buffer_length = j-1; break;} // We've reached end of tag
                special_buffer[j-1] = message[i+j];
                if(j > 20){special_buffer_length = -1; break;} //Tag is too long, must not be a special just a regular '<'
            }
            // We have special buffer and length after the above loop

            // If it was just a regular '<'
            if (special_buffer_length == -1){
                chars[new_length] = 0x36; // '<'
                modifiers[new_length] |= (1 << 1); // LShift
                new_length += 1; 
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}// clear array, probably not strictly necessary
                continue;
            }

            special_buffer[special_buffer_length] = '\0'; // add null termination 
            
            if (strcmp("LCtrl", special_buffer) == 0) {
                special_modifier |= (1 << 0); // LCtrl
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 4; // move past '<', "LCtrl", and '>'
                continue;
            } else if (strcmp("/LCtrl", special_buffer) == 0) {
                special_modifier &= ~(1 << 0); // un-LCtrl
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 5; // move past '<', '/', "LCtrl", and '>'
                continue;
            } else if (strcmp("LShift", special_buffer) == 0) {
                special_modifier |= (1 << 1); // LShift
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 5; // move past '<', "Lshift", and '>'
                continue;
            } else if (strcmp("/LShift", special_buffer) == 0) {
                special_modifier &= ~(1 << 1); // un-LShift
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 6; // move past '<', '/', "LShift", and '>'
                continue;
            } else if (strcmp("LAlt", special_buffer) == 0) {
                special_modifier |= (1 << 2); // LAlt
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 3; // move past '<', "LAlt", and '>'
                continue;
            } else if (strcmp("/LAlt", special_buffer) == 0) {
                special_modifier &= ~(1 << 2); // un-LAlt
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 4; // move past '<', '/', "LAlt", and '>'
                continue;
            } else if (strcmp("LWin", special_buffer) == 0) {
                special_modifier |= (1 << 3); // LWin
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 3; // move past '<', "LWin", and '>'
                continue;
            } else if (strcmp("/LWin", special_buffer) == 0) {
                special_modifier &= ~(1 << 3); // un-LWin
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 4; // move past '<', '/', "LWin", and '>'
                continue;
            } else if (strcmp("RCtrl", special_buffer) == 0) {
                special_modifier |= (1 << 4); // RCtrl
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 4; // move past '<', "RCtrl", and '>'
                continue;
            } else if (strcmp("/RCtrl", special_buffer) == 0) {
                special_modifier &= ~(1 << 4); // un-RCtrl
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 5; // move past '<', '/', "RCtrl", and '>'
                continue;
            } else if (strcmp("RShift", special_buffer) == 0) {
                special_modifier |= (1 << 5); // RShift
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 5; // move past '<', "Rshift", and '>'
                continue;
            } else if (strcmp("/RShift", special_buffer) == 0) {
                special_modifier &= ~(1 << 5); // un-RShift
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 6; // move past '<', '/', "RShift", and '>'
                continue;
            } else if (strcmp("RAlt", special_buffer) == 0) {
                special_modifier |= (1 << 6); // RAlt
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 3; // move past '<', "RAlt", and '>'
                continue;
            } else if (strcmp("/RAlt", special_buffer) == 0) {
                special_modifier &= ~(1 << 6); // un-RAlt
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 4; // move past '<', '/', "RAlt", and '>'
                continue;
            } else if (strcmp("RWin", special_buffer) == 0) {
                special_modifier |= (1 << 7); // RWin
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 3; // move past '<', "RWin", and '>'
                continue;
            } else if (strcmp("/RWin", special_buffer) == 0) {
                special_modifier &= ~(1 << 7); // un-RWin
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 4; // move past '<', '/', "RWin", and '>'
                continue;

            } else if (strcmp("Home", special_buffer) == 0) {
                chars[new_length] = 0x4A; 
                new_length += 1; 
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 3; // move past '<', "home", and '>'
                continue;
            } else if (strcmp("Caps Lock", special_buffer) == 0) {
                chars[new_length] = 0x39;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 8;
                continue;
            } else if (strcmp("Enter", special_buffer) == 0) {
                chars[new_length] = 0x28;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 4;
                continue;
            } else if (strcmp("Escape", special_buffer) == 0) {
                chars[new_length] = 0x29;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 5;
                continue;
            } else if (strcmp("Backspace", special_buffer) == 0) {
                chars[new_length] = 0x2A;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 8;
                continue;
            } else if (strcmp("Tab", special_buffer) == 0) {
                chars[new_length] = 0x2B;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 2;
                continue;
            } else if (strcmp("Spacebar", special_buffer) == 0) {
                chars[new_length] = 0x2C;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 7;
                continue;
            } else if (strcmp("PrintScr", special_buffer) == 0) {
                chars[new_length] = 0x46;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 7;
                continue;
            } else if (strcmp("ScrollLck", special_buffer) == 0) {
                chars[new_length] = 0x47;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 8;
                continue;
            } else if (strcmp("Pause", special_buffer) == 0) {
                chars[new_length] = 0x48;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 4;
                continue;
            } else if (strcmp("Insert", special_buffer) == 0) {
                chars[new_length] = 0x49;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 5;
                continue;
            } else if (strcmp("Delete", special_buffer) == 0) {
                chars[new_length] = 0x4C;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 5;
                continue;
            } else if (strcmp("RightArrow", special_buffer) == 0) {
                chars[new_length] = 0x4F;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 9;
                continue;
            } else if (strcmp("LeftArrow", special_buffer) == 0) {
                chars[new_length] = 0x50;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 8;
                continue;
            } else if (strcmp("DownArrow", special_buffer) == 0) {
                chars[new_length] = 0x51;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 8;
                continue;
            } else if (strcmp("UpArrow", special_buffer) == 0) {
                chars[new_length] = 0x52;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                i += 2 + 7;
                continue;
            } else if (strcmp("PageUp", special_buffer) == 0) {
                chars[new_length] = 0x4B;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 5;
                continue;
            } else if (strcmp("End", special_buffer) == 0) {
                chars[new_length] = 0x4D;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 2; 
                continue;
            } else if (strcmp("PageDown", special_buffer) == 0) {
                chars[new_length] = 0x4E;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 7; 
                continue;
            } else if (strcmp("NumLck", special_buffer) == 0) {
                chars[new_length] = 0x53;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 5; 
                continue;
            } else if (strcmp("KeypadEnter", special_buffer) == 0) {
                chars[new_length] = 0x58;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 10; 
                continue;
            } else if (strcmp("F1", special_buffer) == 0) {
                chars[new_length] = 0x3A;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 1; 
                continue;
            } else if (strcmp("F2", special_buffer) == 0) {
                chars[new_length] = 0x3B;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 1;
                continue;
            } else if (strcmp("F3", special_buffer) == 0) {
                chars[new_length] = 0x3C;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 1;
                continue;
            } else if (strcmp("F4", special_buffer) == 0) {
                chars[new_length] = 0x3D;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 1;
                continue;
            } else if (strcmp("F5", special_buffer) == 0) {
                chars[new_length] = 0x3E;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 1;
                continue;
            } else if (strcmp("F6", special_buffer) == 0) {
                chars[new_length] = 0x3F;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 1;
                continue;
            } else if (strcmp("F7", special_buffer) == 0) {
                chars[new_length] = 0x40;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 1;
                continue;
            } else if (strcmp("F8", special_buffer) == 0) {
                chars[new_length] = 0x41;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 1;
                continue;
            } else if (strcmp("F9", special_buffer) == 0) {
                chars[new_length] = 0x42;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 1;
                continue;
            } else if (strcmp("F10", special_buffer) == 0) {
                chars[new_length] = 0x43;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 2;
                continue;
            } else if (strcmp("F11", special_buffer) == 0) {
                chars[new_length] = 0x44;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 2;
                continue;
            } else if (strcmp("F12", special_buffer) == 0) {
                chars[new_length] = 0x45;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 2;
                continue;
            } else if (strcmp("F13", special_buffer) == 0) {
                chars[new_length] = 0x68;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 2;
                continue;
            } else if (strcmp("F14", special_buffer) == 0) {
                chars[new_length] = 0x69;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 2;
                continue;
            } else if (strcmp("F15", special_buffer) == 0) {
                chars[new_length] = 0x6A;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 2;
                continue;
            } else if (strcmp("F16", special_buffer) == 0) {
                chars[new_length] = 0x6B;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 2;
                continue;
            } else if (strcmp("F17", special_buffer) == 0) {
                chars[new_length] = 0x6D;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 2; // "F17" (3 chars - 1)
                continue;
            } else if (strcmp("F18", special_buffer) == 0) {
                chars[new_length] = 0x6E;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 2;
                continue;
            } else if (strcmp("F19", special_buffer) == 0) {
                chars[new_length] = 0x6F;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 2;
                continue;
            } else if (strcmp("F20", special_buffer) == 0) {
                chars[new_length] = 0x70;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 2;
                continue;
            } else if (strcmp("F21", special_buffer) == 0) {
                chars[new_length] = 0x71;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 2;
                continue;
            } else if (strcmp("F22", special_buffer) == 0) {
                chars[new_length] = 0x72;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 2;
                continue;
            } else if (strcmp("F23", special_buffer) == 0) {
                chars[new_length] = 0x73;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 2;
                continue;
            } else if (strcmp("F24", special_buffer) == 0) {
                chars[new_length] = 0x74;
                new_length += 1;
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) { special_buffer[x] = 0; }
                i += 2 + 2;
                continue;
            } else {
                chars[new_length] = 0x36; // '<'
                modifiers[new_length] |= (1 << 1); // LShift
                new_length += 1; 
                special_buffer_length = 0;
                for(int x = 0; x < 21; x++) {special_buffer[x] = 0;}
                continue;
            }
        }
    
    }
    *length = new_length;
    return;
}

void send_message(char* input){
    char message[100]; // Raw macro text with modifier tags still present
    int length = 0;
    uint8_t new_message[100];
    uint8_t new_message_modifiers[100];

    // Calculate length
    while(input[length] != '\0'){
        message[length] = input[length];
        length++;
    }
    
    encode(message, new_message, new_message_modifiers, &length);

    for (int i = 0; i < length; i++){
        keyboard_modifier = new_message_modifiers[i];
        keyboard_pressed_keys[0] = new_message[i];
        usb_send_keypress();
        delay(4000);
        keyboard_modifier = 0x00;
        keyboard_pressed_keys[0] = 0x00;
        usb_send_keypress();
        delay(4000);
    }

    return;
}

void send_int(uint8_t num) {
    static char str[12];  // Enough space for a 32-bit integer and '!' termination
    int i = 0;
    if (num == 0) {
        str[i++] = '0';
    } else {
        while (num > 0) {
            str[i++] = (num % 10) + '0';
            num /= 10;
        }
    }
    str[i] = '!'; 
    for (int j = 0, k = i - 1; j < k; j++, k--) { // Reverse order
        char temp = str[j];
        str[j] = str[k];
        str[k] = temp;
    }
    send_message(str);

    return;
}

void usb_send_keypress(){
    *UENUM = keyboard_ep;
    if (*UEINTX & (1 << RWAL)) { // Check if banks are writable
        *UEDATX = keyboard_modifier;
        *UEDATX = 0; // write out reserved, empty byte between modifier and 6 keys
        for (int j = 0; j < 6; j++) {
            *UEDATX = keyboard_pressed_keys[j];
        }
        *UEINTX &= ~((0x80) | (1 << TXINI)); // Send packet 
    }
    return;
}

void encode_keypresses(uint8_t* combinedMatrix, uint8_t* pressed_keys, uint8_t* modifier){
    int numPressed = 0;

    *modifier = 0x00;
    for(int i = 0; i < 6; i++){
        pressed_keys[i] = 0x00;
    }

    for(int inPin = 0; inPin < 2*NUM_IN_PIN; inPin++){
        for(int outPin = 0; outPin < NUM_OUT_PIN; outPin++){
            if((combinedMatrix[inPin] >> outPin) & 0x1){

                uint8_t keypress = mapping[inPin][outPin];
                numPressed++;

                if(numPressed > 6) { // Too many keys pressed, error out (send 6x "keyboard_pressed_keys" values and 0x0 modifier)
                    *modifier = 0;
                    for (int i = 0; i < 6; i++){
                        pressed_keys[i] = 0x01;
                    }
                    return;
                }

                if (keypress > 0xDD){ // Modifier key
                    if(keypress == 0xE0){*modifier |= (1 << 0); continue;} //Left Control
                    if(keypress == 0xE1){*modifier |= (1 << 1); continue;} //Left Shift
                    if(keypress == 0xE2){*modifier |= (1 << 2); continue;} //Left Alt
                    if(keypress == 0xE3){*modifier |= (1 << 3); continue;} //Left GUI (Windows / OSX's command-key)
                    if(keypress == 0xE4){*modifier |= (1 << 4); continue;} //Right Control
                    if(keypress == 0xE5){*modifier |= (1 << 5); continue;} //Right Shift
                    if(keypress == 0xE6){*modifier |= (1 << 6); continue;} //Right Alt
                    if(keypress == 0xE7){*modifier |= (1 << 7); continue;} //Right GUI (Windows / OSX's command-key)
                } else { // Normal key pressed
                    pressed_keys[numPressed-1] = keypress;
                }
            }
        }
    }
} 

void send_macros(uint8_t* combinedMatrix, uint8_t* pressed_keys, uint8_t* modifier){
    // Check if any macro keys are pressed
        // If not, return without modifying anything
        // If yes,
            // Record which macros must be sent by looking up mapping array
            // Set those macro keys as unpressed in the combinedMatrix
            // Save current pressed_keys and modifier 
            // Send each macro one after the other
            // Restore pressed_keys and modifier then return 
    uint8_t old_pressed_keys[6];
    uint8_t old_modifier;

    uint8_t macros_to_send[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Start out as all blanks

    uint8_t macro_count = 0;
    for(int inPin = 0; inPin < 2*NUM_IN_PIN; inPin++){
        for(int outPin = 0; outPin < NUM_OUT_PIN; outPin++){
            if((combinedMatrix[inPin] >> outPin) & 0x1){ // If key is pressed 
                uint8_t keypress = mapping[inPin][outPin]; // Get mapping
                if (keypress > 0xE7) { // Is this a macro?
                    combinedMatrix[inPin] &= ~(1 << outPin); // Un-set this key as pressed
                    macros_to_send[macro_count] = keypress; // Save this macro as one we need to send
                    macro_count += 1;
                } 
            }
        }
    }

    if (macro_count){ // We have some macros to send
        for (int i = 0; i < 6; i++){ // Save old pressed keys
            old_pressed_keys[i] = pressed_keys[i];
        }
        old_modifier = *modifier; // Save old modifier byte
        
        // Send macros
        for (int i = 0; i < macro_count; i++){
            send_message(macros[macros_to_send[i]-0xE8]); // Get char* to macro string to send
            delay(50000/strlen(macros[macros_to_send[i]-0xE8]));
        }

        // Restore pressed_keys and modifier
        for (int i = 0; i < 6; i++){ 
            pressed_keys[i] = old_pressed_keys[i];
        }
        *modifier = old_modifier; 
        return;
    } else {
        return;
    }
}

/**
 *  @USB_ENP_I@
 *  "USB Endpoint Interrupt request"
 *  Interrupt service routine for endpoint packets. 
 *  In practice, only need to respond to SETUP packets sent to endpoint 0
 * 
 */
ISR(USB_ENP_I){ 
    if (*UEINT & (1<<0)){ // We're talking about ep0?
        *UENUM = 0;
        if (*UEINTX & (1<<RXSTPI)) { // Setup packet recived, so lets reply to it appropriately
            usb_ep0_setup();
        } // endif ep0
    } else {
        // If we set up any other endpoints, we will add more here
    }
}

/**
 *  @USB_GEN_I@
 *  "USB General Interrupt request"
 *  Interrupt service routine for general USB interrupts. 
 *  In practice, only need to respond to SETUP packets sent to endpoint 0
 * 
 */
ISR(USB_GEN_I){ 
    if (*UDINT & (1<<EORSTI)) { // Was it an end-of-reset interrupt?
        *UDINT &= ~(1<<EORSTI); // Clear EOR interrupt
        configureEndpointZero(); // Reconfigure endpoint (though ep0 should always be configured)   
        *UENUM = 0; // Select USB endpoint
        *UEIENX |= (1<<RXSTPE); // Enable setup packet receipt interrupts to process any setup packets received
    }
}
