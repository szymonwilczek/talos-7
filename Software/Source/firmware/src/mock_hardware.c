#include "mock_hardware.h"
#include "pin_definitions.h"
#include <stdio.h>

// ==================== MOCK STATE ====================
static bool mock_led_states[NUM_BUTTONS] = {false};
static uint32_t mock_update_counter = 0;

// ==================== INICJALIZACJA ====================
void mock_hardware_init(void) {
  printf("[MOCK] Hardware initialization started\n");
  printf("[MOCK] Buttons: GP%d-GP%d (not connected)\n", BTN_PIN_1, BTN_PIN_7);
  printf("[MOCK] LEDs: GP%d-GP%d (not connected)\n", LED_PIN_1, LED_PIN_7);
  printf("[MOCK] OLED SPI: MOSI=GP%d, SCK=GP%d (not connected)\n",
         OLED_MOSI_PIN, OLED_SCK_PIN); 
  printf("[MOCK] All hardware mocked successfully\n");
}

// ==================== MOCK PRZYCISKI ====================
bool mock_button_read(uint8_t button_index) {
  if (button_index >= NUM_BUTTONS) {
    printf("[MOCK] Invalid button index: %d\n", button_index);
    return false;
  }

  // false (nie wcisniety)
  // TODO: SYMULACJA WCISNIEC
  return false;
}

// ==================== MOCK LED ====================
void mock_led_set(uint8_t led_index, bool state) {
  if (led_index >= NUM_BUTTONS) {
    printf("[MOCK] Invalid LED index: %d\n", led_index);
    return;
  }

  mock_led_states[led_index] = state;
  printf("[MOCK] LED %d (GP%d) -> %s\n", led_index + 1, LED_PINS[led_index],
         state ? "ON" : "OFF");
}

// ==================== MOCK OLED ====================
void mock_oled_init(void) {
  printf("[MOCK] OLED initialized (SPI: MOSI=GP%d, SCK=GP%d)\n", OLED_MOSI_PIN,
         OLED_SCK_PIN);
}

void mock_oled_display(uint8_t line, const char *text) {
  if (line > 3) {
    printf("[MOCK] Invalid OLED line: %d\n", line);
    return;
  }

  printf("[MOCK] OLED Line %d: \"%s\"\n", line, text);
}

// ==================== PERIODIC UPDATE ====================
void mock_hardware_update(void) {
  mock_update_counter++;

  // status
  if (mock_update_counter % 10000 == 0) {
    printf("[MOCK] Status update (#%lu):\n", mock_update_counter / 10000);

    // LED status
    printf("[MOCK]   LEDs: ");
    for (int i = 0; i < NUM_BUTTONS; i++) {
      printf("%d:%s ", i + 1, mock_led_states[i] ? "ON" : "OFF");
    }
    printf("\n");

    // button status (released)
    printf("[MOCK]   Buttons: All released (no physical connections)\n");
  }
}
