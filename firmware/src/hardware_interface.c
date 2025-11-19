#include "hardware_interface.h"
#include "cdc_protocol.h"
#include "macro_config.h"
#include "pico/stdlib.h"
#include "pin_definitions.h"
#include <stdio.h>
#include <string.h>

// SSD1306 OLED SPI
#include "hardware/spi.h"

// ==================== OLED SSD1306 CONSTANTS ====================
#define OLED_SPI spi0
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

uint8_t config_mode = 0;

// SSD1306 commands
#define OLED_CMD_SET_CONTRAST 0x81
#define OLED_CMD_DISPLAY_ALL_ON_RESUME 0xA4
#define OLED_CMD_DISPLAY_ALL_ON 0xA5
#define OLED_CMD_NORMAL_DISPLAY 0xA6
#define OLED_CMD_INVERT_DISPLAY 0xA7
#define OLED_CMD_DISPLAY_OFF 0xAE
#define OLED_CMD_DISPLAY_ON 0xAF
#define OLED_CMD_SET_DISPLAY_OFFSET 0xD3
#define OLED_CMD_SET_COM_PINS 0xDA
#define OLED_CMD_SET_VCOM_DETECT 0xDB
#define OLED_CMD_SET_DISPLAY_CLOCK_DIV 0xD5
#define OLED_CMD_SET_PRECHARGE 0xD9
#define OLED_CMD_SET_MULTIPLEX 0xA8
#define OLED_CMD_SET_LOW_COLUMN 0x00
#define OLED_CMD_SET_HIGH_COLUMN 0x10
#define OLED_CMD_SET_START_LINE 0x40
#define OLED_CMD_MEMORY_MODE 0x20
#define OLED_CMD_COLUMN_ADDR 0x21
#define OLED_CMD_PAGE_ADDR 0x22
#define OLED_CMD_COM_SCAN_INC 0xC0
#define OLED_CMD_COM_SCAN_DEC 0xC8
#define OLED_CMD_SEG_REMAP 0xA0
#define OLED_CMD_CHARGE_PUMP 0x8D
#define OLED_CMD_EXTERNAL_VCC 0x01
#define OLED_CMD_SWITCH_CAP_VCC 0x02

// font 5x7
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // backslash
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // }
    {0x10, 0x08, 0x08, 0x10, 0x08}, // ~
    {0x00, 0x00, 0x00, 0x00, 0x00}, // DEL
};

static const uint8_t emoji_bitmaps[18][8] = {
    // 0: 🎮 (controller)
    {0x38, 0x44, 0x94, 0x44, 0x46, 0x95, 0x44, 0x38},
    // 1: 💼 (briefcase)
    {0x7c, 0x46, 0x4a, 0x5a, 0x5a, 0x4a, 0x46, 0x7c},
    // 2: 🏠 (home)
    {0x10, 0xf8, 0x8c, 0xe6, 0x86, 0x8c, 0xf8, 0x10},
    // 3: 🔧 (settings - wrench)
    {0x00, 0x00, 0xc7, 0x7c, 0x7c, 0xc7, 0x00, 0x00},
    // 4: ⚡ (lightning)
    {0x00, 0x80, 0xc8, 0x6c, 0x3e, 0x1b, 0x09, 0x00},
    // 5: 📧 (mail)
    {0x00, 0x3e, 0x41, 0x5d, 0x51, 0x4e, 0x20, 0x00},
    // 6: 💻 (computer)
    {0x3e, 0x22, 0xa2, 0xe2, 0xe2, 0xa2, 0x22, 0x3e},
    // 7: 🎵 (music note)
    {0xc0, 0xfc, 0xc2, 0x02, 0x02, 0x62, 0x7e, 0x60},
    // 8: 📝 (note)
    {0xc0, 0xa0, 0x50, 0x28, 0x1c, 0x0a, 0x07, 0x03},
    // 9: ☕ (coffee cup)
    {0x7e, 0xc2, 0xc2, 0xc2, 0xc2, 0xfe, 0x64, 0x3c},
    // 10: 🗡️ (sword)
    {0xd8, 0xf0, 0x78, 0x7c, 0x5e, 0x0f, 0x07, 0x03},
    // 11: ❤️ (heart)
    {0x00, 0x0c, 0x12, 0x22, 0x44, 0x22, 0x12, 0x0c},
    // 12: 🔔 (bell)
    {0x40, 0x7c, 0xc6, 0xc6, 0x7c, 0x40, 0x00, 0x00},
    // 13: 🧪 (lab probe)
    {0x40, 0xa0, 0x91, 0x8f, 0x8f, 0x91, 0xa0, 0x40},
    // 14: 🔒 (lock)
    {0x78, 0xfe, 0xf9, 0xc9, 0xf9, 0xfe, 0x78, 0x00},
    // 15: ☂️ (umbrella)
    {0x0c, 0x0e, 0x4f, 0x8f, 0x7f, 0x0f, 0x0e, 0x0c},
    // 16: 🦕 (dinosaur)
    {0x06, 0x15, 0xff, 0x7d, 0xff, 0x40, 0x30, 0x00},
    // 17: 👻 (ghost)
    {0x3e, 0x45, 0x47, 0xf5, 0xcf, 0x83, 0x82, 0x84},
};

