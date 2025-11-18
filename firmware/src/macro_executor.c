#include "hardware_interface.h"
#include "macro_config.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <stdio.h>
#include <string.h>

void execute_macro(uint8_t layer, uint8_t button) {
  config_data_t *config = config_get();
  macro_entry_t *macro = &config->macros[layer][button];

  cdc_log("[EXECUTOR] Executing macro: L%d B%d '%s'\n", layer, button,
          macro->name);

  // preview on OLED
  oled_display_button_preview(layer, button);
  sleep_ms(1000);
  oled_display_layer_info(layer);

  switch (macro->type) {
  case MACRO_TYPE_KEY_PRESS: {
    cdc_log("[HID] Sending key: 0x%02X\n", macro->value);

    // czekanie na gotowosc HID
    while (!tud_hid_ready()) {
      tud_task();
      sleep_ms(1);
    }

    uint8_t keycode[6] = {0};
    keycode[0] = macro->value;
    tud_hid_keyboard_report(1, 0, keycode);
    sleep_ms(50);

    // release
    while (!tud_hid_ready()) {
      tud_task();
      sleep_ms(1);
    }
    tud_hid_keyboard_report(1, 0, NULL);
    break;
  }

  case MACRO_TYPE_TEXT_STRING: {
    printf("[HID] Typing text: '%s'\n", macro->macro_string);
    // TODO
    break;
  }

  case MACRO_TYPE_LAYER_TOGGLE: {
    config_cycle_layer();
    uint8_t new_layer = config_get_current_layer();
    printf("[LAYER] Switched to layer %d\n", new_layer);
    leds_update_for_layer(new_layer);
    oled_display_layer_info(new_layer);
    break;
  }

  case MACRO_TYPE_SCRIPT: {
    printf("[SCRIPT] Executing script (platform=%d):\n%s\n",
           macro->script_platform, macro->script);
    // TODO
    break;
  }

  case MACRO_TYPE_KEY_SEQUENCE: {
    cdc_log("[HID] Executing key sequence (%d steps)\n",
            macro->sequence_length);
    for (int i = 0; i < macro->sequence_length; i++) {
      key_step_t *step = &macro->sequence[i];
      uint8_t keycode[6] = {0};
      keycode[0] = step->keycode;

      cdc_log("[HID] Step %d: mods=0x%02X key=0x%02X\n", i, step->modifiers,
              step->keycode);

      // czekanie na gotowosc HID przed "press"
      while (!tud_hid_ready()) {
        tud_task();
        sleep_ms(1);
      }
      tud_hid_keyboard_report(1, step->modifiers, keycode);
      sleep_ms(50);

      // czekanie na gotowosc HID przed "release"
      while (!tud_hid_ready()) {
        tud_task();
        sleep_ms(1);
      }
      tud_hid_keyboard_report(1, 0, NULL);
      sleep_ms(20);
    }
    break;
  }
  }

  led_toggle(button);
  sleep_ms(100);
  led_toggle(button);
}
