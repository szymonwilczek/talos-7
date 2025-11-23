#include "executor/macro_executor.h"

#include "cdc/cdc_transport.h"
#include "executor/actions/exec_hid_core.h"
#include "executor/actions/exec_midi_core.h"
#include "executor/actions/exec_mouse.h"
#include "executor/actions/exec_script.h"
#include "executor/actions/exec_text.h"
#include "hardware_interface.h"
#include "macro_config.h"
#include "oled_display.h"
#include "pico/stdlib.h"
#include "pin_definitions.h"
#include "tusb.h"
#include <stdio.h>
#include <string.h>

void execute_macro(uint8_t layer, uint8_t button) {
  config_data_t *config = config_get();
  macro_entry_t *macro = &config->macros[layer][button];

  cdc_log("[EXECUTOR] Executing macro: L%d B%d '%s'\n", layer, button,
          macro->name);

  oled_display_button_preview(layer, button);
  sleep_ms(1000);
  oled_display_layer_info(layer);

  switch (macro->type) {
  case MACRO_TYPE_KEY_PRESS: {
    exec_key_repeat_fast((uint8_t)macro->value, macro->repeat_count,
                         macro->repeat_interval);
    break;
  }

  case MACRO_TYPE_MOUSE_BUTTON: {
    exec_mouse_click((uint8_t)macro->value, macro->repeat_count,
                     macro->repeat_interval, button);
    break;
  }

  case MACRO_TYPE_MOUSE_MOVE: {
    exec_mouse_move(macro->move_x, macro->move_y, macro->repeat_count,
                    macro->repeat_interval, button);
    break;
  }

  case MACRO_TYPE_MOUSE_WHEEL: {
    exec_mouse_wheel((int8_t)macro->value);
    break;
  }

  case MACRO_TYPE_TEXT_STRING: {
    exec_type_text_auto(macro->macro_string);
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
    exec_script(macro->script, macro->script_platform, macro->terminal_shortcut,
                macro->terminal_shortcut_length);
    break;
  }

  case MACRO_TYPE_KEY_SEQUENCE: {
    exec_key_sequence(macro->sequence, macro->sequence_length);
    break;
  }

  case MACRO_TYPE_MIDI_NOTE: {
    // value  -> Note
    // move_x -> Velocity (default is 127)
    // move_y -> Channel (default is 1)

    uint8_t note = (uint8_t)macro->value;
    uint8_t velocity = (macro->move_x > 0) ? (uint8_t)macro->move_x : 127;
    uint8_t channel = (macro->move_y > 0) ? (uint8_t)macro->move_y : 1;

    exec_midi_note(note, velocity, channel);
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
