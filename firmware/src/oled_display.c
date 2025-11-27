#include "oled_display.h"

#include "assets_graphics.h"
#include "cdc/cdc_transport.h"
#include "emoji.h"
#include "font.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "hardware_interface.h"
#include "macro_config.h"
#include "pico/stdlib.h"
#include "pin_definitions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool g_oled_active = true;
static uint32_t g_last_activity_time = 0;
static uint8_t oled_buffer[OLED_WIDTH * OLED_HEIGHT / 8];
uint8_t config_mode = 0;

static uint32_t preview_end_time = 0;
static bool is_preview_active = false;
#define PREVIEW_DURATION_MS 2000

// matrix effect
static RainColumn rain_cols[MATRIX_COLS];
static bool matrix_initialized = false;
static struct repeating_timer matrix_timer;
static volatile bool matrix_tick_flag = false;
static bool timer_running = false;

static bool matrix_timer_callback(struct repeating_timer *t) {
  matrix_tick_flag = true;
  return true;
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
  oled_draw_icon_raw(48, 2, 32, 4, talos_boot_logo_32x32);
  oled_update();

  sleep_ms(2000);

  oled_clear();
  oled_update();

  g_last_activity_time = to_ms_since_boot(get_absolute_time());
  g_oled_active = true;

  cdc_log("[OLED] Initialized (128x64)\n");
}

void oled_clear(void) { memset(oled_buffer, 0, sizeof(oled_buffer)); }

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

void oled_write_cmd(uint8_t cmd) {
  gpio_put(OLED_DC_PIN, 0); // command mode
  gpio_put(OLED_CS_PIN, 0);
  spi_write_blocking(OLED_SPI, &cmd, 1);
  gpio_put(OLED_CS_PIN, 1);
}

