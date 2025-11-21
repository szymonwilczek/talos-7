#include "cdc_protocol.h"
#include "hardware_interface.h"
#include "macro_config.h"
#include "tusb.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==================== PRYWATNE ZMIENNE ====================
static char cmd_buffer[CDC_MAX_COMMAND_LEN];
static uint16_t cmd_buffer_pos = 0;

// ==================== WYSYŁANIE ODPOWIEDZI ====================
void cdc_send_response(const char *response) {
  if (tud_cdc_connected()) {
    tud_cdc_write_str(response);
    tud_cdc_write_str("\r\n");
    tud_cdc_write_flush();

    uint32_t timeout = 100; // 100ms timeout
    uint32_t start = time_us_32();
    while (tud_cdc_write_available() < CFG_TUD_CDC_TX_BUFSIZE &&
           (time_us_32() - start) < (timeout * 1000)) {
      tud_task();
    }
  }
}

void cdc_send_response_fmt(const char *format, ...) {
  if (!tud_cdc_connected())
    return;

  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  cdc_send_response(buffer);
}

// ==================== ODEBRANIE SKRYPTU ====================

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

// ==================== PARSOWANIE KOMEND ====================

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

static void process_command(const char *cmd_input) {
  printf("[CDC] Command received: %s\n", cmd_input);

  // lokalna kopia do modyfikacji
  char cmd[CDC_MAX_COMMAND_LEN];
  strncpy(cmd, cmd_input, CDC_MAX_COMMAND_LEN - 1);
  cmd[CDC_MAX_COMMAND_LEN - 1] = '\0';

  // usuniecie whitespace z poczatku
  char *cmd_ptr = cmd;
  while (*cmd_ptr == ' ' || *cmd_ptr == '\t' || *cmd_ptr == '\r' ||
         *cmd_ptr == '\n') {
    cmd_ptr++;
  }

  // usuniecie whitespace z konca
  size_t len = strlen(cmd_ptr);
  while (len > 0 && (cmd_ptr[len - 1] == ' ' || cmd_ptr[len - 1] == '\t' ||
                     cmd_ptr[len - 1] == '\r' || cmd_ptr[len - 1] == '\n')) {
    cmd_ptr[len - 1] = '\0';
    len--;
  }

  // debug log: TODO usunac pozniej
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

  // GET_CONF
  if (strcmp(cmd_ptr, "GET_CONF") == 0) {
    printf("[CDC] GET_CONF command received\n");

    cdc_send_response("CONF_START");
    printf("[CDC] Sent CONF_START\n");
    tud_cdc_write_flush();

    cdc_send_response_fmt("SETTINGS|%lu", config->oled_timeout_s);

    // nazwy i emotki warstw
    for (int layer = 0; layer < MAX_LAYERS; layer++) {
      cdc_send_response_fmt("LAYER_NAME|%d|%s|%d", layer,
                            config->layer_names[layer],
                            config->layer_emojis[layer]);
      tud_cdc_write_flush();
    }
    printf("[CDC] Sent %d layer names\n", MAX_LAYERS);

    // wszystkie makra
    int macro_count = 0;
    for (int layer = 0; layer < MAX_LAYERS; layer++) {
      for (int btn = 0; btn < NUM_BUTTONS; btn++) {
        macro_entry_t *macro = &config->macros[layer][btn];
        if (macro->type == MACRO_TYPE_SCRIPT) {
          // tylko metadane
          cdc_send_response_fmt("MACRO|%d|%d|%d|%d|%s|%s|%d|%d|%d", layer, btn,
                                macro->type, macro->value, macro->macro_string,
                                macro->name, macro->emoji_index,
                                macro->script_platform,
                                macro->terminal_shortcut_length);

          for (int i = 0; i < macro->terminal_shortcut_length; i++) {
            cdc_send_response_fmt("SCRIPT_SHORTCUT|%d|%d|%d|%d|%d", layer, btn,
                                  i, macro->terminal_shortcut[i].keycode,
                                  macro->terminal_shortcut[i].modifiers);
          }

          // PELNA tresc skryptu strumieniowo
          // najpierw naglowek linii
          char header[64];
          snprintf(header, sizeof(header), "SCRIPT_DATA|%d|%d|%d|", layer, btn,
                   macro->script_platform);
          tud_cdc_write_str(header);

          // tresc znak po znaku bezposrednio do usb
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

            // flushowanie bufora raz na jakis czas zeby nie zapchac FIFO
            // TinyUSB
            if (i % 64 == 0)
              tud_task();
          }

          // koniec linii SCRIPT_DATA
          tud_cdc_write_str("\r\n");
          tud_cdc_write_flush();
        } else if (macro->type == MACRO_TYPE_KEY_SEQUENCE &&
                   macro->sequence_length > 0) {
          cdc_send_response_fmt("MACRO_SEQ|%d|%d|%s|%d|%d", layer, btn,
                                macro->name, macro->emoji_index,
                                macro->sequence_length);

          for (int i = 0; i < macro->sequence_length; i++) {
            cdc_send_response_fmt("SEQ_STEP|%d|%d|%d|%d|%d", layer, btn, i,
                                  macro->sequence[i].keycode,
                                  macro->sequence[i].modifiers);
          }
        } else {
          cdc_send_response_fmt("MACRO|%d|%d|%d|%d|%s|%s|%d", layer, btn,
                                macro->type, macro->value, macro->macro_string,
                                macro->name, macro->emoji_index);
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
    return;
  }

  // SET_OLED_TIMEOUT|seconds
  if (strncmp(cmd_ptr, "SET_OLED_TIMEOUT|", 17) == 0) {
    char *token = cmd_ptr + 17;
    uint32_t timeout = strtoul(token, NULL, 10);

    config_data_t *config = config_get();
    config->oled_timeout_s = timeout;

    cdc_send_response("OK");
    return;
  }

  // SET_MACRO|layer|button|type|value|string|name|emoji_index
  if (strncmp(cmd_ptr, "SET_MACRO|", 10) == 0) {
    int layer, button, type, value;
    char macro_string[MACRO_STRING_LEN] = {0};
    char name[MAX_NAME_LEN] = {0};
    uint8_t emoji_index = 0;

    char *token = cmd_ptr + 10;

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
      // backward compatibility: jesli brak emoji_index, same name
      strncpy(name, token, MAX_NAME_LEN - 1);
      name[MAX_NAME_LEN - 1] = '\0';
      emoji_index = 0; // domyslny
    }

    // walidacja
    if (layer >= 0 && layer < MAX_LAYERS && button >= 0 &&
        button < NUM_BUTTONS) {

      macro_entry_t *macro = &config->macros[layer][button];
      macro->type = type;
      macro->value = value;
      strncpy(macro->macro_string, macro_string, MACRO_STRING_LEN - 1);
      strncpy(macro->name, name, MAX_NAME_LEN - 1);
      macro->emoji_index = emoji_index;

      cdc_send_response("OK");
      printf("[CDC] Macro set: L%d B%d '%s' %d\n", layer, button, name,
             emoji_index);
    } else {
      cdc_send_response("ERROR|Invalid parameters");
    }
    return;
  }

  // SET_LAYER_NAME|layer|name|emoji_index
  if (strncmp(cmd_ptr, "SET_LAYER_NAME|", 15) == 0) {
    int layer;
    char name[MAX_NAME_LEN] = {0};
    uint8_t emoji_index = 0;

    char *token = cmd_ptr + 15;

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

      // emoji_index
      emoji_index = atoi(token);
    } else {
      // backward compatibility
      strncpy(name, token, MAX_NAME_LEN - 1);
      name[MAX_NAME_LEN - 1] = '\0';
      emoji_index = 0; // domyslny
    }

    if (layer >= 0 && layer < MAX_LAYERS) {
      strncpy(config->layer_names[layer], name, MAX_NAME_LEN - 1);
      config->layer_emojis[layer] = emoji_index;
      cdc_send_response("OK");
      printf("[CDC] Layer name set: L%d = %d %s\n", layer, emoji_index, name);
    } else {
      cdc_send_response("ERROR|Invalid parameters");
    }
    return;
  }

  // SET_MACRO_SCRIPT|layer|button|platform|size
  // nastepnie wyslane sa surowe bajty skryptu
  if (strncmp(cmd_ptr, "SET_MACRO_SCRIPT|", 17) == 0) {
    int layer, button, platform, size, shortcut_len;
    char terminal_cmd[64] = {0};

    char *token = cmd_ptr + 17;

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

    // parsowanie dlugosci skrotu
    shortcut_len = atoi(token);
    if (shortcut_len > MAX_SEQUENCE_STEPS)
      shortcut_len = MAX_SEQUENCE_STEPS;

    config_data_t *config = config_get();
    macro_entry_t *macro = &config->macros[layer][button];
    macro->terminal_shortcut_length = shortcut_len;

    // parsowanie krokow skrotu (format: k,m,k,m...)
    char *step_token = strchr(token, '|'); // szukanie poczatku sekwencji
    if (step_token && shortcut_len > 0) {
      step_token++; // pomin '|'
      for (int i = 0; i < shortcut_len; i++) {
        int k, m;
        if (sscanf(step_token, "%d,%d", &k, &m) == 2) {
          macro->terminal_shortcut[i].keycode = (uint8_t)k;
          macro->terminal_shortcut[i].modifiers = (uint8_t)m;

          // przesun do nastepnej pary
          step_token = strchr(step_token, ','); // za k
          if (step_token)
            step_token =
                strchr(step_token + 1, ','); // za m (separator kolejnej pary)
          if (step_token)
            step_token++;
        }
      }
    }

    if (layer >= 0 && layer < MAX_LAYERS && button >= 0 &&
        button < NUM_BUTTONS && platform >= 0 && platform <= 2 && size > 0 &&
        size <= MAX_SCRIPT_SIZE) {

      cdc_send_response("READY"); // gotowy na odbior bajtow skryptu
      cdc_receive_script(layer, button, platform, size);
    } else {
      cdc_send_response("ERROR|Invalid parameters");
    }
    return;
  }

  if (strcmp(cmd_ptr, "RELOAD_CONFIG") == 0) {
    printf("[CDC] RELOAD_CONFIG command received\n");
    config_init();
    oled_display_layer_info(0);
    cdc_send_response("OK");
    printf("[CDC] Config reloaded\n");
    config_mode = 0; // wyjscie z trybu konfiguracji
    return;
  }

  if (strncmp(cmd_ptr, "SET_CONFIG_MODE|", 16) == 0) {
    char *token = cmd_ptr + 16;
    uint8_t val = atoi(token);

    config_mode = val;

    // wybudz ekran i zresetuj timer przy kazdej zmianie trybu
    // jesli wchodzimy w config -> ekran sie wlacza
    // jesli wychodzimy -> resetujemy timer, zeby nie zgasl od razu
    oled_wake_up();

    if (config_mode) {
      cdc_log("[CDC] Config mode ENABLED\n");
      oled_display_layer_info(
          config_get_current_layer()); // odswiez zeby pokazac setup screen
    } else {
      cdc_log("[CDC] Config mode DISABLED\n");
      // powrot do normalnego ekranu
      oled_display_layer_info(config_get_current_layer());
    }

    cdc_send_response("OK");
    return;
  }

  if (strncmp(cmd_ptr, "SET_MACRO_SEQ|", 14) == 0) {
    cmd_ptr += 14;

    int layer, button, step_count;
    char name[MAX_NAME_LEN] = {0};
    uint8_t emoji_index = 0;

    char *token = cmd_ptr;

    // layer
    layer = atoi(token);
    token = strchr(token, '|');
    if (!token) {
      cdc_send_response("ERROR|Invalid SET_MACRO_SEQ format");
      return;
    }
    token++;

    // button
    button = atoi(token);
    token = strchr(token, '|');
    if (!token) {
      cdc_send_response("ERROR|Invalid SET_MACRO_SEQ format");
      return;
    }
    token++;

    // name
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

    // step_count
    step_count = atoi(token);
    token = strchr(token, '|');
    if (!token) {
      cdc_send_response("ERROR|Invalid SET_MACRO_SEQ format");
      return;
    }
    token++;

    if (step_count < 0 || step_count > MAX_SEQUENCE_STEPS) {
      cdc_send_response("ERROR|Invalid step count");
      return;
    }

    // kroki
    config_data_t *config = config_get();
    macro_entry_t *macro = &config->macros[layer][button];

    macro->type = MACRO_TYPE_KEY_SEQUENCE;
    macro->sequence_length = step_count;
    strncpy(macro->name, name, MAX_NAME_LEN - 1);
    macro->emoji_index = emoji_index;

    cmd_ptr = token; // cmd_ptr na poczatek steps

    for (int i = 0; i < step_count; i++) {
      int keycode, mods;
      if (sscanf(cmd_ptr, "%d,%d", &keycode, &mods) == 2) {
        macro->sequence[i].keycode = (uint8_t)keycode;
        macro->sequence[i].modifiers = (uint8_t)mods;
        printf("[CDC] Step %d: keycode=%d mods=0x%02X\n", i, keycode, mods);

        // nastepny krok
        cmd_ptr = strchr(cmd_ptr, ',');
        if (cmd_ptr)
          cmd_ptr++;
        cmd_ptr = strchr(cmd_ptr, ',');
        if (cmd_ptr)
          cmd_ptr++;
      } else {
        cdc_send_response("ERROR|Invalid step format");
        return;
      }
    }

    cdc_send_response("OK");
    return;
  }

  // SAVE_FLASH
  if (strcmp(cmd_ptr, "SAVE_FLASH") == 0) {
    if (config_save()) {
      cdc_send_response("OK");
      printf("[CDC] Configuration saved to flash\n");
    } else {
      cdc_send_response("ERROR|Flash write failed");
    }
    return;
  }

  // Unknown command
  printf("[CDC] Unknown command: '%s'\n", cmd_ptr);
  cdc_send_response("ERROR|Unknown command");
}

