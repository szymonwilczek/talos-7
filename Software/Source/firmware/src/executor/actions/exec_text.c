#include "executor/actions/exec_text.h"

#include "cdc/cdc_transport.h"
#include "hardware/watchdog.h"
#include "hardware_interface.h"
#include "macro_config.h"
#include "tusb.h"
#include <stdbool.h>
#include <stdio.h>

void send_unicode(uint8_t platform, uint32_t codepoint) {
  char hex[9];
  sprintf(hex, "%x", codepoint);
  cdc_log("[HID] Unicode hex (lower): %s\n", hex);

  if (platform == 0) { // Linux (GTK / IBus)

    // Ctrl + Shift
    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, 0x03, NULL);
    sleep_ms(2); // registering modifiers

    // U (holding Ctrl + Shift)
    while (!tud_hid_ready())
      tud_task();

    tud_hid_keyboard_report(1, 0x03, (uint8_t[]){24, 0, 0, 0, 0, 0});
    sleep_ms(2);

    // release U (still holding Ctrl + Shift)
    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, 0x03, NULL);
    sleep_ms(2);

    // release Ctrl + Shift
    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, 0, NULL);
    sleep_ms(15); // waiting for gui response

    // hex digits
    for (char *h = hex; *h; h++) {
      watchdog_update();

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

    // spacebar to confirm input
    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, 0, (uint8_t[]){44, 0, 0, 0, 0, 0});
    sleep_ms(2);

    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, 0, NULL);
    sleep_ms(2);

  } else if (platform == 1) { // Windows: hex digits + alt+x sequence

    // hex
    for (char *h = hex; *h; h++) {
      watchdog_update();

      uint8_t keycode, modifiers;
      if (map_char_to_hid(*h, &keycode, &modifiers)) {
        while (!tud_hid_ready())
          tud_task();

        uint8_t report[6] = {keycode, 0, 0, 0, 0, 0};
        tud_hid_keyboard_report(1, modifiers, report);
        sleep_ms(2);

        while (!tud_hid_ready())
          tud_task();
        tud_hid_keyboard_report(1, 0, NULL); // release
        sleep_ms(2);
      }
    }

    sleep_ms(10);

    // Alt + x
    while (!tud_hid_ready())
      tud_task();

    tud_hid_keyboard_report(1, 0x04, (uint8_t[]){27, 0, 0, 0, 0, 0});
    cdc_log("[HID] Sent Alt+X conversion\n");
    sleep_ms(20);

    // release Alt + x
    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, 0, NULL);
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
      watchdog_update();

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

bool is_pure_ascii(const char *str) {
  while (*str) {
    if ((unsigned char)*str > 127)
      return false;
    str++;
  }
  return true;
}

void type_text_turbo_ascii(const char *text) {
  const char *p = text;
  while (*p) {
    watchdog_update();

    uint8_t keycode, modifiers;

    if (map_char_to_hid(*p, &keycode, &modifiers)) {
      // press
      while (!tud_hid_ready())
        tud_task(); // waiting only if buffer is full

      uint8_t report[6] = {keycode, 0, 0, 0, 0, 0};
      tud_hid_keyboard_report(1, modifiers, report);
      sleep_ms(5); // minimal delay for keypress

      // release
      while (!tud_hid_ready())
        tud_task();
      tud_hid_keyboard_report(1, 0, NULL);

      sleep_ms(5);
    }
    p++;
  }
}

void type_text_content(const char *text, uint8_t platform) {
  const char *p = text;
  while (*p) {
    watchdog_update();

    uint32_t code = utf8_to_codepoint(&p);

    if (code == 0)
      continue;

    // ascii
    if (code < 128) {
      uint8_t keycode, modifiers;

      if (map_char_to_hid((char)code, &keycode, &modifiers)) {
        while (!tud_hid_ready())
          tud_task();

        uint8_t report[6] = {keycode, 0, 0, 0, 0, 0};
        tud_hid_keyboard_report(1, modifiers, report);
        sleep_ms(5);

        while (!tud_hid_ready())
          tud_task();

        tud_hid_keyboard_report(1, 0, NULL);
        sleep_ms(5);
      }
    } else {
      // unicode
      send_unicode(platform, code);
    }
  }
}

void exec_type_text_auto(const char *text) {
  uint8_t detected_os = detect_platform();

  if (is_pure_ascii(text)) {
    cdc_log("[HID] Typing Turbo ASCII\n");
    type_text_turbo_ascii(text);
  } else {
    cdc_log("[HID] Typing Unicode Text (auto-os: %d)\n", detected_os);
    type_text_content(text, detected_os);
  }

  while (!tud_hid_ready())
    tud_task();

  tud_hid_keyboard_report(1, 0, NULL);
}
