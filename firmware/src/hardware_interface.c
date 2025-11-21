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

static void oled_write_cmd(uint8_t cmd);
static void oled_write_data(const uint8_t *data, size_t len);

static bool g_oled_active = true;
static uint32_t g_last_activity_time = 0;

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

static const uint8_t emoji_bitmaps[21][8] = {
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
    {0xc0, 0xfe, 0xc2, 0x02, 0x62, 0x7e, 0x60, 0x00},
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
    // 18: 🔫 (pistol)
    {0x07, 0x06, 0x06, 0x0e, 0x16, 0x96, 0xfe, 0x7c},
    // 19: ⏳ (hourglass)
    {0x82, 0xee, 0x92, 0x92, 0x92, 0xee, 0x82, 0x00},
    // 20: 🌷 (tulip)
    {0x20, 0x4f, 0x9e, 0xff, 0x9e, 0x4f, 0x20, 0x00},
};

#define MAX_EMOJIS 21
static const char *emoji_strings[MAX_EMOJIS] = {
    "🎮", "💼", "🏠", "🔧", "⚡", "📧", "💻", "🎵", "📝", "☕", "🗡️",
    "❤️",  "🔔", "🧪", "🔒", "☂️",  "🦕", "👻", "🔫", "⏳", "🌷"};

static uint8_t oled_buffer[OLED_WIDTH * OLED_HEIGHT / 8];

// button states
static bool button_states[NUM_BUTTONS] = {false};
static bool button_prev_states[NUM_BUTTONS] = {false};

// LED states
static bool led_states[NUM_BUTTONS] = {false};

const char *get_key_name(uint8_t keycode) {
  static const char *key_names[] = {
      "A",     "B",   "C",         "D",   "E",     "F",   "G",   "H",   "I",
      "J",     "K",   "L",         "M",   "N",     "O",   "P",   "Q",   "R",
      "S",     "T",   "U",         "V",   "W",     "X",   "Y",   "Z",   "1",
      "2",     "3",   "4",         "5",   "6",     "7",   "8",   "9",   "0",
      "Enter", "Esc", "Backspace", "Tab", "Space", "F1",  "F2",  "F3",  "F4",
      "F5",    "F6",  "F7",        "F8",  "F9",    "F10", "F11", "F12", "→",
      "←",     "↓",   "↑"};
  if (keycode >= 4 && keycode <= 29)
    return key_names[keycode - 4]; // A-Z
  if (keycode >= 30 && keycode <= 39)
    return key_names[keycode - 30 + 26]; // 1-0
  if (keycode == 40)
    return "Enter";
  if (keycode == 41)
    return "Esc";
  if (keycode == 42)
    return "Backspace";
  if (keycode == 43)
    return "Tab";
  if (keycode == 44)
    return "Space";
  if (keycode >= 58 && keycode <= 69)
    return key_names[keycode - 58 + 36]; // F1-F12
  if (keycode >= 79 && keycode <= 82)
    return key_names[keycode - 79 + 48]; // arrows
  if (keycode == 224)
    return "Ctrl";
  if (keycode == 225)
    return "Shift";
  if (keycode == 226)
    return "Alt";
  if (keycode == 227)
    return "Meta";
  return "Key";
}

const char *format_sequence_short(key_step_t *sequence, uint8_t len) {
  static char buf[32];
  buf[0] = '\0';
  if (len == 0)
    return buf;

  // pierwszy klawisz + modyfikatory (vim like)
  char mods[8] = "";
  if (sequence[0].modifiers & 1)
    strcat(mods, "C"); // ctrl
  if (sequence[0].modifiers & 2)
    strcat(mods, "S"); // shift
  if (sequence[0].modifiers & 4)
    strcat(mods, "A"); // alt
  if (sequence[0].modifiers & 8)
    strcat(mods, "M"); // meta (gui)
  if (strlen(mods) > 0) {
    sprintf(buf, "<%s-%s>", mods, get_key_name(sequence[0].keycode));
  } else {
    strcpy(buf, get_key_name(sequence[0].keycode));
  }

  // nastepne klawisze
  for (int i = 1; i < len && i < 3; i++) {
    strcat(buf, "+");
    strcat(buf, get_key_name(sequence[i].keycode));
  }
  if (len > 3)
    strcat(buf, "...");
  return buf;
}

