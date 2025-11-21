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

static void press_sequence(uint8_t modifiers, uint8_t keycode) {
  // 1. modyfikatory + klawisz
  while (!tud_hid_ready())
    tud_task();

  uint8_t report[6] = {keycode, 0, 0, 0, 0, 0};
  tud_hid_keyboard_report(1, modifiers, report);

  // solidne przytrzymanie, zeby system zarejestrowal skrot
  sleep_ms(60);

  // 2. pusc klawisz (zostaw modyfikatory)
  if (keycode != 0) {
    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, modifiers, NULL);
    sleep_ms(20);
  }

  // 3. ANTI-SPOTLIGHT TRICK
  // jesli wsrod modyfikatorow jest GUI (Meta/Command), docisniecie na chwile
  // CTRL system pomysli, ze konczona jest sekwencja "Meta+Ctrl" ktora nie
  // otwiera menu na linuxach
  if (modifiers & (MODIFIER_LEFT_GUI | MODIFIER_RIGHT_GUI)) {
    uint8_t safe_mods = modifiers | MODIFIER_LEFT_CTRL;

    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, safe_mods, NULL);
    sleep_ms(20);
  }

  // 4. pusc wszystko
  while (!tud_hid_ready())
    tud_task();
  tud_hid_keyboard_report(1, 0, NULL);

  sleep_ms(50);
}

// SZYBKA wersja do spammowania klawiszy (Turbo Mode)
// Omija hacki systemowe, stawia na predkosc
static void perform_key_repeat_fast(uint8_t keycode, uint16_t count,
                                    uint16_t interval) {
  if (count == 0)
    count = 1;

  // Jesli interval < 10ms, uznajemy ze uzytkownik chce "MAX SPEED"
  // Skracamy czas trzymania do minimum (1-2ms)
  uint32_t hold_time = (interval < 10) ? 2 : 20;
  uint32_t wait_time = (interval > 0) ? interval : 2;

  for (uint16_t i = 0; i < count; i++) {
    // Press
    while (!tud_hid_ready())
      tud_task();
    uint8_t report[6] = {keycode, 0, 0, 0, 0, 0};
    tud_hid_keyboard_report(1, 0, report);

    sleep_ms(hold_time);

    // Release
    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, 0, NULL);

    // Delay
    if (i < count - 1) {
      sleep_ms(wait_time);
    }
    tud_task(); // Keep USB alive
  }
}

// wpisuje tekst znak po znaku (obsluguje unicode i ascii)
static void type_text_content(const char *text, uint8_t platform) {
  const char *p = text;
  while (*p) {
    uint32_t code = utf8_to_codepoint(&p);
    if (code == 0)
      continue;

    if (code < 128) {
      // ascii
      uint8_t keycode, modifiers;
      if (map_char_to_hid((char)code, &keycode, &modifiers)) {
        while (!tud_hid_ready())
          tud_task();

        uint8_t report[6] = {keycode, 0, 0, 0, 0, 0};
        tud_hid_keyboard_report(1, modifiers, report);
        sleep_ms(2);

        while (!tud_hid_ready())
          tud_task();
        tud_hid_keyboard_report(1, 0, NULL);
        sleep_ms(2);
      }
    } else {
      // unicode - wymaga wczesniejszej definicji send_unicode
      send_unicode(platform, code);
    }
  }
}

// Helper do obslugi myszy
static void perform_mouse_click(uint8_t buttons, uint16_t count,
                                uint16_t interval) {
  if (count == 0)
    count = 1;
  uint32_t hold_time = (interval < 10) ? 5 : 30; // Mysz potrzebuje min 5ms
  uint32_t wait_time = (interval > 0) ? interval : 30;

  for (uint16_t i = 0; i < count; i++) {
    // Press
    while (!tud_hid_ready())
      tud_task();
    // Report ID 2 = Mouse
    // Petla retry, jesli bufor jest zajety
    int retry = 10;
    while (!tud_hid_mouse_report(2, buttons, 0, 0, 0, 0) && retry-- > 0) {
      sleep_ms(1);
      tud_task();
    }

    sleep_ms(hold_time);

    // Release
    while (!tud_hid_ready())
      tud_task();
    tud_hid_mouse_report(2, 0, 0, 0, 0, 0);

    if (i < count - 1) {
      sleep_ms(wait_time);
    }
    tud_task();
  }
}

