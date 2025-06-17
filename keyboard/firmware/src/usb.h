/**
 * usb.h
*/

#ifndef USB_H
#define USB_H

// Include register / bit defines
#include "helper.h"
#include "mapping.h"
#include "macros.h"
#include <string.h>

// Allows for placing of ISRs at correct addresses, specific to each device (32u4 here), using ISR(<VECTOR>) macro
#define _AVR_IO_H_ // Need this to use versions of register names and not those AVR libraries tries to define for 32u4
#include <avr/interrupt.h>

#include <stdint.h> // For uint8_t type definition
#include <avr/pgmspace.h> // For PROGMEM directive to place data in flash ROM due to limited RAM and read it using pgm_read_byte

// See page 63 of datasheet for number <-> name mapping, but offset by 1 because ISR 1 in datasheet (reset) is actually mapped to ISR 0
// Also see "iom32u4.h" and "sfr_defs.h"
#define USB_GEN_I __vector_10 // USB General Interrupt Request
#define USB_ENP_I __vector_11 // USB Endpoint/Pipe Interrupt Communication Request

// Indexes of descriptors used when host requests descriptors)
#define Lang_idx    0   // LanguageDescriptorIndex
#define Manu_idx    1   // ManufacturerStringIndex
#define Prod_idx    2   // ProductStringIndex
#define Seri_idx    3   // SerialNumberStringIndex
#define Intf_idx    2   // InterfaceStringIndex

// Number of endpoints we will provide descriptors for (just one for keyboard input, EP0 requires no descriptor aas it's default control EP)
#define Nr_eps 1

// EP# for keyboard (data) endpoint
#define keyboard_ep 0x1

// Length of report descriptor
# define HID_report_descriptor_len 63

// Length of configuration descriptor (a function of number of endpoints)
// 9 bytes for configuration descriptor, 9 bytes for HID descriptor and 9 bytes for interface descriptor (then 7 for each endpoint descriptor)
#define wTotalLength 9+9+9+(7*Nr_eps) 

// Temporarily being used, found in an example 
#define low(x)   ((x) & 0xFF)
#define high(x)  (((x)>>8) & 0xFF)

extern volatile uint8_t keyboard_pressed_keys[];
extern volatile uint8_t keyboard_modifier;

// PLL / Clock
extern volatile unsigned int *CLKSEL0; //Clock selection register (internal RC oscillator or external crystal)
extern unsigned int EXTE; //Enable external clock / oscillator
extern unsigned int CLKS; //Select external (1) or intenral (0) clock / oscillator 

extern volatile unsigned int *CLKSTA; //Clock status register (is RC oscillator or external crystal running?)

extern volatile unsigned int *PLLCSR; //PLL control and status register
extern unsigned int PINDIV; //PLL Input Prescaler enable bit (set 1 to divide by 2 and use 16mhz instead of 8mhz)
extern unsigned int PLLE; // PLL Enable
extern unsigned int PLOCK; // PLL Lock detector (set by hardware, clear with PLLE)

extern volatile unsigned int *PLLFRQ; // PLL Frequency Control Register
extern unsigned int PINMUX; // PLL Input Multiplexer
extern unsigned int PLLUSB; // PLL Post-scaler for USB Peripheral
extern unsigned int PDIV0; // PLL lock frequency (bit 0)
extern unsigned int PDIV1; // PLL lock frequency (bit 1)
extern unsigned int PDIV2; // PLL lock frequency (bit 2)
extern unsigned int PDIV3; // PLL lock frequency (bit 3)

extern volatile unsigned int *CLKPR; // Clock pre-scaler register
extern unsigned int CLKPCE; // Enable changes to pre-scaler register


// USB Endpoint
extern volatile unsigned int *UESTA0X; // USB endpoint status register

extern volatile unsigned int *UERST; // Endpoint reset register 

extern volatile unsigned int *UENUM; // USB endpoint selection register

extern volatile unsigned int *UECONX; // USB endpoint config register (has EPEN enable bit to enable end point selected with UENUM register)
extern unsigned int STALLRQ;

extern volatile unsigned int *UECFG0X; // USB endpoint type and direction configuration
extern unsigned int EPDIR;
extern unsigned int EPTYPE0;
extern unsigned int EPTYPE1;

extern volatile unsigned int *UECFG1X; // USB endpoint size, bank count and allocation configuration register
extern unsigned int EPSIZE0;
extern unsigned int EPSIZE1;
extern unsigned int EPSIZE2;
extern unsigned int EPBK0;
extern unsigned int EPBK1;
extern unsigned int ALLOC;

extern volatile unsigned int *UEIENX; // USB endpoint interrupt flag register
extern unsigned int RXSTPE; // Setup packet receipt interrupt
extern unsigned int TXINE; // Transmitter Ready Interrupt Enable Flag
extern unsigned int RXOUTE; // OUT packet received interrupt (includes ZLP)

