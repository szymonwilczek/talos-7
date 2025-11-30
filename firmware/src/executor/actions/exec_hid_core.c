#include "executor/actions/exec_hid_core.h"

#include "cdc/cdc_transport.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "pin_definitions.h"
#include "tusb.h"

bool check_cancel(uint8_t btn_index, bool *wait_for_release) {
  bool currently_pressed = !gpio_get(BUTTON_PINS[btn_index]);

  if (*wait_for_release) {
    if (!currently_pressed) {
      *wait_for_release = false; // user released the button
    }

    return false;
  } else {
    if (currently_pressed) {
      cdc_log("[EXECUTOR] Loop cancelled by user!\n");
      return true; // cancel the loop
    }
  }
  return false;
}

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

void press_sequence(uint8_t modifiers, uint8_t keycode) {
  // 1. modifiers + key
  while (!tud_hid_ready())
    tud_task();

  uint8_t report[6] = {keycode, 0, 0, 0, 0, 0};
  tud_hid_keyboard_report(1, modifiers, report);

  sleep_ms(60); // registering shortcut in os

  // 2. release key, keep modifiers
  if (keycode != 0) {
    while (!tud_hid_ready())
      tud_task();
    tud_hid_keyboard_report(1, modifiers, NULL);
    sleep_ms(20);
  }

  // 3. ANTI-SPOTLIGHT
  // if there is a GUI (Meta/Command) among the modifiers, pressing
  // CTRL for a moment will cause the system to think that the "Meta+Ctrl"
  // sequence has been completed, which does not open the menu on Linux
  if ((modifiers & (MODIFIER_LEFT_GUI | MODIFIER_RIGHT_GUI)) && keycode == 0) {
    uint8_t safe_mods = modifiers | MODIFIER_LEFT_CTRL;

    while (!tud_hid_ready())
      tud_task();

    tud_hid_keyboard_report(1, safe_mods, NULL);
    sleep_ms(20);
  }

  // 4. release all
  while (!tud_hid_ready())
    tud_task();

  tud_hid_keyboard_report(1, 0, NULL);
  sleep_ms(50);
}

void exec_key_repeat(uint8_t keycode, uint16_t count, uint16_t interval) {
  if (count == 0)
    count = 1;

  for (uint16_t i = 0; i < count; i++) {
    press_sequence(0, keycode);

    if (i < count - 1) {
      sleep_ms(interval > 0 ? interval : 20);
    }
    tud_task(); // keep alive
  }
}

void exec_key_repeat_fast(uint8_t keycode, uint16_t count, uint16_t interval) {
  if (count == 0)
    count = 1;

  uint32_t hold_time = (interval < 10) ? 2 : 20;
  uint32_t wait_time = (interval > 0) ? interval : 2;

  for (uint16_t i = 0; i < count; i++) {
    watchdog_update();

    // press
    while (!tud_hid_ready())
      tud_task();

    uint8_t report[6] = {keycode, 0, 0, 0, 0, 0};
    tud_hid_keyboard_report(1, 0, report);
    sleep_ms(hold_time);

    // release
    while (!tud_hid_ready())
      tud_task();

    tud_hid_keyboard_report(1, 0, NULL);

    // delay
    if (i < count - 1) {
      sleep_ms(wait_time);
    }
    tud_task(); // keep USB alive
  }
}

void exec_key_sequence(const key_step_t *sequence, uint8_t len) {
  for (int i = 0; i < len; i++) {
    watchdog_update();
    press_sequence(sequence[i].modifiers, sequence[i].keycode);

    if (sequence[i].duration > 0) {
      sleep_ms(sequence[i].duration);
    }
  }
}