static const char *emoji_strings[19] = {"🎮", "💼", "🏠", "⚡", "📧", "💻",
                                        "🎵", "📝", "☕", "🗡️", "❤️",  "🔔",
                                        "🧪", "🔒", "☂️",  "🦕", "👻", "🔧"};

static uint8_t oled_buffer[OLED_WIDTH * OLED_HEIGHT / 8];

// button states
static bool button_states[NUM_BUTTONS] = {false};
static bool button_prev_states[NUM_BUTTONS] = {false};

// LED states
static bool led_states[NUM_BUTTONS] = {false};

// ==================== OLED LOW-LEVEL ====================

static void oled_write_cmd(uint8_t cmd) {
  gpio_put(OLED_DC_PIN, 0); // command mode
  gpio_put(OLED_CS_PIN, 0);
  spi_write_blocking(OLED_SPI, &cmd, 1);
  gpio_put(OLED_CS_PIN, 1);
}

static void oled_write_data(const uint8_t *data, size_t len) {
  gpio_put(OLED_DC_PIN, 1); // data mode
  gpio_put(OLED_CS_PIN, 0);
  spi_write_blocking(OLED_SPI, data, len);
  gpio_put(OLED_CS_PIN, 1);
}

void oled_init(void) {
  // SPI init
  spi_init(OLED_SPI, 8 * 1000 * 1000); // 8MHz
  gpio_set_function(OLED_MOSI_PIN, GPIO_FUNC_SPI);
  gpio_set_function(OLED_SCK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(OLED_CS_PIN, GPIO_FUNC_SPI);
  gpio_set_function(OLED_DC_PIN, GPIO_FUNC_SPI);
  gpio_set_function(OLED_RST_PIN, GPIO_FUNC_SPI);

  // CS jako output
  gpio_init(OLED_CS_PIN);
  gpio_set_dir(OLED_CS_PIN, GPIO_OUT);
  gpio_put(OLED_CS_PIN, 1);

  // DC jako output
  gpio_init(OLED_DC_PIN);
  gpio_set_dir(OLED_DC_PIN, GPIO_OUT);

  // RST jako output
  gpio_init(OLED_RST_PIN);
  gpio_set_dir(OLED_RST_PIN, GPIO_OUT);

  sleep_ms(100);

  // reset OLED
  gpio_put(OLED_RST_PIN, 0);
  sleep_ms(10);
  gpio_put(OLED_RST_PIN, 1);

  cdc_log("[OLED] SPI initialized\n");

  // init sequence
  oled_write_cmd(OLED_CMD_DISPLAY_OFF);
  oled_write_cmd(OLED_CMD_SET_DISPLAY_CLOCK_DIV);
  oled_write_cmd(0x80);
  oled_write_cmd(OLED_CMD_SET_MULTIPLEX);
  oled_write_cmd(0x3F);
  oled_write_cmd(OLED_CMD_SET_DISPLAY_OFFSET);
  oled_write_cmd(0x00);
  oled_write_cmd(OLED_CMD_SET_START_LINE | 0x00);
  oled_write_cmd(OLED_CMD_CHARGE_PUMP);
  oled_write_cmd(0x14);
  oled_write_cmd(OLED_CMD_MEMORY_MODE);
  oled_write_cmd(0x00);
  oled_write_cmd(OLED_CMD_SEG_REMAP); // bez | 0x01 (odwrocony: TODO ZMIENIC PO
                                      // SKONCZENIU PROJEKTU)
  oled_write_cmd(OLED_CMD_COM_SCAN_INC); // zamiast DEC (odwrocony)
  oled_write_cmd(OLED_CMD_SET_COM_PINS);
  oled_write_cmd(0x12);
  oled_write_cmd(OLED_CMD_SET_CONTRAST);
  oled_write_cmd(0xCF);
  oled_write_cmd(OLED_CMD_SET_PRECHARGE);
  oled_write_cmd(0xF1);
  oled_write_cmd(OLED_CMD_SET_VCOM_DETECT);
  oled_write_cmd(0x40);
  oled_write_cmd(OLED_CMD_DISPLAY_ALL_ON_RESUME);
  oled_write_cmd(OLED_CMD_NORMAL_DISPLAY);
  oled_write_cmd(OLED_CMD_DISPLAY_ON);

  oled_clear();
  oled_update();
  cdc_log("[OLED] Initialized (128x64)\n");
}

void oled_clear(void) { memset(oled_buffer, 0, sizeof(oled_buffer)); }

static void oled_draw_char(uint8_t x, uint8_t y, char c) {
  const uint8_t *glyph = font5x7[0]; // default space
  if (c >= ' ' && c <= '~') {
    glyph = font5x7[c - ' '];
  }

  for (int i = 0; i < 5; i++) {
    if (x + i >= OLED_WIDTH)
      break;
    for (int j = 0; j < 8; j++) {
      if (y + j >= OLED_HEIGHT)
        break;
      if (glyph[i] & (1 << j)) {
        int byte_idx = (y / 8) * OLED_WIDTH + x + i;
        int bit_idx = y % 8 + j;
        if (bit_idx < 8 && byte_idx < sizeof(oled_buffer)) {
          oled_buffer[byte_idx] |= (1 << bit_idx);
        }
      }
    }
  }
}

static void oled_draw_string(uint8_t x, uint8_t y, const char *str) {
  while (*str) {
    oled_draw_char(x, y, *str);
    x += 6; // 5 pixels + 1 space
    if (x >= OLED_WIDTH - 6)
      break; // prevent overflow
    str++;
  }
}

static void oled_draw_line(uint8_t y) {
  for (int x = 0; x < OLED_WIDTH; x++) {
    int byte_idx = (y / 8) * OLED_WIDTH + x;
    int bit_idx = y % 8;
    if (byte_idx < sizeof(oled_buffer)) {
      oled_buffer[byte_idx] |= (1 << bit_idx);
    }
  }
}

void oled_draw_bitmap(uint8_t x, uint8_t y, uint8_t width, uint8_t height,
                      const uint8_t *bitmap) {
  for (uint8_t i = 0; i < width; i++) {
    for (uint8_t j = 0; j < height; j++) {
      if (bitmap[i] & (1 << j)) {
        int byte_idx = (y / 8) * OLED_WIDTH + x + i;
        int bit_idx = y % 8 + j;
        if (bit_idx < 8 && byte_idx < sizeof(oled_buffer)) {
          oled_buffer[byte_idx] |= (1 << bit_idx);
        }
      }
    }
  }
}

void oled_draw_emoji(uint8_t x, uint8_t y, uint8_t emoji_index) {
  if (emoji_index >= 18)
    emoji_index = 0; // fallback
  oled_draw_bitmap(x, y, 8, 8, emoji_bitmaps[emoji_index]);
}

void oled_update(void) {
  oled_write_cmd(OLED_CMD_COLUMN_ADDR);
  oled_write_cmd(0);
  oled_write_cmd(OLED_WIDTH - 1);
  oled_write_cmd(OLED_CMD_PAGE_ADDR);
  oled_write_cmd(0);
  oled_write_cmd((OLED_HEIGHT / 8) - 1);

  // send in chunks
  for (size_t i = 0; i < sizeof(oled_buffer); i += 16) {
    size_t chunk_size =
        (i + 16 > sizeof(oled_buffer)) ? sizeof(oled_buffer) - i : 16;
    oled_write_data(&oled_buffer[i], chunk_size);
  }
}

void oled_display_layer_info(uint8_t layer) {
  oled_clear();

  if (config_mode == 1) {
    printf("[OLED] Config mode active, displaying special screen\n");
    oled_clear();
    oled_draw_string(0, 16, "Setup...");
    oled_draw_string(0, 32, "See live web preview");
    oled_draw_string(0, 48, "Remember to apply!");
    oled_update();
    printf("[OLED] Special screen drawn\n");
    return;
  }

  config_data_t *config = config_get();

  // tytul na gorze (y=0)
  oled_draw_emoji(0, 0, config->layer_emojis[layer]);
  char line1[32];
  if (strlen(config->layer_names[layer]) > 0) {
    // nazwa warstwy
    snprintf(line1, sizeof(line1), "%s", config->layer_names[layer]);
  } else {
    // fallback: Layer X/4
    snprintf(line1, sizeof(line1), "Layer %d/4", layer + 1);
  }
  oled_draw_string(15, 0, line1);

  // separator
  oled_draw_line(15);

  // rzad 1: przyciski 1 2 3 (y=16 dla label, y=24 dla emoji)
  oled_draw_string(0, 24, "[1]");
  oled_draw_emoji(18, 24, config->macros[layer][0].emoji_index); // Obok [1]

  oled_draw_string(42, 24, "[2]");
  oled_draw_emoji(60, 24, config->macros[layer][1].emoji_index); // Obok [2]

  oled_draw_string(84, 24, "[3]");
  oled_draw_emoji(102, 24, config->macros[layer][2].emoji_index); // Obok [3]

  // rzad 2: przyciski 4 5 6 (y=32 dla label, y=40 dla emoji)
  oled_draw_string(0, 40, "[4]");
  oled_draw_emoji(18, 40, config->macros[layer][3].emoji_index);

  oled_draw_string(42, 40, "[5]");
  oled_draw_emoji(60, 40, config->macros[layer][4].emoji_index);

  oled_draw_string(84, 40, "[6]");
  oled_draw_emoji(102, 40, config->macros[layer][5].emoji_index);

  // layer switch na dole (y=56)
  oled_draw_string(0, 56, "[7]");
  oled_draw_emoji(18, 56, config->macros[layer][6].emoji_index);

  oled_update();
}

void oled_display_button_preview(uint8_t layer, uint8_t button) {
  oled_clear();

  config_data_t *config = config_get();
  macro_entry_t *macro = &config->macros[layer][button];

  oled_draw_emoji(0, 8, macro->emoji_index);
  char title[32];
  snprintf(title, sizeof(title), "Button %d", button + 1);
  oled_draw_string(12, 8, title);

  // separator
  oled_draw_line(25);

  const char *type_str = "";
  switch (macro->type) {
  case MACRO_TYPE_KEY_PRESS:
    type_str = "Key";
    break;
  case MACRO_TYPE_TEXT_STRING:
    type_str = "Text";
    break;
  case MACRO_TYPE_LAYER_TOGGLE:
    type_str = "Layer";
    break;
  case MACRO_TYPE_SCRIPT:
    type_str = "Script";
    break;
  case MACRO_TYPE_KEY_SEQUENCE:
    type_str = "Sequence";
    break;
  }
  int len = strlen(type_str);
  int x = (OLED_WIDTH - len * 6) / 2;
  oled_draw_string(x, 48, type_str); // wycentrowany

  oled_update();
}

// ==================== BUTTONS ====================

void buttons_init(void) {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    gpio_init(BUTTON_PINS[i]);
    gpio_set_dir(BUTTON_PINS[i], GPIO_IN);
    gpio_pull_up(BUTTON_PINS[i]); // pull-up (button connects to GND)
  }
  cdc_log("[BUTTONS] Initialized (%d buttons)\n", NUM_BUTTONS);
}

