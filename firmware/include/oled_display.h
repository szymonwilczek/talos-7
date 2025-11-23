#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

// obsluga OLED
void oled_init(void);
void oled_clear(void);
void oled_display_layer_info(uint8_t layer);
void oled_display_button_preview(uint8_t layer, uint8_t button);
void oled_update(void);

// zarzadzanie energia OLED
void oled_power_save_task(void);          // w petli glownej
void oled_wake_up(void);                  // wybudzenie
bool oled_is_active(void);                // getter stanu aktywnosci
void oled_set_contrast(uint8_t contrast); // do przygaszania
void oled_draw_string(uint8_t x, uint8_t y, const char *str);
void oled_draw_icon_raw(uint8_t x, uint8_t start_page, uint8_t width_px,
                        uint8_t height_pages, const uint8_t *icon_data);

#endif // OLED_DISPLAY_H