// Helper do klawiatury (Auto-Clicker)
static void perform_key_repeat(uint8_t keycode, uint16_t count,
                               uint16_t interval) {
  if (count == 0)
    count = 1;

  for (uint16_t i = 0; i < count; i++) {
    press_sequence(0, keycode); // Uzywamy naszej funkcji "Atomowej"

    if (i < count - 1) {
      sleep_ms(interval > 0 ? interval : 20);
    }
    tud_task(); // Keep-alive
  }
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
    perform_key_repeat_fast((uint8_t)macro->value, macro->repeat_count,
                            macro->repeat_interval);
    break;
  }

  case MACRO_TYPE_MOUSE_BUTTON: {
    // value to maska (1=L, 2=R, 4=M)
    perform_mouse_click((uint8_t)macro->value, macro->repeat_count,
                        macro->repeat_interval);
    break;
  }

  case MACRO_TYPE_MOUSE_MOVE: {
    // value uzywane opcjonalnie, ale glownie move_x/move_y
    if (tud_hid_ready()) {
      tud_hid_mouse_report(2, 0, (int8_t)macro->move_x, (int8_t)macro->move_y,
                           0, 0);
    }
    break;
  }

  case MACRO_TYPE_MOUSE_WHEEL: {
    // value to scroll amount (int8)
    if (tud_hid_ready()) {
      tud_hid_mouse_report(2, 0, 0, 0, (int8_t)macro->value, 0);
    }
    break;
  }

  case MACRO_TYPE_TEXT_STRING: {
    uint8_t detected_os = detect_platform();
    type_text_content(macro->macro_string, detected_os);
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
    uint8_t platform = macro->script_platform;
    cdc_log("[SCRIPT] Executing script (platform=%d)\n", platform);

    if (platform == 0) { // LINUX
      // 1. wykonaj skrot do terminala
      if (macro->terminal_shortcut_length > 0) {
        for (int i = 0; i < macro->terminal_shortcut_length; i++) {
          press_sequence(macro->terminal_shortcut[i].modifiers,
                         macro->terminal_shortcut[i].keycode);

          // minimalny odstep miedzy kolejnymi skrotami (jesli sekwencja ma > 1)
          if (i < macro->terminal_shortcut_length - 1)
            sleep_ms(100);
        }
      } else {
        // fallback: ctrl+alt+t
        press_sequence(0x05, 23);
      }

      // zwiekszony czas na otwarcie okna terminala
      sleep_ms(1500);

      // 2. utworz plik (heredoc)
      type_text_content("cat << 'EOF' > /tmp/m.sh\n", platform);
      sleep_ms(200);

      // 3. wpisz tresc
      type_text_content(macro->script, platform);

      // 4. zamknij plik (enter -> eof -> enter)
      press_sequence(0, 40); // enter
      type_text_content("EOF\n", platform);
      sleep_ms(200);

      // 5. uruchom
      type_text_content("chmod +x /tmp/m.sh && /tmp/m.sh && rm /tmp/m.sh\n",
                        platform);

    } else if (platform == 1) { // WINDOWS
      press_sequence(0x08, 21); // Win + R
      sleep_ms(500);
      type_text_content("powershell\n", platform);
      sleep_ms(1500);
      type_text_content(macro->script, platform);
      press_sequence(0, 40);

    } else if (platform == 2) { // MACOS
      press_sequence(0x08, 44); // Cmd + Space
      sleep_ms(300);
      type_text_content("Terminal", platform);
      sleep_ms(100);
      press_sequence(0, 40);
      sleep_ms(1000);
      type_text_content("cat << 'EOF' > /tmp/m.sh\n", platform);
      type_text_content(macro->script, platform);
      press_sequence(0, 40);
      type_text_content("EOF\n", platform);
      sleep_ms(100);
      type_text_content("sh /tmp/m.sh && rm /tmp/m.sh\n", platform);
    }
    break;
  }

  case MACRO_TYPE_KEY_SEQUENCE: {
    cdc_log("[HID] Seq steps: %d\n", macro->sequence_length);
    for (int i = 0; i < macro->sequence_length; i++) {
      // Uzywamy bezpiecznej sekwencji (z Anti-Spotlight), bo to skroty
      press_sequence(macro->sequence[i].modifiers, macro->sequence[i].keycode);

      // Opcjonalny dodatkowy delay z duration
      if (macro->sequence[i].duration > 0) {
        sleep_ms(macro->sequence[i].duration);
      }
    }
    break;
  }
  }

  led_toggle(button);
  sleep_ms(100);
  led_toggle(button);

  if (tud_hid_ready()) {
    tud_hid_keyboard_report(1, 0, NULL);
    tud_hid_mouse_report(2, 0, 0, 0, 0, 0);
  }
}
