#include "cdc/commands/cdc_cmd_system.h"

#include "cdc/cdc_transport.h"
#include "cdc/commands/cdc_cmd_write.h"
#include "hardware_interface.h"
#include "macro_config.h"
#include "oled_display.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>

void cmd_handle_save_flash(void) {
  if (config_save()) {
    cdc_send_response("OK");
    printf("[CDC] Configuration saved to flash\n");
  } else {
    cdc_send_response("ERROR|Flash write failed");
  }
}

void cmd_handle_reload_config(void) {
  printf("[CDC] RELOAD_CONFIG command received\n");
  config_init();
  oled_display_layer_info(0);
  cdc_send_response("OK");
  printf("[CDC] Config reloaded\n");
  config_mode = 0; // exit configuration mode
}

void cmd_handle_set_config_mode(const char *args) {
  const char *token = args;
  uint8_t val = atoi(token);

  config_mode = val;

  // wake up the screen and reset the timer each time the mode changes
  // if we enter config -> the screen turns on
  // if we exit -> we reset the timer so that it does not turn off immediately
  oled_wake_up();

  if (config_mode) {
    cdc_log("[CDC] Config mode ENABLED\n");
    oled_display_layer_info(
        config_get_current_layer()); // refresh to show setup screen
  } else {
    cdc_log("[CDC] Config mode DISABLED\n");
    // return to default layout
    oled_display_layer_info(config_get_current_layer());
  }

  cdc_send_response("OK");
}

void cmd_handle_set_oled_timeout(const char *args) {
  uint32_t timeout = strtoul(args, NULL, 10);
  config_data_t *config = config_get();
  config->oled_timeout_s = timeout;
  cdc_send_response("OK");
}

void cmd_handle_bootsel(void) {
  cdc_log("[SYSTEM] Entering BOOTSEL mode...\n");

  oled_clear();
  oled_draw_string(10, 20, "FIRMWARE UPDATE");
  oled_draw_string(10, 36, "Drop .uf2 file");
  oled_update();

  cdc_send_response("OK");
  sleep_ms(50);

  reset_usb_boot(0, 0);
}