extern volatile unsigned int *UEINT; // Indicates which endpoint triggered interrupt (bit 0 for ep0, bit 1 for ep1, etc. So read if == 1, == 2, == 4, etc.)

extern volatile unsigned int *UEINTX; // With UENUM set (to specify which endpoint we want to look at) indicates which interrupt source triggered IRQ
extern unsigned int RXSTPI; // REceived setup packet interrupt flag
extern unsigned int TXINI; // Transmitter Ready Interrupt Flag (set by hardware when endpoint bank is free to fill with data to send. Clearing indicates to usb controller to send contents)
extern unsigned int RXOUTI; // Received OUT Data Interrupt Flag (set by hardware when data is waiting in endpoint's fifo to be read. Clearing indicates to controller that we've read it)
extern unsigned int RWAL; // Can read from bank (for OUT type), or can write to bank (for IN type)

extern volatile unsigned int *UEDATX; // Endpoint FIFO data buffer for read/write. When you read one byte it'll load the next one from fifo


// USB Interface
extern volatile unsigned int *UHWCON;
extern unsigned int UVREGE; // Set to enable USB pad voltage regulator

extern volatile unsigned int *USBCON;
extern unsigned int USBE; // Set to enable USB controller
extern unsigned int FRZCLK; // Set to freeze USB clock
extern unsigned int OTGPADE; // OTGPADE / set to enable vbus pad
extern unsigned int VBUSTE; // Set to enable VBUS Transition Interrupts (might be needed for plug in/out interrupt generation?)

extern volatile unsigned int *USBSTA;
extern unsigned int VBUS; // VBUS flag (is VBUS powered or not (are we plugged in?))

extern volatile unsigned int *USBINT;
extern unsigned int VBUSTI; // Set high when VBUS transition occurs (plugged or unplugged), must be cleared by software

extern volatile unsigned int *UDCON; 
extern unsigned int RSTCPU; // Set to reset CPU on USB End of Reset signal (but leave controller attached to host)
extern unsigned int LSM; // Set for low-speed mode, otherwise high-speed
extern unsigned int RMWKUP; // idek
extern unsigned int DETACH; // Set to physically disconnect USB from host

extern volatile unsigned int *UDIEN; // USB-related interrupt enable register
extern unsigned int EORSTE; // End-of-reset interrupt enable bit
extern unsigned int SOFE; // Start-of-frame interrupt enable bit

extern volatile unsigned int *UDINT; // USB interrupt register
extern unsigned int EORSTI; // End-of-reset interrupt
extern unsigned int SOFI; // Start-of-frame interrupt


extern volatile unsigned int *UDADDR; // USB address register (we're assigned an address by the host and must place it in lower 7 bits here)
extern unsigned int ADDEN; // Set once address is loaded to start using this one

/**
 *  @configureUSBInterface@
 *  Configure PLL, initialize USB interface, and configure and enable interrupts 
 * 
 */
void configureUSBInterface();

/**
 *  @configureEndpointZero@
 *  Configure and enable endpoint 0 (default CONTROL endpoint) 
 * 
 */
void configureEndpointZero();

/**
 *  @usb_send_descriptor@
 *  Send given descriptor (from flash ROM) to host 
 *  @descriptor: Pointer to descriptor
 *  @desc_bytes: Length in bytes of descriptor
 *
 */
void usb_send_descriptor(uint8_t descriptor[] ,uint8_t desc_bytes);

/**
 *  @usb_ep0_setup@
 *  Respond appropriately to SETUP packets received by endpoint 0
 * 
 */
void usb_ep0_setup();

/**
 *  @encode@
 *  Encodes message of given length to USB-encoded sequence of bytes.
 *  Supports special tags in message (<LShift> ... </LShift> or <Home>).
 *  Used for debugging and sending macros.
 *  @message: Character array containing message.
 *  @chars: uint8_t array to place new message in
 *  @modifiers: uint8_t array to place modifiers for each of the keys in our new message
 *  @length: Length of message, is modified to return length of new message
 *
 */
void encode(char message[], uint8_t chars[], uint8_t modifiers[], int* length);

/**
 *  @send_message@
 *  Sends a "!"-terminated string (ex: "abc123!") over USB.
 *  Used for debugging.
 *  @input: Message to send, MUST BE TERMINATED IN "!", valid chars [A-Za-z0-9] and "-" for backspace.
 *
 */
void send_message(char* input);

/**
 *  @send_int@
 *  Sends an integer over USB.
 *  Used for debugging.
 *  @num: Number to send/type out.
 *
 */
void send_int(uint8_t num);
    
/**
 *  @usb_send_keypress@
 *  Sends current contents of keyboard_pressed_keys array and keyboard_modifier.
 *  @num: Number to send/type out.
 *
 */
