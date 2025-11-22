#include "cdc/cdc_dispatcher.h"
#include "cdc/cdc_transport.h"
#include "cdc/commands/cdc_cmd_read.h"
#include "cdc/commands/cdc_cmd_system.h"
#include "cdc/commands/cdc_cmd_write.h"
#include "macro_config.h"
#include "tusb.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void process_command(const char *cmd_input) {
  printf("[CDC] Command received: %s\n", cmd_input);

  // local copy to modify
  char cmd[CDC_MAX_COMMAND_LEN];
  strncpy(cmd, cmd_input, CDC_MAX_COMMAND_LEN - 1);
  cmd[CDC_MAX_COMMAND_LEN - 1] = '\0';

  // removing whitespace from the start
  char *cmd_ptr = cmd;
  while (*cmd_ptr == ' ' || *cmd_ptr == '\t' || *cmd_ptr == '\r' ||
         *cmd_ptr == '\n') {
    cmd_ptr++;
  }

  // removing whitespace from the end
  size_t len = strlen(cmd_ptr);
  while (len > 0 && (cmd_ptr[len - 1] == ' ' || cmd_ptr[len - 1] == '\t' ||
                     cmd_ptr[len - 1] == '\r' || cmd_ptr[len - 1] == '\n')) {
    cmd_ptr[len - 1] = '\0';
    len--;
  }

  // debug log: TODO remove later
  printf("[CDC] Processed command: '%s' (len=%zu)\n", cmd_ptr, len);
  for (size_t i = 0; i < len && i < 20; i++) {
    printf("[CDC]   byte[%zu] = 0x%02x ('%c')\n", i, (unsigned char)cmd_ptr[i],
           (cmd_ptr[i] >= 32 && cmd_ptr[i] < 127) ? cmd_ptr[i] : '?');
  }

  if (len == 0) {
    printf("[CDC] Empty command, ignoring\n");
    return;
  }

  config_data_t *config = config_get();

  if (strcmp(cmd_ptr, "GET_CONF") == 0) {
    cmd_handle_get_conf();
    return;
  }

  if (strncmp(cmd_ptr, "SET_OLED_TIMEOUT|", 17) == 0) {
    char *token = cmd_ptr + 17;
    cmd_handle_set_oled_timeout(token);
    return;
  }

  if (strncmp(cmd_ptr, "SET_MACRO|", 10) == 0) {
    char *token = cmd_ptr + 10;
    cmd_handle_set_macro(token);
    return;
  }

  if (strncmp(cmd_ptr, "SET_LAYER_NAME|", 15) == 0) {
    char *token = cmd_ptr + 15;
    cmd_handle_set_layer_name(token);
    return;
  }

  if (strncmp(cmd_ptr, "SET_MACRO_SCRIPT|", 17) == 0) {
    char *token = cmd_ptr + 17;
    cmd_handle_set_macro_script(token);
    return;
  }

  if (strcmp(cmd_ptr, "RELOAD_CONFIG") == 0) {
    cmd_handle_reload_config();
    return;
  }

  if (strncmp(cmd_ptr, "SET_CONFIG_MODE|", 16) == 0) {
    char *token = cmd_ptr + 16;
    cmd_handle_set_config_mode(token);
    return;
  }

  if (strncmp(cmd_ptr, "SET_MACRO_SEQ|", 14) == 0) {
    char *token = cmd_ptr + 14;
    cmd_handle_set_macro_seq(token);
    return;
  }

  if (strcmp(cmd_ptr, "SAVE_FLASH") == 0) {
    cmd_handle_save_flash();
    return;
  }

  printf("[CDC] Unknown command: '%s'\n", cmd_ptr);
  cdc_send_response("ERROR|Unknown command");
}

void cdc_receive_script(uint8_t layer, uint8_t button, uint8_t platform,
                        uint16_t script_size) {
  config_data_t *config = config_get();
  macro_entry_t *macro = &config->macros[layer][button];

  printf("[CDC] Receiving script: L%d B%d Platform=%d Size=%d\n", layer, button,
         platform, script_size);

  uint16_t received = 0;
  uint32_t timeout_start = time_us_32();
  const uint32_t TIMEOUT_MS = 5000; // 5s timeout

  while (received < script_size) {
    if ((time_us_32() - timeout_start) > (TIMEOUT_MS * 1000)) {
      cdc_send_response("ERROR|Timeout");
      printf("[CDC] Script receive timeout\n");
      return;
    }

    if (tud_cdc_available()) {
      char c = tud_cdc_read_char();
      macro->script[received++] = c;
      timeout_start = time_us_32(); // reset timeout
    }

    tud_task();
    sleep_us(100);
  }

  macro->script[received] = '\0';
  macro->type = MACRO_TYPE_SCRIPT;
  macro->script_platform = platform;

  cdc_send_response("OK");
  printf("[CDC] Script received successfully\n");
}

void cdc_receive_sequence(uint8_t layer, uint8_t button, uint8_t count,
                          uint8_t *steps) {
  config_data_t *config = config_get();
  macro_entry_t *macro = &config->macros[layer][button];

  macro->type = MACRO_TYPE_KEY_SEQUENCE;
  macro->sequence_length = count;

  for (int i = 0; i < count && i < MAX_SEQUENCE_STEPS; i++) {
    macro->sequence[i].keycode = steps[i * 2];
    macro->sequence[i].modifiers = steps[i * 2 + 1];
  }

  cdc_send_response("OK");
}
