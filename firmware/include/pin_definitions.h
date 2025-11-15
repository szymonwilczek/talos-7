#ifndef PIN_DEFINITIONS_H
#define PIN_DEFINITIONS_H

#include <stdint.h>

// ==================== GPIO PINS ====================
// przyciski
#define BTN_PIN_1       2
#define BTN_PIN_2       3
#define BTN_PIN_3       4
#define BTN_PIN_4       5
#define BTN_PIN_5       6
#define BTN_PIN_6       7
#define BTN_PIN_7       8

// LED 
#define LED_PIN_1       16
#define LED_PIN_2       17
#define LED_PIN_3       18
#define LED_PIN_4       19
#define LED_PIN_5       20
#define LED_PIN_6       21
#define LED_PIN_7       22

// OLED I2C
#define OLED_SDA_PIN    26
#define OLED_SCL_PIN    27

// ==================== USB CONFIGURATION ====================
// USB VID/PID - Raspberry Pi Foundation
#define USB_VID         0x2E8A
#define USB_PID         0x000A

// ==================== DEVICE CONSTANTS ====================
#define NUM_BUTTONS     7
#define MAX_LAYERS      4

// nazwy pinow dla arrawy (ulatwia iteracje)
static const uint8_t BUTTON_PINS[NUM_BUTTONS] = {
    BTN_PIN_1, BTN_PIN_2, BTN_PIN_3, BTN_PIN_4,
    BTN_PIN_5, BTN_PIN_6, BTN_PIN_7
};

static const uint8_t LED_PINS[NUM_BUTTONS] = {
    LED_PIN_1, LED_PIN_2, LED_PIN_3, LED_PIN_4,
    LED_PIN_5, LED_PIN_6, LED_PIN_7
};

#endif // PIN_DEFINITIONS_H
