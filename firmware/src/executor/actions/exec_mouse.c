#include "executor/actions/exec_mouse.h"

#include "cdc/cdc_transport.h"
#include "executor/actions/exec_hid_core.h"
#include "tusb.h"
#include <stdbool.h>
#include <stdint.h>

void exec_mouse_click(uint8_t buttons, uint16_t count, uint16_t interval,
                      uint8_t trigger_btn) {
  if (count == 0)
    count = 1;

  uint32_t hold_time = (interval < 10) ? 5 : 30;
  uint32_t wait_time = (interval > 0) ? interval : 30;
  bool wait_for_release = true; // interrupt flag

  // wake up logic
  if (buttons & 1) {
    int retry = 50;
    while (retry-- > 0) {
      if (tud_hid_mouse_report(2, buttons, 0, 0, 0, 0))
        break;

      tud_task();
      sleep_ms(1);
    }

    sleep_ms(50);

    retry = 50;
    while (retry-- > 0) {
      if (tud_hid_mouse_report(2, 0, 0, 0, 0, 0))
        break;

      tud_task();
      sleep_ms(1);
    }

    sleep_ms(50);
  }

  for (uint16_t i = 0; i < count; i++) {
    // interrupt check
    if (check_cancel(trigger_btn, &wait_for_release))
      break;

    // press
    int retry = 50;
    bool sent = false;

    while (retry-- > 0) {
      if (tud_hid_mouse_report(2, buttons, 0, 0, 0, 0)) {
        sent = true;
        break;
      }

      tud_task();
      sleep_ms(1);
    }

    if (!sent)
      cdc_log("[MOUSE] Drop Press\n");

    sleep_ms(hold_time);

    // release
    retry = 50;
    sent = false;

    while (retry-- > 0) {
      if (tud_hid_mouse_report(2, 0, 0, 0, 0, 0)) {
        sent = true;
        break;
      }

      tud_task();
      sleep_ms(1);
    }

    if (!sent)
      cdc_log("[MOUSE] Drop Release\n");

    cdc_log("[MOUSE] Click %d/%d done\n", i + 1, count);

    if (i < count - 1) {
      sleep_ms(wait_time);
    }

    tud_task();
  }
}

void exec_mouse_move(int16_t total_x, int16_t total_y, uint16_t count,
                     uint16_t interval, uint8_t trigger_btn) {
  bool wait_for_release = true;
  bool infinite = (count == 0);
  uint32_t i = 0;

  // parametry wygladzania
  const int8_t MAX_STEP = 5;    // max 5 pikseli na raport (dla plynnosci)
  const uint8_t STEP_DELAY = 8; // 8ms miedzy krokami (ok 125Hz)

  while (infinite || i < count) {
    if (check_cancel(trigger_btn, &wait_for_release))
      break;

    int16_t x = total_x;
    int16_t y = total_y;

    // petla interpolacji ruchu
    while (x != 0 || y != 0) {
      // uzytkownik anuluje w trakcie dlugiego przejazdu
      if (check_cancel(trigger_btn, &wait_for_release))
        goto end_move;

      int8_t dx = (x > MAX_STEP) ? MAX_STEP : (x < -MAX_STEP) ? -MAX_STEP : x;
      int8_t dy = (y > MAX_STEP) ? MAX_STEP : (y < -MAX_STEP) ? -MAX_STEP : y;

      x -= dx;
      y -= dy;

      if (tud_hid_ready()) {
        tud_hid_mouse_report(2, 0, dx, dy, 0, 0);
      }
      sleep_ms(STEP_DELAY);
      tud_task();
    }

    if (infinite || i < count - 1)
      sleep_ms(interval > 0 ? interval : 20);
    if (!infinite)
      i++;
  }
end_move:;
}

void exec_mouse_wheel(int8_t wheel_value) {
  if (tud_hid_ready()) {
    tud_hid_mouse_report(2, 0, 0, 0, wheel_value, 0);
  }
}