// ==================== OLED LOW-LEVEL ====================

// ==================== OLED POWER MANAGMENT ====================

void oled_set_power(bool on) {
  oled_write_cmd(on ? OLED_CMD_DISPLAY_ON : OLED_CMD_DISPLAY_OFF);
  g_oled_active = on;
  if (on) {
    cdc_log("[OLED] Display ON\n");
  } else {
    cdc_log("[OLED] Display OFF (Power Save)\n");
  }
}

void oled_wake_up(void) {
  g_last_activity_time = to_ms_since_boot(get_absolute_time());
  if (!g_oled_active) {
    oled_set_power(true);
    oled_update();
  }
}

bool oled_is_active(void) { return g_oled_active; }

void oled_power_save_task(void) {
  if (config_mode == 1)
    return; // ignorowanie w trakcie konfiguracji przez interfejs
  if (!g_oled_active)
    return; // juz wylaczony

  config_data_t *config = config_get();
  uint32_t timeout_s = config->oled_timeout_s;

  // funkcja jest wylaczona (always on)
  if (timeout_s == 0)
    return;

  uint32_t now = to_ms_since_boot(get_absolute_time());
  if ((now - g_last_activity_time) > (timeout_s * 1000)) {
    oled_set_power(false);
  }
}

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

  g_last_activity_time = to_ms_since_boot(get_absolute_time());
  g_oled_active = true;

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
  if (emoji_index >= 21)
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

  // tytul
  char line1[32];
  if (strlen(config->layer_names[layer]) > 0) {
    snprintf(line1, sizeof(line1), "%s", config->layer_names[layer]);
  } else {
    snprintf(line1, sizeof(line1), "Layer %d/4", layer + 1);
  }

  int title_len = strlen(line1);
  int title_width = 8 + title_len * 6; // emoji 8px + tekst 6px/znak
  int title_x = 0;
  title_x = (OLED_WIDTH - title_width) / 2;
  if (title_x < 0)
    title_x = 0;
  oled_draw_emoji(title_x, 0, config->layer_emojis[layer]);
  oled_draw_string(title_x + 15, 0, line1);

  // separator
  oled_draw_line(15);

  // wycentrowany grid przyciskow
  int gap = 12;                                     // odstep miedzy przyciskami
  int button_width = 26 + gap;                      // 26px + gap
  int grid_width = 3 * button_width - gap;          // 90px (ostatni bez gap)
  int grid_x_start = (OLED_WIDTH - grid_width) / 2; // ~19
  int row1_y = 24;
  int row2_y = 40;
  int row3_y = 56;

  // rzad 1: [1] [2] [3]
  oled_draw_string(grid_x_start, row1_y, "[1]");
  oled_draw_emoji(grid_x_start + 18, row1_y,
                  config->macros[layer][0].emoji_index);
  oled_draw_string(grid_x_start + button_width, row1_y, "[2]");
  oled_draw_emoji(grid_x_start + button_width + 18, row1_y,
                  config->macros[layer][1].emoji_index);
  oled_draw_string(grid_x_start + 2 * button_width, row1_y, "[3]");
  oled_draw_emoji(grid_x_start + 2 * button_width + 18, row1_y,
                  config->macros[layer][2].emoji_index);

  // rzad 2: [4] [5] [6]
  oled_draw_string(grid_x_start, row2_y, "[4]");
  oled_draw_emoji(grid_x_start + 18, row2_y,
                  config->macros[layer][3].emoji_index);
  oled_draw_string(grid_x_start + button_width, row2_y, "[5]");
  oled_draw_emoji(grid_x_start + button_width + 18, row2_y,
                  config->macros[layer][4].emoji_index);
  oled_draw_string(grid_x_start + 2 * button_width, row2_y, "[6]");
  oled_draw_emoji(grid_x_start + 2 * button_width + 18, row2_y,
                  config->macros[layer][5].emoji_index);

  // rzad 3: [7]
  int center_x = (OLED_WIDTH - 26) / 2; // 26px dla [7] bez gap
  oled_draw_string(center_x, row3_y, "[7]");
  oled_draw_emoji(center_x + 18, row3_y, config->macros[layer][6].emoji_index);

  oled_update();
}

