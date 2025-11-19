#ifndef HARDWARE_INTERFACE_H
#define HARDWARE_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>

extern uint8_t config_mode; // 0 - normal, 1 - config mode

// inicjalizacja calego hardware
void hardware_init(void);

// obsluga przyciskow
void buttons_init(void);
bool button_is_pressed(uint8_t button_index);
void buttons_scan(void);

// obsluga LED
void leds_init(void);
void led_set(uint8_t led_index, bool state);
void led_toggle(uint8_t led_index);
void leds_update_for_layer(uint8_t layer);

// obsluga OLED
void oled_init(void);
void oled_clear(void);
void oled_display_layer_info(uint8_t layer);
void oled_display_button_preview(uint8_t layer, uint8_t button);
void oled_update(void);

// helpery
bool map_char_to_hid(char c, uint8_t *keycode, uint8_t *modifiers);
uint32_t utf8_to_codepoint(const char **str);

#endif // HARDWARE_INTERFACE_H
