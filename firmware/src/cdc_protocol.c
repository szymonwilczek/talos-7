#include "cdc_protocol.h"
#include "macro_config.h"
#include "tusb.h"
#include <stdarg.h>
#include <stdio.h>
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

    // nazwy i emotki warstw
    for (int layer = 0; layer < MAX_LAYERS; layer++) {
      cdc_send_response_fmt("LAYER_NAME|%d|%s|%s", layer,
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
          // tylko metadane, skrypt w nastepnej linii
          cdc_send_response_fmt(
              "MACRO|%d|%d|%d|%d|%s|%s|%s|%d|%d", layer, btn, macro->type,
              macro->value, macro->macro_string, macro->name, macro->emoji,
              macro->script_platform, (int)strlen(macro->script));

          // skrypt jako osobna linia (escaped)
          printf("SCRIPT_DATA|");
          for (size_t i = 0; i < strlen(macro->script); i++) {
            char c = macro->script[i];
            if (c == '\n') {
              printf("\\n");
            } else if (c == '\r') {
              printf("\\r");
            } else if (c == '|') {
              printf("\\|");
            } else if (c == '\\') {
              printf("\\\\");
            } else {
              printf("%c", c);
            }
          }
          printf("\r\n");
          tud_cdc_write_flush();
        } else {
          cdc_send_response_fmt("MACRO|%d|%d|%d|%d|%s|%s|%s", layer, btn,
                                macro->type, macro->value, macro->macro_string,
                                macro->name, macro->emoji);
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

  // SET_MACRO|layer|button|type|value|string|name|emoji
  if (strncmp(cmd_ptr, "SET_MACRO|", 10) == 0) {
    int layer, button, type, value;
    char macro_string[MACRO_STRING_LEN] = {0};
    char name[MAX_NAME_LEN] = {0};
    char emoji[MAX_EMOJI_LEN] = {0};

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

      // emoji
      strncpy(emoji, token, MAX_EMOJI_LEN - 1);
      emoji[MAX_EMOJI_LEN - 1] = '\0';
    } else {
      // backward compatibility: jesli brak emoji, same name
      strncpy(name, token, MAX_NAME_LEN - 1);
      name[MAX_NAME_LEN - 1] = '\0';
    }

    // walidacja
    if (layer >= 0 && layer < MAX_LAYERS && button >= 0 &&
        button < NUM_BUTTONS) {

      macro_entry_t *macro = &config->macros[layer][button];
      macro->type = type;
      macro->value = value;
      strncpy(macro->macro_string, macro_string, MACRO_STRING_LEN - 1);
      strncpy(macro->name, name, MAX_NAME_LEN - 1);
      strncpy(macro->emoji, emoji, MAX_EMOJI_LEN - 1);

      cdc_send_response("OK");
      printf("[CDC] Macro set: L%d B%d '%s' %s\n", layer, button, name, emoji);
    } else {
      cdc_send_response("ERROR|Invalid parameters");
    }
    return;
  }

  // SET_LAYER_NAME|layer|name|emoji
  if (strncmp(cmd_ptr, "SET_LAYER_NAME|", 15) == 0) {
    int layer;
    char name[MAX_NAME_LEN] = {0};
    char emoji[MAX_EMOJI_LEN] = {0};

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

      // emoji
      strncpy(emoji, token, MAX_EMOJI_LEN - 1);
      emoji[MAX_EMOJI_LEN - 1] = '\0';
    } else {
      // backward compatibility
      strncpy(name, token, MAX_NAME_LEN - 1);
      name[MAX_NAME_LEN - 1] = '\0';
    }

    if (layer >= 0 && layer < MAX_LAYERS) {
      strncpy(config->layer_names[layer], name, MAX_NAME_LEN - 1);
      strncpy(config->layer_emojis[layer], emoji, MAX_EMOJI_LEN - 1);
      cdc_send_response("OK");
      printf("[CDC] Layer name set: L%d = %s %s\n", layer, emoji, name);
    } else {
      cdc_send_response("ERROR|Invalid parameters");
    }
    return;
  }

  // SET_SCRIPT|layer|button|platform|size
  // nastepnie wyslane sa surowe bajty skryptu
  if (strncmp(cmd_ptr, "SET_SCRIPT|", 11) == 0) {
    int layer, button, platform, size;

    char *token = cmd_ptr + 11;

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

    if (layer >= 0 && layer < MAX_LAYERS && button >= 0 &&
        button < NUM_BUTTONS && platform >= 0 && platform <= 2 && size > 0 &&
        size <= MAX_SCRIPT_SIZE) {

      cdc_send_response("READY"); // gotowy na odbior
      cdc_receive_script(layer, button, platform, size);
    } else {
      cdc_send_response("ERROR|Invalid parameters");
    }
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
