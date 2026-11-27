/*
 * mock definitions - global variables and hardware doubles for host unit tests
 */

#include "mock_pico.h"
#include "mock_companion_hw.h"
#include "mock_flash.h"
#include "mock_gpio.h"
#include "mock_oled.h"
#include "mock_tusb.h"
#include "hardware/spi.h"
#include "macro_config.h"
#include "pin_definitions.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// flash mock storage
uint8_t mock_flash_storage[MOCK_FLASH_SIZE];

// CDC mock buffers
char mock_cdc_output[4096] = { 0 };
int mock_cdc_output_pos = 0;
char mock_cdc_input[4096] = { 0 };
int mock_cdc_input_pos = 0;
int mock_cdc_input_len = 0;

// Companion hardware doubles
uint32_t mock_time_ms = 0;
bool mock_buttons[7] = { false };
bool mock_leds[7] = { false };
bool mock_rgb[3] = { false };
uint16_t mock_last_consumer_usage = 0;
int mock_consumer_count = 0;
uint8_t mock_last_hid_keycode = 0;
int mock_keyboard_count = 0;

// Virtual OLED framebuffer & SSD1306 SPI parser
spi_inst_t mock_spi0_inst = { 0 };
uint8_t mock_oled_fb[MOCK_OLED_FB_SIZE] = { 0 };
uint8_t mock_oled_last_contrast = 0xCF;
bool mock_oled_power_on = true;
static size_t mock_oled_cursor = 0;
static bool mock_expect_contrast_arg = false;
static uint8_t mock_skip_cmd_args = 0;
static bool mock_master_leds = true;
static config_data_t mock_config_storage;

void mock_oled_reset(void)
{
	memset(mock_oled_fb, 0, sizeof(mock_oled_fb));
	mock_oled_last_contrast = 0xCF;
	mock_oled_power_on = true;
	mock_oled_cursor = 0;
	mock_expect_contrast_arg = false;
	mock_skip_cmd_args = 0;
}

bool mock_oled_get_pixel(int x, int y)
{
	if (x < 0 || x >= MOCK_OLED_WIDTH || y < 0 || y >= MOCK_OLED_HEIGHT)
		return false;
	size_t idx = (size_t)(y / 8) * MOCK_OLED_WIDTH + (size_t)x;
	return (mock_oled_fb[idx] & (1U << (y % 8))) != 0;
}

int mock_oled_count_lit(int x0, int y0, int x1, int y1)
{
	int count = 0;
	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++) {
			if (mock_oled_get_pixel(x, y))
				count++;
		}
	}
	return count;
}

void mock_companion_hw_reset(void)
{
	mock_time_ms = 0;
	memset(mock_buttons, 0, sizeof(mock_buttons));
	memset(mock_leds, 0, sizeof(mock_leds));
	memset(mock_rgb, 0, sizeof(mock_rgb));
	mock_last_consumer_usage = 0;
	mock_consumer_count = 0;
	mock_last_hid_keycode = 0;
	mock_keyboard_count = 0;
	mock_master_leds = true;
	mock_oled_reset();
}

int spi_write_blocking(spi_inst_t *spi, const uint8_t *src, size_t len)
{
	(void)spi;
	if (!src)
		return 0;

	if (mock_gpio_state[OLED_DC_PIN]) {
		for (size_t i = 0;
		     i < len && mock_oled_cursor < MOCK_OLED_FB_SIZE; i++) {
			mock_oled_fb[mock_oled_cursor++] = src[i];
		}
	} else {
		for (size_t i = 0; i < len; i++) {
			uint8_t cmd = src[i];
			if (mock_expect_contrast_arg) {
				mock_oled_last_contrast = cmd;
				mock_expect_contrast_arg = false;
			} else if (mock_skip_cmd_args > 0) {
				mock_skip_cmd_args--;
			} else if (cmd == 0x81) {
				mock_expect_contrast_arg = true;
			} else if (cmd == 0x21 || cmd == 0x22) {
				mock_oled_cursor = 0;
				mock_skip_cmd_args = 2;
			} else if (cmd == 0xAE) {
				mock_oled_power_on = false;
			} else if (cmd == 0xAF) {
				mock_oled_power_on = true;
			}
		}
	}
	return (int)len;
}

bool button_is_pressed(uint8_t button_index)
{
	if (button_index >= 7)
		return false;
	return mock_buttons[button_index];
}

void led_set(uint8_t led_index, bool state)
{
	if (led_index < 7)
		mock_leds[led_index] = state;
}

void led_toggle(uint8_t led_index)
{
	if (led_index < 7)
		mock_leds[led_index] = !mock_leds[led_index];
}

void leds_refresh(void)
{
}
bool leds_master_enabled(void)
{
	return mock_master_leds;
}
void leds_master_toggle(void)
{
	mock_master_leds = !mock_master_leds;
}

void led_rgb_set(bool r, bool g, bool b)
{
	mock_rgb[0] = r;
	mock_rgb[1] = g;
	mock_rgb[2] = b;
}

void led_rgb_update_os(uint8_t platform)
{
	(void)platform;
}
uint8_t detect_platform(void)
{
	return 0;
}

void cdc_log(const char *format, ...)
{
	(void)format;
}

void press_sequence(uint8_t modifiers, uint8_t keycode)
{
	(void)modifiers;
	mock_last_hid_keycode = keycode;
	mock_keyboard_count++;
}

void exec_hid_consumer_press(uint16_t usage)
{
	mock_last_consumer_usage = usage;
	mock_consumer_count++;
}

void screensaver_start_new_session(void)
{
}
void screensaver_update(void)
{
}

config_data_t *config_get(void)
{
	return &mock_config_storage;
}
uint8_t config_get_current_layer(void)
{
	return 0;
}
const char *get_key_name(uint8_t keycode)
{
	(void)keycode;
	return "Key";
}
const char *format_sequence_short(key_step_t *sequence, uint8_t len)
{
	(void)sequence;
	(void)len;
	return "";
}
uint32_t utf8_to_codepoint(const char **str)
{
	unsigned char c = (unsigned char)**str;
	if (c)
		(*str)++;
	return c;
}
