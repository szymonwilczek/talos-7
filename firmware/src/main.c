#include "cdc_protocol.h"
#include "hardware_interface.h"
#include "macro_config.h"
#include "macro_executor.h"
#include "pico/stdlib.h"
#include "pin_definitions.h"
#include "tusb.h"
#include <stdio.h>

static bool button_processed[NUM_BUTTONS] = {false};

static void print_boot_message(void) {
  sleep_ms(4000); // czekanie na CDC connection

  cdc_log("\n");
  cdc_log("========================================\n");
  cdc_log("  MACRO KEYBOARD FIRMWARE v1.0\n");
  cdc_log("========================================\n");
  cdc_log("Device: Raspberry Pi Pico (RP2040)\n");
  cdc_log("USB VID:PID = 0x%04X:0x%04X\n", USB_VID, USB_PID);
  cdc_log("Buttons: %d (GP%d-GP%d)\n", NUM_BUTTONS, BTN_PIN_1, BTN_PIN_7);
  cdc_log("LEDs: %d (GP%d-GP%d)\n", NUM_BUTTONS, LED_PIN_1, LED_PIN_7);
  cdc_log("OLED: SPI0 (MOSI=GP%d, SCK=GP%d)\n", OLED_MOSI_PIN, OLED_SCK_PIN);
  cdc_log("Layers: %d\n", MAX_LAYERS);
  cdc_log("========================================\n");
  cdc_log("\n");
}

int main(void) {
  stdio_init_all();
  tusb_init();
  sleep_ms(1000);

  cdc_log("[BOOT] Waiting for CDC connection...\n");
  for (int i = 0; i < 100; i++) {
    tud_task();
    if (tud_cdc_connected()) {
      cdc_log("[BOOT] CDC connected!\n");
      break;
    }
    sleep_ms(100);
  }

  print_boot_message();

  cdc_log("[MAIN] Loading configuration...\n");
  config_init();

  cdc_log("[MAIN] Initializing CDC protocol...\n");
  cdc_protocol_init();

  cdc_log("[MAIN] Initializing hardware...\n");
  hardware_init();

  // displaying initial layer
  uint8_t current_layer = config_get_current_layer();
  oled_display_layer_info(current_layer);
  leds_update_for_layer(current_layer);

  cdc_log("[MAIN] System ready!\n");
  cdc_log("[MAIN] Press Button 1 (GP%d) to test\n", BTN_PIN_1);
  cdc_log("\n");

  // button debounce
  uint32_t last_button_time[NUM_BUTTONS] = {0};
  const uint32_t DEBOUNCE_MS = 50;

  while (1) {
    tud_task();
    cdc_protocol_task();

    buttons_scan();

    uint32_t now = to_ms_since_boot(get_absolute_time());
    current_layer = config_get_current_layer();

    for (int i = 0; i < NUM_BUTTONS; i++) {
      bool pressed = button_is_pressed(i);

      if (pressed && !button_processed[i] &&
          (now - last_button_time[i] > DEBOUNCE_MS)) {
        cdc_log("[MAIN] Button %d pressed!\n", i + 1);
        execute_macro(current_layer, i);
        last_button_time[i] = now;
        button_processed[i] = true;
      } else if (!pressed) {
        button_processed[i] = false; // reset after release
      }
    }

    sleep_ms(1);
  }

  return 0;
}

// USB CDC Callbacks
void tud_cdc_rx_cb(uint8_t itf) { (void)itf; }

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
  (void)itf;
  (void)rts;

  if (dtr) {
    printf("[USB] CDC connected\n");
  } else {
    printf("[USB] CDC disconnected\n");
  }
}
