#include "cdc/commands/cdc_cmd_read.h"
#include "cdc/cdc_transport.h"
#include "macro_config.h"
#include "tusb.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

void cmd_handle_get_conf(void) {
  config_data_t *config = config_get();

  cdc_log("[CDC] GET_CONF command received\n");

  cdc_send_response("CONF_START");
  tud_cdc_write_flush();

  // global settings
  cdc_send_response_fmt("SETTINGS|%lu", config->oled_timeout_s);

  // layer names and emojis
  for (int layer = 0; layer < MAX_LAYERS; layer++) {
    cdc_send_response_fmt("LAYER_NAME|%d|%s|%d", layer,
                          config->layer_names[layer],
                          config->layer_emojis[layer]);
    tud_cdc_write_flush();
  }
  printf("[CDC] Sent %d layer names\n", MAX_LAYERS);

  // all macros
  int macro_count = 0;
  for (int layer = 0; layer < MAX_LAYERS; layer++) {
    for (int btn = 0; btn < NUM_BUTTONS; btn++) {
      macro_entry_t *macro = &config->macros[layer][btn];
      if (macro->type == MACRO_TYPE_SCRIPT) {
        // just metadata first
        cdc_send_response_fmt(
            "MACRO|%d|%d|%d|%d|%s|%s|%d|%d|%d", layer, btn, macro->type,
            macro->value, macro->macro_string, macro->name, macro->emoji_index,
            macro->script_platform, macro->terminal_shortcut_length);

        for (int i = 0; i < macro->terminal_shortcut_length; i++) {
          cdc_send_response_fmt("SCRIPT_SHORTCUT|%d|%d|%d|%d|%d", layer, btn, i,
                                macro->terminal_shortcut[i].keycode,
                                macro->terminal_shortcut[i].modifiers);
        }

        // WHOLE script content streamingly
        // first header
        char header[64];
        snprintf(header, sizeof(header), "SCRIPT_DATA|%d|%d|%d|", layer, btn,
                 macro->script_platform);
        tud_cdc_write_str(header);

        // content char by char directly to usb
        size_t len = strlen(macro->script);
        for (size_t i = 0; i < len; i++) {
          char c = macro->script[i];
          if (c == '\n')
            tud_cdc_write_str("\\n");
          else if (c == '\r')
            tud_cdc_write_str("\\r");
          else if (c == '|')
            tud_cdc_write_str("\\|");
          else if (c == '\\')
            tud_cdc_write_str("\\\\");
          else
            tud_cdc_write_char(c);

          // flushing the buffer from time to time to avoid filling TinyUSB FIFO
          if (i % 64 == 0)
            tud_task();
        }

        // SCRIPT_DATA end line
        tud_cdc_write_str("\r\n");
        tud_cdc_write_flush();
      } else if (macro->type == MACRO_TYPE_KEY_SEQUENCE &&
                 macro->sequence_length > 0) {
        cdc_send_response_fmt("MACRO_SEQ|%d|%d|%s|%d|%d", layer, btn,
                              macro->name, macro->emoji_index,
                              macro->sequence_length);

        for (int i = 0; i < macro->sequence_length; i++) {
          cdc_send_response_fmt("SEQ_STEP|%d|%d|%d|%d|%d|%d", layer, btn, i,
                                macro->sequence[i].keycode,
                                macro->sequence[i].modifiers,
                                macro->sequence[i].duration);
        }
      } else {
        cdc_send_response_fmt(
            "MACRO|%d|%d|%d|%d|%s|%s|%d|%d|%d|%d|%d", layer, btn, macro->type,
            macro->value, macro->macro_string, macro->name, macro->emoji_index,
            macro->repeat_count, macro->repeat_interval, macro->move_x,
            macro->move_y);
      }
    }
  }
  printf("[CDC] Sent %d macros\n", macro_count);

  printf("[CDC] About to send CONF_END...\n");
  cdc_send_response("CONF_END");
  printf("[CDC] Sent CONF_END\n");

  tud_cdc_write_flush();
  sleep_ms(10);
  tud_cdc_write_flush();
  printf("[CDC] Configuration sent complete\n");
}
