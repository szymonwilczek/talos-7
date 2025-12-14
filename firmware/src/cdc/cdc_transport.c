#include "cdc/cdc_transport.h"
#include "cdc/cdc_dispatcher.h"
#include "tusb.h"
#include <stdarg.h>

static char cmd_buffer[CDC_MAX_COMMAND_LEN];
static uint16_t cmd_buffer_pos = 0;
static volatile bool cdc_binary_mode = false;

void cdc_set_binary_mode(bool enabled) { cdc_binary_mode = enabled; }

bool cdc_is_binary_mode(void) { return cdc_binary_mode; }

void cdc_protocol_init(void) {
  cmd_buffer_pos = 0;
  cdc_binary_mode = false;
  memset(cmd_buffer, 0, sizeof(cmd_buffer));
  printf("[CDC] Protocol initialized\n");
}

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

  char buffer[512];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  cdc_send_response(buffer);
}

void cdc_protocol_task(void) {
  if (!tud_cdc_connected()) {
    return;
  }

  if (cdc_binary_mode) {
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

void cdc_log(const char *format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  printf("%s", buffer);
}