void usb_send_keypress();

/**
 *  @encode_keypresses@
 *  Converts the combined keyboard matrix into a USB-ready modifier byte and 6x keypress bytes using the mapping array.
 *  Note that this function's results should be compared against the current modifier and keypress array to determine if the keyboard state has changed.
 *  NOTA BENE!!!: be sure to reset pressed_keys and modifier between encodings! 
 *  @combinedMatrix: Matrix representing combined states of slave and master keyboards. Shape should be [2*NUM_IN_PIN][NUM_OUT_PIN].
 *  @pressed_keys: Array that this function will modify to contain current 6x keypresses (USB-formatted bytes)
 *  @modifier: Byte that this function will modify to contain the modifier bits (USB-formatted byte, one bit for each modifier key (ex. LShift, RAlt, etc.))
 *
 */
void encode_keypresses(uint8_t* combinedMatrix, uint8_t* pressed_keys, uint8_t* modifier);

/**
 *  @send_macros@
 *  Extracts any macro keypresses from combined matrix, replacing them with a 0 before sending to encode_keypresses.
 *  If any macros are being pressed, this function will send those keypresses in order before returning and allowing encode_keypresses to send all regular ones.
 *  @combinedMatrix: Matrix representing combined states of slave and master keyboards. Shape should be [2*NUM_IN_PIN][NUM_OUT_PIN]. Will have macro keys removed.
 *  @pressed_keys: Array that this function will save, then use to send macros, then restore.
 *  @modifier: Byte that this function will save, then use to send macros, then restore.
 *
 */
void send_macros(uint8_t* combinedMatrix, uint8_t* pressed_keys, uint8_t* modifier);

// Device Descriptor
static const uint8_t dev_des[] PROGMEM = {
    18,   // bLength = 18 (0x12), descriptor length in bytest
    0x01, // bDescriptorType = 0x01, Descriptor ID = 1 -> Device descriptor
    0x10,     0x01, // bcdUSB = 0x0200, USB_Spec2_0
    0x00, // bDeviceClass = 0x00, class code defined on interface level
    0x00, // bDeviceSubClass = 0x00
    0x00, // bDeviceProtocoll = 0x00
    8,    // bMaxPacketSize0 = EP0FS, max. package size EP0 (here 8 bytes)
    0xDE,     0x08, // idVendor = 0x08DE , ??? or 0x03eb, Atmel
    0x20,     0x04, // idProduct = 0x0420, Produkt ID
    0x69,     0x69, // bcdDevice = 0x6969, Release number device
    Manu_idx, // iManufacturer = Index for string-descriptor manufacturer
    Prod_idx, // iProduct = Index for string-descriptor product
    Seri_idx, // iSerialNumber = Index for string-descriptor serial number
    0x01      // bNumConfigurations = 1, Number of available configurations
};

// Configuration Descriptor
static const uint8_t conf_des[] PROGMEM = {
    9,    // bLength = 0x09, descriptor length in bytes
    0x02, // bDescriptorType = 0x02, Descriptor ID = 2 -> Configuration
          // descriptor
    low(wTotalLength),
    high(wTotalLength), // wTotalLength, length of Configuration
    0x01,               // bNumInterfaces = 1
    0x01,               // bConfigurationValue = 1, must not be 0
    0,    // iConfiguration = 0, index for str.-descriptor configuration
    0x80, // bmAttributes = 0x80,bus-powered, no remote wakeup Bit 7=1
    250,  // MaxPower = 250(dezimal), means 250*2mA = 500mA

    // Interface Descriptor
    9,    // bLength = 0x09, length of descriptor in bytes
    0x04, // bDescriptorType = 0x04, descriptor ID = 4 -> Interface descriptor
    0,    // bInterfaceNumber = 0;
    0,    // bAlternateSetting = 0;
    Nr_eps, // bNumEndpoints = 1 (don't count ep0, it's default control EP and
            // known as such (no descriptor required))
    0x03,   // bInterfaceClass = 0x03, classcode: HID
    0x01, // bInterfaceSubClass = 0x01, subclasscode: boot report (6 key roll
          // over, BIOS keyboard)
    0x01, // bInterfaceProtocol = 0x01, protocoll code: keyboard
    Intf_idx, // iInterface = 0, Index for string descriptor interface

    // HID Class descriptor (sample in table e.4 of his specsheet)
    9,          // bLength
    0x21,       // bDescriptorType - 0x21 is HID
    0x11, 0x01, // bcdHID - HID Class Specification 1.11
    0,          // bCountryCode (see hid spec, above 6.2.2)
    1,          // bNumDescriptors - Number of HID descriptors
    0x22,       // bDescriptorType - Type of descriptor
    HID_report_descriptor_len,
    0, // wDescriptorLength (length in bytes of keyboard report descriptor
       // (not this one, but 0x22))

    // Endpoint descriptors (see sample in table e.5 of HID spec sheet)
    7,                  // bLength
    0x05,               // bDescriptorType
    0x80 | keyboard_ep, // bEndpointAddress - Conbimation of setting ep1 (0x1)
                        // to be "IN" type (0x80)
    0x03,               // bmAttributes - Set endpoint to interrupt
    8, 0, // wMaxPacketSize - The size of the keyboard banks (8 bytes I think)
    0x0A  // wInterval - polling rate
};

