#include "cdc_protocol.h"
#include "hardware_interface.h"
#include "macro_config.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <stdio.h>
#include <string.h>

void send_key_with_modifiers(uint8_t modifiers, uint8_t keycode,
                             uint16_t press_ms) {
  while (!tud_hid_ready())
    tud_task();
  uint8_t report[6] = {keycode, 0, 0, 0, 0, 0};
  tud_hid_keyboard_report(1, modifiers, report);
  sleep_ms(press_ms);
  while (!tud_hid_ready())
    tud_task();
  tud_hid_keyboard_report(1, modifiers, NULL); // release key, keep modifiers
  sleep_ms(20);
}

void send_unicode(uint8_t platform, uint32_t codepoint) {
  char hex[9];
  sprintf(hex, "%x", codepoint);
  cdc_log("[HID] Unicode hex (lower): %s\n", hex);

  if (platform == 0) { // Linux (GTK / IBus)

    // Ctrl + Shift
    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, 0x03, NULL);
    sleep_ms(2); // zarejestrowanie modyfikatorow

    // U (trzymajac Ctrl + Shift)
    while (!tud_hid_ready())
      tud_task();

    tud_hid_keyboard_report(1, 0x03, (uint8_t[]){24, 0, 0, 0, 0, 0});
    sleep_ms(2);

    // pusc U (nadal trzymajac Ctrl + Shift)
    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, 0x03, NULL);
    sleep_ms(2);

    // pusc Ctrl + Shift
    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, 0, NULL);
    sleep_ms(15); // czekanie na reakcje gui

    // hex digits
    for (char *h = hex; *h; h++) {
      uint8_t keycode, modifiers;
      if (map_char_to_hid(*h, &keycode, &modifiers)) {
        while (!tud_hid_ready())
          tud_task();

        uint8_t report[6] = {keycode, 0, 0, 0, 0, 0};
        tud_hid_keyboard_report(1, 0, report);
        sleep_ms(2);

        while (!tud_hid_ready())
          tud_task();
        tud_hid_keyboard_report(1, 0, NULL); // release
        sleep_ms(2);
      }
    }

    // spacja
    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, 0, (uint8_t[]){44, 0, 0, 0, 0, 0});
    sleep_ms(2);

    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, 0, NULL);
    sleep_ms(2);

  } else if (platform == 1) { // Windows: Alt + + + hex + Alt release
    // alt down
    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, 0x04, NULL);
    cdc_log("[HID] Sent Alt down\n");
    sleep_ms(100);
    // +
    tud_hid_keyboard_report(1, 0x04, (uint8_t[]){46, 0, 0, 0, 0, 0});
    cdc_log("[HID] Sent +\n");
    sleep_ms(100);
    tud_hid_keyboard_report(1, 0x04, NULL);
    sleep_ms(50);
    // hex
    for (char *h = hex; *h; h++) {
      uint8_t keycode = 0;
      if (*h >= '0' && *h <= '9')
        keycode = 30 + (*h - '0');
      else if (*h >= 'A' && *h <= 'F')
        keycode = 4 + (*h - 'A');
      if (keycode) {
        while (!tud_hid_ready())
          tud_task();
        tud_hid_keyboard_report(1, 0x04, (uint8_t[]){keycode, 0, 0, 0, 0, 0});
        cdc_log("[HID] Sent hex digit %c\n", *h);
        sleep_ms(100);
        tud_hid_keyboard_report(1, 0, NULL);
        sleep_ms(50);
      }
    }
    // alt up
    tud_hid_keyboard_report(1, 0, NULL);
    cdc_log("[HID] Sent Alt up\n");
    sleep_ms(50);
  } else if (platform == 2) { // Mac: Ctrl+Cmd+Space + hex + space
    // Ctrl+Cmd+Space
    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, 0x11, (uint8_t[]){44, 0, 0, 0, 0, 0});
    cdc_log("[HID] Sent Ctrl+Cmd+Space\n");
    sleep_ms(100);
    tud_hid_keyboard_report(1, 0, NULL);
    sleep_ms(50);
    for (char *h = hex; *h; h++) {
      uint8_t keycode, modifiers;
      if (map_char_to_hid(*h, &keycode, &modifiers)) {
        while (!tud_hid_ready())
          tud_task();
        uint8_t report[6] = {keycode, 0, 0, 0, 0, 0};
        tud_hid_keyboard_report(1, modifiers, report);
        cdc_log("[HID] Sent hex digit %c\n", *h);
        sleep_ms(100);
        tud_hid_keyboard_report(1, 0, NULL);
        sleep_ms(50);
      }
    }
    // Space
    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, 0, (uint8_t[]){44, 0, 0, 0, 0, 0});
    cdc_log("[HID] Sent space\n");
    sleep_ms(100);
    tud_hid_keyboard_report(1, 0, NULL);
    sleep_ms(50);
  } else {
    cdc_log("[HID] Unicode not supported for platform %d\n", platform);
  }

  tud_hid_keyboard_report(1, 0, NULL);
}

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
    cdc_log("[HID] Typing text: '%s'\n", macro->macro_string);
    uint8_t detected_os = detect_platform();
    const char *p = macro->macro_string;
    while (*p) {
      uint32_t code = utf8_to_codepoint(&p);
      if (code == 0) {
        cdc_log("[HID] Invalid UTF-8\n");
        continue;
      }
      cdc_log("[HID] Codepoint: %u (0x%X)\n", code, code);
      if (code < 128) {
        // ASCII
        uint8_t keycode, modifiers;
        if (map_char_to_hid((char)code, &keycode, &modifiers)) {
          cdc_log("[HID] ASCII: key=%d mods=%d\n", keycode, modifiers);
          while (!tud_hid_ready())
            tud_task();
          uint8_t report[6] = {keycode, 0, 0, 0, 0, 0};
          tud_hid_keyboard_report(1, modifiers, report);
          cdc_log("[HID] Sent ASCII press\n");
          sleep_ms(2);
          tud_hid_keyboard_report(1, 0, NULL);
          cdc_log("[HID] Sent ASCII release\n");
          sleep_ms(10);
        }
      } else {
        // Unicode
        cdc_log("[HID] Unicode for platform %d\n", detected_os);
        send_unicode(detected_os, code);
      }
    }
    tud_hid_keyboard_report(1, 0, NULL);
    cdc_log("[EXECUTOR] Macro finished\n");
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

  tud_hid_keyboard_report(1, 0, NULL);
}
