#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// OLED SSD1306 CONSTANTS
#define OLED_SPI spi0
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// SSD1306 COMMANDS
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

// Matrix Rain Effect
#define MATRIX_COL_WIDTH 6        // pixels per column
#define MATRIX_MAX_ACTIVE_DROPS 8 // max simultaneous drops
#define MATRIX_COLS (OLED_WIDTH / MATRIX_COL_WIDTH)
#define MATRIX_TRAIL_LEN 3            // length of the trail in rows
#define SCREENSAVER_DURATION_MS 10000 // 10 seconds

/**
 * @brief Structure to hold the state of a rain column
 */
typedef struct {
  int16_t y;     // Current vertical position of the drop (in character rows)
  uint8_t speed; // Speed of the drop (rows per frame)
  bool active;   // Is the drop currently active
} RainColumn;

/**
 * @brief OLED initialization
 * @param
 * @param
 */
void oled_init(void);

/**
 * @brief Clear OLED display buffer
 * @param
 * @param
 */
void oled_clear(void);

/**
 * @brief Update OLED display with current buffer content
 * @param
 * @param
 */
void oled_update(void);

/**
 * @brief Write command to OLED
 * @param cmd Command byte to send
 * @param
 */
void oled_write_cmd(uint8_t cmd);

/**
 * @brief Write data to OLED
 * @param data Pointer to data bytes
 * @param len Number of bytes to write
 */
void oled_write_data(const uint8_t *data, size_t len);

/**
 * @brief Triggers a temporary preview of the macro on the screen.
 * Does NOT block execution. Sets a timer to revert to layer info automatically.
 */
void oled_trigger_preview(uint8_t layer, uint8_t button);

/**
 * @brief Handles UI timeouts (like reverting preview to layer info).
 * Should be called in the main loop.
 */
void oled_ui_task(void);

/**
 * @brief Display layer information on OLED
 * @param layer Layer index to display
 */
void oled_display_layer_info(uint8_t layer);

/**
 * @brief Display button macro preview on OLED
 * @param layer Layer index
 * @param button Button index
 */
void oled_display_button_preview(uint8_t layer, uint8_t button);

/**
 * @brief Draw a character at specified position
 * @param x X coordinate
 * @param y Y coordinate
 * @param c Character to draw
 */
void oled_draw_char(uint8_t x, uint8_t y, char c);

/**
 * @brief Draw a string at specified position
 * @param x X coordinate
 * @param y Y coordinate
 * @param str String to draw
 */
void oled_draw_string(uint8_t x, uint8_t y, const char *str);

/**
 * @brief Draw a horizontal line at specified Y coordinate (separator)
 * @param y Y coordinate
 */
void oled_draw_line(uint8_t y);

/**
 * @brief Draw a bitmap at specified position
 * @param x X coordinate
 * @param y Y coordinate
 * @param width Width of the bitmap in pixels
 * @param height Height of the bitmap in pixels
 * @param bitmap Pointer to bitmap data
 */
void oled_draw_bitmap(uint8_t x, uint8_t y, uint8_t width, uint8_t height,
                      const uint8_t *bitmap);

/**
 * @brief Draw an emoji at specified position
 * @param x X coordinate
 * @param y Y coordinate
 * @param emoji_index Index of the emoji to draw (see: emoji_bitmaps from
 * emoji.h)
 */
void oled_draw_emoji(uint8_t x, uint8_t y, uint8_t emoji_index);

/**
 * @brief Draw raw icon data at specified position in the buffer
 * @param x X coordinate
 * @param start_page Starting page (8 pixel rows)
 * @param width_px Width of the icon in pixels
 * @param height_pages Height of the icon in pages (8 pixel rows)
 * @param icon_data Pointer to raw icon data
 */
void oled_draw_icon_raw(uint8_t x, uint8_t start_page, uint8_t width_px,
                        uint8_t height_pages, const uint8_t *icon_data);

/**
 * @brief OLED power saving task to be called in main loop
 */
void oled_power_save_task(void); // w petli glownej

/**
 * @brief Set OLED power state
 * @param on true to turn on, false to turn off
 */
void oled_set_power(bool on);

/**
 * @brief Wake up or put to sleep the OLED display
 */
void oled_wake_up(void);

/**
 * @brief Check if OLED is currently active
 * @return true if active, false if in power save mode
 */
bool oled_is_active(void);

/**
 * @brief Renders one frame of the Matrix Rain effect.
 * Should be called periodically (e.g., every 50-100ms) when idle.
 */
void oled_effect_matrix_rain(void);

/**
 * @brief Resets the Matrix Rain state (clears drops).
 * Call this when entering the screensaver mode.
 */
void oled_effect_matrix_reset(void);

/**
 * @brief Count the number of active drops in the Matrix Rain effect
 */
int count_active_drops(void);

#endif // OLED_DISPLAY_H
