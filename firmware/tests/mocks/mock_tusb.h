/*
 * mock for TinyUSB
 */

#ifndef MOCK_TUSB_H
#define MOCK_TUSB_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// CDC mock state
extern char mock_cdc_output[4096];
extern int mock_cdc_output_pos;
extern char mock_cdc_input[4096];
extern int mock_cdc_input_pos;
extern int mock_cdc_input_len;

// TUD CDC functions
static inline bool tud_cdc_connected(void) { return true; }

static inline uint32_t tud_cdc_available(void) {
  return mock_cdc_input_len - mock_cdc_input_pos;
}

static inline char tud_cdc_read_char(void) {
  if (mock_cdc_input_pos < mock_cdc_input_len) {
    return mock_cdc_input[mock_cdc_input_pos++];
  }
  return 0;
}

static inline void tud_cdc_write_str(const char *str) {
  int len = strlen(str);
  if (mock_cdc_output_pos + len < (int)sizeof(mock_cdc_output)) {
    strcpy(&mock_cdc_output[mock_cdc_output_pos], str);
    mock_cdc_output_pos += len;
  }
}

static inline void tud_cdc_write_char(char c) {
  if (mock_cdc_output_pos < (int)sizeof(mock_cdc_output) - 1) {
    mock_cdc_output[mock_cdc_output_pos++] = c;
    mock_cdc_output[mock_cdc_output_pos] = '\0';
  }
}

static inline uint32_t tud_cdc_write_available(void) { return 256; }
static inline void tud_cdc_write_flush(void) {}
static inline void tud_task(void) {}
static inline void tusb_init(void) {}

// HID
#define HID_KEY_A 0x04
#define HID_KEY_Z 0x1D

static inline bool tud_hid_ready(void) { return true; }
static inline bool tud_hid_keyboard_report(uint8_t id, uint8_t mod,
                                           uint8_t keys[6]) {
  (void)id;
  (void)mod;
  (void)keys;
  return true;
}

// MIDI
static inline bool tud_midi_mounted(void) { return true; }
static inline uint32_t tud_midi_stream_write(uint8_t cable, uint8_t *buf,
                                             uint32_t len) {
  (void)cable;
  (void)buf;
  return len;
}

// HID mouse
static inline bool tud_hid_mouse_report(uint8_t id, uint8_t btns, int8_t x,
                                        int8_t y, int8_t v, int8_t h) {
  (void)id;
  (void)btns;
  (void)x;
  (void)y;
  (void)v;
  (void)h;
  return true;
}

// CFG constants
#define CFG_TUD_CDC_TX_BUFSIZE 256

// helper to setup mock CDC input
static inline void mock_cdc_set_input(const char *input) {
  strcpy(mock_cdc_input, input);
  mock_cdc_input_len = strlen(input);
  mock_cdc_input_pos = 0;
}

// helper to reset mock CDC output
static inline void mock_cdc_reset_output(void) {
  memset(mock_cdc_output, 0, sizeof(mock_cdc_output));
  mock_cdc_output_pos = 0;
}

#endif // MOCK_TUSB_H
