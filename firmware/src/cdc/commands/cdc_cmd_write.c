#include "cdc/commands/cdc_cmd_write.h"

#include "cdc/cdc_dispatcher.h"
#include "cdc/cdc_transport.h"
#include "hardware_interface.h"
#include "macro_config.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cmd_handle_set_macro(char *args) {
  int layer, button, type, value;
  int rep_cnt = 1, rep_int = 0, mx = 0, my = 0;
  char macro_string[MACRO_STRING_LEN] = {0};
  char name[MAX_NAME_LEN] = {0};
  uint8_t emoji_index = 0;
  config_data_t *config = config_get();

  char *token = args;

  layer = atoi(token);
  token = strchr(token, '|');
  if (!token) {
    cdc_send_response("ERROR|Invalid format");
    return;
  }
  token++;

  button = atoi(token);
  token = strchr(token, '|');
  if (!token) {
    cdc_send_response("ERROR|Invalid format");
    return;
  }
  token++;

  type = atoi(token);
  token = strchr(token, '|');
  if (!token) {
    cdc_send_response("ERROR|Invalid format");
    return;
  }
  token++;

  value = atoi(token);
  token = strchr(token, '|');
  if (!token) {
    cdc_send_response("ERROR|Invalid format");
    return;
  }
  token++;

  // macro_string
  char *next_pipe = strchr(token, '|');
  if (next_pipe) {
    size_t str_len = next_pipe - token;
    if (str_len >= MACRO_STRING_LEN)
      str_len = MACRO_STRING_LEN - 1;
    strncpy(macro_string, token, str_len);
    macro_string[str_len] = '\0';
    token = next_pipe + 1;
  } else {
    cdc_send_response("ERROR|Invalid format");
    return;
  }

  // name
  next_pipe = strchr(token, '|');
  if (next_pipe) {
    size_t name_len = next_pipe - token;
    if (name_len >= MAX_NAME_LEN)
      name_len = MAX_NAME_LEN - 1;
    strncpy(name, token, name_len);
    name[name_len] = '\0';
    token = next_pipe + 1;

    // emoji_index
    emoji_index = atoi(token);
  } else {
    strncpy(name, token, MAX_NAME_LEN - 1);
    name[MAX_NAME_LEN - 1] = '\0';
    emoji_index = 0;
  }

  token = strchr(token, '|');
  if (token) {
    token++;
    rep_cnt = atoi(token);
    token = strchr(token, '|');
    if (token) {
      token++;
      rep_int = atoi(token);
      token = strchr(token, '|');
      if (token) {
        token++;
        mx = atoi(token);
        token = strchr(token, '|');
        if (token) {
          token++;
          my = atoi(token);
        }
      }
    }
  }

  // validation and assignment
  if (layer >= 0 && layer < MAX_LAYERS && button >= 0 && button < NUM_BUTTONS) {

    macro_entry_t *macro = &config->macros[layer][button];
    macro->type = type;
    macro->value = value;
    strncpy(macro->macro_string, macro_string, MACRO_STRING_LEN - 1);
    strncpy(macro->name, name, MAX_NAME_LEN - 1);
    macro->emoji_index = emoji_index;
    macro->repeat_count = (uint16_t)rep_cnt;
    macro->repeat_interval = (uint16_t)rep_int;
    macro->move_x = (int16_t)mx;
    macro->move_y = (int16_t)my;

    cdc_send_response("OK");
    printf("[CDC] Macro set: L%d B%d '%s' %d\n", layer, button, name,
           emoji_index);
  } else {
    cdc_send_response("ERROR|Invalid parameters");
  }
}

void cmd_handle_set_layer_name(char *args) {
  int layer;
  char name[MAX_NAME_LEN] = {0};
  uint8_t emoji_index = 0;
  config_data_t *config = config_get();

  char *token = args;

  layer = atoi(token);
  token = strchr(token, '|');
  if (!token) {
    cdc_send_response("ERROR|Invalid format");
    return;
  }
  token++;

  // name
  char *next_pipe = strchr(token, '|');
  if (next_pipe) {
    size_t name_len = next_pipe - token;

    if (name_len >= MAX_NAME_LEN)
      name_len = MAX_NAME_LEN - 1;

    strncpy(name, token, name_len);
    name[name_len] = '\0';
    token = next_pipe + 1;
    emoji_index = atoi(token);

  } else {
    strncpy(name, token, MAX_NAME_LEN - 1);
    name[MAX_NAME_LEN - 1] = '\0';
    emoji_index = 0;
  }

  if (layer >= 0 && layer < MAX_LAYERS) {
    strncpy(config->layer_names[layer], name, MAX_NAME_LEN - 1);
    config->layer_emojis[layer] = emoji_index;

    cdc_send_response("OK");
    printf("[CDC] Layer name set: L%d = %d %s\n", layer, emoji_index, name);
  } else {
    cdc_send_response("ERROR|Invalid parameters");
  }
}