bool button_is_pressed(uint8_t button_index) {
  if (button_index >= NUM_BUTTONS)
    return false;
  return button_states[button_index];
}

void buttons_scan(void) {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    button_prev_states[i] = button_states[i];
    button_states[i] = !gpio_get(BUTTON_PINS[i]); // inverted (pull-up)
  }
}

// ==================== LED ====================

void leds_init(void) {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    gpio_init(LED_PINS[i]);
    gpio_set_dir(LED_PINS[i], GPIO_OUT);
    gpio_put(LED_PINS[i], false);
  }
  cdc_log("[LEDS] Initialized (%d LEDs)\n", NUM_BUTTONS);
}

void led_set(uint8_t led_index, bool state) {
  if (led_index >= NUM_BUTTONS)
    return;
  led_states[led_index] = state;
  gpio_put(LED_PINS[led_index], state);
}

void led_toggle(uint8_t led_index) {
  led_set(led_index, !led_states[led_index]);
}

void leds_update_for_layer(uint8_t layer) {
  // turn off all LEDs, then turn on LED for current layer (if layer < 7)
  for (int i = 0; i < NUM_BUTTONS; i++) {
    led_set(i, false);
  }

  if (layer < NUM_BUTTONS - 1) {
    led_set(layer, true);
  }
}

// ==================== INIT ====================

void hardware_init(void) {
  cdc_log("[HARDWARE] Initializing...\n");
  buttons_init();
  leds_init();
  oled_init();
  cdc_log("[HARDWARE] Initialization complete\n");
}