void oled_display_button_preview(uint8_t layer, uint8_t button) {
  oled_clear();

  config_data_t *config = config_get();
  macro_entry_t *macro = &config->macros[layer][button];

  // emoji + nazwa przycisku
  char title[32];
  if (strlen(macro->name) > 0) {
    snprintf(title, sizeof(title), "%s", macro->name);
  } else {
    snprintf(title, sizeof(title), "Button %d", button + 1);
  }
  int title_len = strlen(title);
  int title_width = 8 + title_len * 6; // emoji 8px + tekst 6px/znak
  int title_x = (OLED_WIDTH - title_width) / 2;
  if (title_x < 0)
    title_x = 0;
  oled_draw_emoji(title_x, 0, macro->emoji_index);
  oled_draw_string(title_x + 15, 0, title);

  // separator
  oled_draw_line(16);

  // szczegoly akcji przycisku
  char details[64];
  switch (macro->type) {
  case MACRO_TYPE_KEY_PRESS:
    snprintf(details, sizeof(details), "Key: %s", get_key_name(macro->value));
    break;
  case MACRO_TYPE_TEXT_STRING: {
    const char *analyze_ptr = macro->macro_string;
    bool has_unicode = false;
    bool has_visible_ascii = false;

    // analiza znakow w stringu
    while (*analyze_ptr) {
      uint32_t cp = utf8_to_codepoint(&analyze_ptr);
      if (cp == 0)
        continue;

      if (cp > 127) {
        has_unicode = true;
      } else if (cp > 32 && cp < 127) {
        has_visible_ascii = true;
      }
    }

    if (has_unicode && !has_visible_ascii) {
      snprintf(details, sizeof(details), "Text: [Unicode/Emoji]");
    } else {
      // 10 znakow tresci + 3 kropki + null terminator
      char safe_preview[16];
      const char *ptr = macro->macro_string;
      size_t idx = 0;
      const size_t CHAR_LIMIT = 10; // limit widocznych znakow przed kropkami

      while (*ptr && idx < CHAR_LIMIT) {
        uint32_t cp = utf8_to_codepoint(&ptr);
        if (cp == 0)
          continue;

        if (cp < 128) {
          safe_preview[idx++] = (char)cp;
        } else {
          safe_preview[idx++] = '?';
        }
      }
      safe_preview[idx] = '\0';

      // cos zostalo - dodaj kropki
      if (*ptr != '\0') {
        strcat(safe_preview, "...");
      }

      snprintf(details, sizeof(details), "Text: \"%s\"", safe_preview);
    }
    break;
  }
  case MACRO_TYPE_LAYER_TOGGLE:
    snprintf(details, sizeof(details), "Layer Toggle to Layer %d",
             macro->value + 1);
    break;
  case MACRO_TYPE_SCRIPT:
    snprintf(details, sizeof(details), "Script (%s)",
             macro->script_platform == 0   ? "Linux"
             : macro->script_platform == 1 ? "Windows"
                                           : "macOS");
    break;
  case MACRO_TYPE_KEY_SEQUENCE:
    // skrocona wersja
    snprintf(details, sizeof(details), "Sequence: %s",
             format_sequence_short(macro->sequence, macro->sequence_length));
    break;
  case MACRO_TYPE_MOUSE_BUTTON:
    snprintf(details, sizeof(details), "Mouse Button: %s",
             macro->value == 1   ? "Left"
             : macro->value == 2 ? "Right"
             : macro->value == 4 ? "Middle"
                                 : "Other");
    break;
  case MACRO_TYPE_MOUSE_MOVE:
    snprintf(details, sizeof(details), "Mouse Move: X=%d Y=%d", macro->move_x,
             macro->move_y);
    break;

  case MACRO_TYPE_MOUSE_WHEEL:
    snprintf(details, sizeof(details), "Mouse Wheel (x%d)",
             macro->value);
    break;
  default:
    snprintf(details, sizeof(details), "Unknown Action");
    break;
  }

  // wycentrowane szczegoly
  int len = strlen(details);
  int x = (OLED_WIDTH - len * 6) / 2;
  oled_draw_string(x, 40, details);

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

bool map_char_to_hid(char c, uint8_t *keycode, uint8_t *modifiers) {
  *modifiers = 0;
  if (c >= 'a' && c <= 'z') {
    *keycode = c - 'a' + 4;
  } else if (c >= 'A' && c <= 'Z') {
    *keycode = c - 'A' + 4;
    *modifiers = 0x02; // SHIFT
  } else if (c >= '1' && c <= '9') {
    *keycode = c - '1' + 30;
  } else if (c == '0') {
    *keycode = 39;
  } else if (c == ' ') {
    *keycode = 44;
  } else if (c == '\n' || c == '\r') {
    *keycode = 40; // Enter
  } else if (c == '!') {
    *keycode = 30; // 1
    *modifiers = 0x02;
  } else if (c == '@') {
    *keycode = 31; // 2
    *modifiers = 0x02;
  } else if (c == '#') {
    *keycode = 32; // 3
    *modifiers = 0x02;
  } else if (c == '$') {
    *keycode = 33; // 4
    *modifiers = 0x02;
  } else if (c == '%') {
    *keycode = 34; // 5
    *modifiers = 0x02;
  } else if (c == '^') {
    *keycode = 35; // 6
    *modifiers = 0x02;
  } else if (c == '&') {
    *keycode = 36; // 7
    *modifiers = 0x02;
  } else if (c == '*') {
    *keycode = 37; // 8
    *modifiers = 0x02;
  } else if (c == '(') {
    *keycode = 38; // 9
    *modifiers = 0x02;
  } else if (c == ')') {
    *keycode = 39; // 0
    *modifiers = 0x02;
  } else if (c == '_') {
    *keycode = 45; // -
    *modifiers = 0x02;
  } else if (c == '+') {
    *keycode = 46; // =
    *modifiers = 0x02;
  } else if (c == '{') {
    *keycode = 47; // [
    *modifiers = 0x02;
  } else if (c == '}') {
    *keycode = 48; // ]
    *modifiers = 0x02;
  } else if (c == '|') {
    *keycode = 49; // \
    *modifiers = 0x02;
  } else if (c == ':') {
    *keycode = 51; // ;
    *modifiers = 0x02;
  } else if (c == '"') {
    *keycode = 52; // '
    *modifiers = 0x02;
  } else if (c == '~') {
    *keycode = 53; // `
    *modifiers = 0x02;
  } else if (c == '<') {
    *keycode = 54; // ,
    *modifiers = 0x02;
  } else if (c == '>') {
    *keycode = 55; // .
    *modifiers = 0x02;
  } else if (c == '?') {
    *keycode = 56; // /
    *modifiers = 0x02;
  } else if (c == '\t') {
    *keycode = 43; // Tab
  } else if (c == '\b') {
    *keycode = 42; // Backspace
  } else if (c == '.') {
    *keycode = 55;
  } else if (c == ',') {
    *keycode = 54;
  } else if (c == '-') {
    *keycode = 45;
  } else if (c == '=') {
    *keycode = 46;
  } else if (c == '[') {
    *keycode = 47;
  } else if (c == ']') {
    *keycode = 48;
  } else if (c == '\\') {
    *keycode = 49;
  } else if (c == ';') {
    *keycode = 51;
  } else if (c == '\'') {
    *keycode = 52;
  } else if (c == '`') {
    *keycode = 53;
  } else if (c == '/') {
    *keycode = 56;
  } else {
    return false; // Nieobsługiwany znak
  }
  return true;
}

uint32_t utf8_to_codepoint(const char **str) {
  unsigned char c = **str;
  (*str)++;
  if (c < 0x80)
    return c;
  if (c < 0xC0)
    return 0; // Invalid
  int len = 0;
  if (c < 0xE0)
    len = 1;
  else if (c < 0xF0)
    len = 2;
  else if (c < 0xF8)
    len = 3;
  else
    return 0;
  uint32_t code = c & (0xFF >> (len + 1));
  for (int i = 0; i < len; i++) {
    c = **str;
    (*str)++;
    if ((c & 0xC0) != 0x80)
      return 0;
    code = (code << 6) | (c & 0x3F);
  }
  return code;
}

// ==================== INIT ====================

void hardware_init(void) {
  cdc_log("[HARDWARE] Initializing...\n");
  buttons_init();
  leds_init();
  oled_init();
  cdc_log("[HARDWARE] Initialization complete\n");
}