void cmd_handle_set_macro_seq(char *args) {
  int layer, button, step_count;
  char name[MAX_NAME_LEN] = {0};
  uint8_t emoji_index = 0;

  char *token = args;

  layer = atoi(token);
  token = strchr(token, '|');
  if (!token) {
    cdc_send_response("ERROR|Invalid SET_MACRO_SEQ format");
    return;
  }
  token++;

  button = atoi(token);
  token = strchr(token, '|');
  if (!token) {
    cdc_send_response("ERROR|Invalid SET_MACRO_SEQ format");
    return;
  }
  token++;

  char *end_name = strchr(token, '|');
  if (!end_name) {
    cdc_send_response("ERROR|Invalid SET_MACRO_SEQ format");
    return;
  }
  size_t name_len = end_name - token;

  if (name_len >= MAX_NAME_LEN)
    name_len = MAX_NAME_LEN - 1;

  strncpy(name, token, name_len);
  name[name_len] = '\0';
  token = end_name + 1;

  // emoji_index
  char *end_emoji = strchr(token, '|');
  if (!end_emoji) {
    cdc_send_response("ERROR|Invalid SET_MACRO_SEQ format");
    return;
  }
  size_t emoji_len = end_emoji - token;
  emoji_index = atoi(token);
  token = end_emoji + 1;

  step_count = atoi(token);
  token = strchr(token, '|');
  if (!token)
    return;
  token++;

  config_data_t *config = config_get();
  macro_entry_t *macro = &config->macros[layer][button];

  macro->type = MACRO_TYPE_KEY_SEQUENCE;
  macro->sequence_length = step_count;
  strncpy(macro->name, name, MAX_NAME_LEN - 1);
  macro->emoji_index = emoji_index;

  args = token; // start of the sequence data (steps)

  for (int i = 0; i < step_count; i++) {
    int keycode = 0, mods = 0, duration = 0;
    int chars_read = 0;

    // %n to know how many characters to move
    if (sscanf(args, "%d,%d,%d%n", &keycode, &mods, &duration, &chars_read) >=
        2) {
      macro->sequence[i].keycode = (uint8_t)keycode;
      macro->sequence[i].modifiers = (uint8_t)mods;
      macro->sequence[i].duration = (uint16_t)duration;

      printf("[CDC] Step %d: k=%d m=%d d=%d\n", i, keycode, mods, duration);

      // moving pointer for the offest: readed data + 1 (for comma)
      args += chars_read;
      if (*args == ',')
        args++;
    } else {
      printf("[CDC] Error parsing step %d\n", i);
      break;
    }
  }

  cdc_send_response("OK");
}

void cmd_handle_set_macro_script(char *args) {

  int layer, button, platform, size, shortcut_len;
  key_step_t temp_shortcut[MAX_SEQUENCE_STEPS];
  memset(temp_shortcut, 0, sizeof(temp_shortcut));

  char *token = args;

  layer = atoi(token);
  token = strchr(token, '|');
  if (!token) {
    cdc_send_response("ERROR|Invalid format");
    return;
  }
  token++;

  button = atoi(token);
  token = strchr(token, '|');
  if (!token) {
    cdc_send_response("ERROR|Invalid format");
    return;
  }
  token++;

  platform = atoi(token);
  token = strchr(token, '|');
  if (!token) {
    cdc_send_response("ERROR|Invalid format");
    return;
  }
  token++;

  size = atoi(token);
  token = strchr(token, '|');
  if (!token) {
    cdc_send_response("ERROR|Invalid format");
    return;
  }
  token++;

  shortcut_len = atoi(token);
  if (shortcut_len > MAX_SEQUENCE_STEPS)
    shortcut_len = MAX_SEQUENCE_STEPS;

  // format: k,m,k,m...
  char *step_token = strchr(token, '|');
  if (step_token && shortcut_len > 0) {
    step_token++;
    for (int i = 0; i < shortcut_len; i++) {
      int k, m;
      if (sscanf(step_token, "%d,%d", &k, &m) == 2) {
        temp_shortcut[i].keycode = (uint8_t)k;
        temp_shortcut[i].modifiers = (uint8_t)m;

        step_token = strchr(step_token, ',');
        if (step_token)
          step_token = strchr(step_token + 1, ',');
        if (step_token)
          step_token++;
      }
    }
  }

  // validation and assignment
  if (layer >= 0 && layer < MAX_LAYERS && button >= 0 && button < NUM_BUTTONS &&
      platform >= 0 && platform <= 2 && size > 0 &&
      size < MAX_SCRIPT_SIZE) { // leave room for null terminator

    config_data_t *config = config_get();
    macro_entry_t *macro = &config->macros[layer][button];

    macro->terminal_shortcut_length = shortcut_len;
    for (int i = 0; i < MAX_SEQUENCE_STEPS; i++) {
      macro->terminal_shortcut[i] = temp_shortcut[i];
    }

    // flush any pending data in RX buffer to ensure clean state
    cdc_flush_rx();

    // clear the script buffer to prevent garbage from previous runs
    memset(macro->script, 0, MAX_SCRIPT_SIZE);

    cdc_send_response("READY");
    cdc_receive_script(layer, button, platform, size);
  } else {
    cdc_send_response("ERROR|Invalid parameters");
  }
}