// ==================== INICJALIZACJA ====================
void cdc_protocol_init(void) {
  cmd_buffer_pos = 0;
  memset(cmd_buffer, 0, sizeof(cmd_buffer));
  printf("[CDC] Protocol initialized\n");
}

// ==================== GŁÓWNA PĘTLA ====================
void cdc_protocol_task(void) {
  if (!tud_cdc_connected()) {
    return;
  }

  // available characters
  while (tud_cdc_available()) {
    char c = tud_cdc_read_char();

    // line endings
    if (c == '\n' || c == '\r') {
      if (cmd_buffer_pos > 0) {
        cmd_buffer[cmd_buffer_pos] = '\0';
        process_command(cmd_buffer);
        cmd_buffer_pos = 0;
        memset(cmd_buffer, 0, sizeof(cmd_buffer));
      }
    }
    // buffer character
    else if (cmd_buffer_pos < CDC_MAX_COMMAND_LEN - 1) {
      cmd_buffer[cmd_buffer_pos++] = c;
    }
    // buffer overflow
    else {
      cdc_send_response("ERROR|Command too long");
      cmd_buffer_pos = 0;
      memset(cmd_buffer, 0, sizeof(cmd_buffer));
    }
  }
}

// ==================== LOGOWANIE ====================
void cdc_log(const char *format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  printf("%s", buffer);
}
