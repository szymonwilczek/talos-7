#ifndef HARDWARE_INTERFACE_H
#define HARDWARE_INTERFACE_H

#include "macro_config.h"
#include <stdbool.h>
#include <stdint.h>

extern uint8_t config_mode; // 0 - normal, 1 - config mode

// inicjalizacja calego hardware
void hardware_init(void);

// obsluga przyciskow
void buttons_init(void);
bool button_is_pressed(uint8_t button_index);

// obsluga LED
void leds_init(void);
void led_set(uint8_t led_index, bool state);
void led_toggle(uint8_t led_index);
void leds_update_for_layer(uint8_t layer);

// helpery
bool map_char_to_hid(char c, uint8_t *keycode, uint8_t *modifiers);
uint32_t utf8_to_codepoint(const char **str);
const char *get_key_name(uint8_t keycode);
const char *format_sequence_short(key_step_t *sequence, uint8_t len);

#endif // HARDWARE_INTERFACE_H