// HID Report Descriptor
static const uint8_t HID_report_descriptor[] PROGMEM = {
    // See sample in e.6 of hid spec sheet
    // Can pretty much ignore the first column of the below descriptor

    0x05, 0x01, // Usage Page - Generic Desktop - correct is: 0x05, 0x01
    0x09, 0x06, // Usage - Keyboard
    0xA1, 0x01, // Collection - Application

    // This section deals with the byte for modifier keys
    0x05, 0x07, // Usage Page - Key Codes
    0x19, 0xE0, // Usage Minimum - The bit that controls the 8 modifier
                // characters (ctrl, command, etc)
    0x29, 0xE7, // Usage Maximum - The end of the modifier bit (0xE7 - 0xE0 =
                // 1 byte)
    0x15, 0x00, // Logical Minimum - These keys are either not pressed or
                // pressed, 0 or 1
    0x25, 0x01, // Logical Maximum - Pressed state == 1
    0x75, 0x01, // Report Size - (one byte for modifier keys)
    0x95, 0x08, // Report Count - (8 possible keys in the above 1 byte)
    0x81, 0x02, // Input - These are variable type inputs

    // This describes the byte between modifier and keys (this is reserved as
    // per spec)
    0x95, 0x01, // Report Count - 1
    0x75, 0x08, // Report Size - 8
    0x81, 0x01, // Input type - Constant (reserved)
    0x95, 0x05, // Report Count - This is for the keyboard LEDs
    0x75, 0x01, // Report Size

    // Section for on-board LEDs
    0x05, 0x08, // Usage Page for LEDs
    0x19, 0x01, // Usage minimum for LEDs
    0x29, 0x05, // Usage maximum for LEDs
    0x91, 0x02, // Output - variable type

    // Padding for LED
    0x95, 0x01, // Report Count -
    0x75, 0x03, // Report Size
    0x91, 0x01, // Output - Constant for padding

    // Finally, the keys themselves
    0x95, 0x06, // Report Count - 6 keys
    0x75, 0x08, // Report Size - Each key gets 8 bits (1 byte / key)
    0x15, 0x00, // Logical Minimum (0)
    0x25, 0x65, // Logical Maximum (101)
    0x05, 0x07, // Usage Page - Key Codes
    0x19, 0x00, // Usage Minimum - 0
    0x29, 0x65, // Usage Maximum - 101
    0x81, 0x00, // Input - Data, Array
    // length:  31 rows = 62 bytes + 1 byte (end of collection) = 63 bytes
    0xC0 // End collection
};

// Language Descriptor
static const uint8_t PROGMEM lang_des[] = {
    4,    // bLength = 0x04, length of descriptor in bytes
    0x03, // bDescriptorType = 0x03, Descriptor ID = 3 -> String descriptor
    0x09, 0x04 // wLANGID[0] = 0x0409 = English USA (Supported Lang. Code 0)
};

// Manufacturer Descriptor
static const uint8_t PROGMEM manu_des[] = {
    18,   // bLength = length of descriptor in bytes
    0x03, // bDescriptorType = 0x03, Descriptor ID = 3 -> String descriptor
          // bString = Unicode Encoded String (16 Bit!) so add 0 at end of chars
    'G',
    0,
    'R',
    0,
    'O',
    0,
    'U',
    0,
    'P',
    0,
    ' ',
    0,
    '5',
    0,
    '7',
    0,
};

// Product Descriptor
static const uint8_t PROGMEM prod_des[] = {
    26,   // bLength = length of descriptor in bytes
    0x03, // bDescriptorType = 0x03, Descriptor ID = 3 -> String descriptor
          // bString = Unicode Encoded String (16 Bit!)
    'E', 0, 'p', 0, 'i', 0, 'c', 0, 'K', 0, 'e', 0, 'y', 0, 'b', 0, 'o', 0, 'a', 0, 'r', 0, 'd', 0};

// Serial Descriptor
static const uint8_t PROGMEM seri_des[] = {
    10,   // bLength = 0x12, length of descriptor in bytes
    0x03, // bDescriptorType = 0x03, Descriptor ID = 3 -> String descriptor
          // bString = Unicode Encoded String (16 Bit!)
    '6',
    0,
    '9',
    0,
    '6',
    0,
    '9',
    0,
};

#endif // USB_H