void oled_write_data(const uint8_t *data, size_t len) {
  gpio_put(OLED_DC_PIN, 1); // data mode
  gpio_put(OLED_CS_PIN, 0);
  spi_write_blocking(OLED_SPI, data, len);
  gpio_put(OLED_CS_PIN, 1);
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

  uint8_t icon_idx = 0;
  uint8_t detected_platform = detect_platform();

  if (detected_platform == 1) {
    icon_idx = 1; // Windows
  } else if (detected_platform == 2) {
    icon_idx = 2; // macOS
  } else {
    icon_idx = 0; // Linux
  }

  oled_draw_bitmap(120, 0, 8, 8, os_icons[icon_idx]);

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
    snprintf(details, sizeof(details), "Mouse Wheel (x%d)", macro->value);
    break;

  case MACRO_TYPE_MIDI_NOTE:
    snprintf(details, sizeof(details), "Note: %d Vel: %d Ch: %d",
             (uint8_t)macro->value,
             (macro->move_x > 0) ? (uint8_t)macro->move_x : 127,
             (macro->move_y > 0) ? (uint8_t)macro->move_y : 1);
    break;
  case MACRO_TYPE_MIDI_CC:
    snprintf(details, sizeof(details), "CC: %d Val: %d Ch: %d",
             (uint8_t)macro->value,
             (macro->move_x > 0) ? (uint8_t)macro->move_x : 127,
             (macro->move_y > 0) ? (uint8_t)macro->move_y : 1);
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

void oled_draw_char(uint8_t x, uint8_t y, char c) {
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

void oled_draw_string(uint8_t x, uint8_t y, const char *str) {
  while (*str) {
    oled_draw_char(x, y, *str);
    x += 6; // 5 pixels + 1 space
    if (x >= OLED_WIDTH - 6)
      break; // prevent overflow
    str++;
  }
}

void oled_draw_line(uint8_t y) {
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

void oled_draw_icon_raw(uint8_t x, uint8_t start_page, uint8_t width_px,
                        uint8_t height_pages, const uint8_t *icon_data) {
  // zabezpieczenie przed wyjsciem poza ekran
  if (x + width_px > OLED_WIDTH ||
      start_page + height_pages > OLED_HEIGHT / 8) {
    return;
  }

  for (uint8_t i = 0; i < height_pages; i++) {
    // indeks dla biezacej strony
    uint16_t buf_offset = (start_page + i) * OLED_WIDTH + x;

    // offest w danych ikony
    uint16_t data_offset = i * width_px;

    // kopiowanie jednego wiersza danych ikony do bufora
    memcpy(&oled_buffer[buf_offset], icon_data + data_offset, width_px);
  }
}

void oled_power_save_task(void) {
  if (config_mode == 1)
    return;

  config_data_t *config = config_get();
  uint32_t timeout_s = config->oled_timeout_s;

  if (timeout_s == 0) {
    if (!g_oled_active)
      oled_set_power(true);
    return;
  }

  uint32_t now = to_ms_since_boot(get_absolute_time());
  uint32_t time_since_activity = now - g_last_activity_time;
  uint32_t timeout_ms = timeout_s * 1000;

  if (time_since_activity > timeout_ms &&
      time_since_activity < (timeout_ms + SCREENSAVER_DURATION_MS)) {

    if (!g_oled_active)
      oled_set_power(true);

    if (!timer_running) {
      add_repeating_timer_ms(50, matrix_timer_callback, NULL, &matrix_timer);
      timer_running = true;
    }

    if (matrix_tick_flag) {
      oled_effect_matrix_rain();
      matrix_tick_flag = false;
    }
  } else if (time_since_activity >= (timeout_ms + SCREENSAVER_DURATION_MS)) {
    if (timer_running) {
      cancel_repeating_timer(&matrix_timer);
      timer_running = false;
    }

    if (g_oled_active) {
      oled_clear();
      oled_update();
      oled_set_power(false);
    }
  } else {
    if (timer_running) {
      cancel_repeating_timer(&matrix_timer);
      timer_running = false;
    }

    if (!g_oled_active) {
      oled_set_power(true);
      uint8_t current_layer = config_get_current_layer();
      oled_display_layer_info(current_layer);
    }
  }
}

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
  matrix_initialized = false;

  if (timer_running) {
    cancel_repeating_timer(&matrix_timer);
    timer_running = false;
  }

  if (!g_oled_active) {
    oled_set_power(true);
  }
}

bool oled_is_active(void) { return g_oled_active; }

int count_active_drops(void) {
  int count = 0;
  for (int i = 0; i < MATRIX_COLS; i++) {
    if (rain_cols[i].active)
      count++;
  }
  return count;
}

void oled_effect_matrix_reset(void) {
  for (int i = 0; i < MATRIX_COLS; i++) {
    rain_cols[i].y = -100;
    rain_cols[i].active = false;
  }
  oled_clear();
  matrix_initialized = true;
}

void oled_effect_matrix_rain(void) {
  if (!matrix_initialized) {
    oled_effect_matrix_reset();
  }

  if (count_active_drops() < MATRIX_MAX_ACTIVE_DROPS) {
    if (rand() % 3 == 0) {
      int col = rand() % MATRIX_COLS;

      if (!rain_cols[col].active) {
        rain_cols[col].active = true;
        rain_cols[col].y = -1;
      }
    }
  }

  for (int i = 0; i < MATRIX_COLS; i++) {
    if (!rain_cols[i].active)
      continue;

    int x = i * MATRIX_COL_WIDTH;
    int y_char_pos = rain_cols[i].y;

    // head
    if (y_char_pos >= 0 && y_char_pos < 8) {
      char random_char = (rand() % (122 - 33 + 1)) + 33;
      oled_draw_char(x, y_char_pos * 8, random_char);
    }

    // clear tail
    int y_erase_pos = y_char_pos - MATRIX_TRAIL_LEN;
    if (y_erase_pos >= 0 && y_erase_pos < 8) {
      oled_draw_char(x, y_erase_pos * 8, ' ');
    }

    // move down
    rain_cols[i].y++;

    // deactivate if off screen
    if (rain_cols[i].y - MATRIX_TRAIL_LEN > 8) {
      rain_cols[i].active = false;
      rain_cols[i].y = -100;
    }
  }

  oled_update();
}

void oled_trigger_preview(uint8_t layer, uint8_t button) {
  oled_display_button_preview(layer, button);

  preview_end_time =
      to_ms_since_boot(get_absolute_time()) + PREVIEW_DURATION_MS;
  is_preview_active = true;

  oled_wake_up();
}

void oled_ui_task(void) {
  if (config_mode == 1)
    return;

  if (is_preview_active) {
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (now > preview_end_time) {
      is_preview_active = false;

      uint8_t current = config_get_current_layer();
      oled_display_layer_info(current);
    }
  }
}
