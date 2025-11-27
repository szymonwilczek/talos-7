#ifndef PIN_DEFINITIONS_H
#define PIN_DEFINITIONS_H

#include <stdint.h>

// ==================== GPIO PINS ====================
// przyciski
#define BTN_PIN_1 2
#define BTN_PIN_2 3
#define BTN_PIN_3 4
#define BTN_PIN_4 5
#define BTN_PIN_5 6
#define BTN_PIN_6 7
#define BTN_PIN_7 8
#define BTN_OS_TOGGLE_PIN 9

// LED
#define LED_PIN_1 20
#define LED_PIN_2 21
#define LED_PIN_3 22
#define LED_PIN_4 26
#define LED_PIN_5 27
#define LED_PIN_6 28
#define LED_PIN_7 0

// OLED SPI
#define OLED_SPI spi0
#define OLED_MOSI_PIN 19 // GP19
#define OLED_SCK_PIN 18  // GP18
#define OLED_CS_PIN 17   // GP17
#define OLED_DC_PIN 16   // GP16
#define OLED_RST_PIN 15  // GP15

// ==================== USB CONFIGURATION ====================
// USB VID/PID - Raspberry Pi Foundation
#define USB_VID 0x2E8A
#define USB_PID 0x0032

// ==================== DEVICE CONSTANTS ====================
#define NUM_BUTTONS 7
#define MAX_LAYERS 4

// nazwy pinow
static const uint8_t BUTTON_PINS[NUM_BUTTONS] = {
    BTN_PIN_1, BTN_PIN_2, BTN_PIN_3, BTN_PIN_4,
    BTN_PIN_5, BTN_PIN_6, BTN_PIN_7};

static const uint8_t LED_PINS[NUM_BUTTONS] = {LED_PIN_1, LED_PIN_2, LED_PIN_3,
                                              LED_PIN_4, LED_PIN_5, LED_PIN_6,
                                              LED_PIN_7};

#endif // PIN_DEFINITIONS_H
