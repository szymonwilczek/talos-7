#ifndef MOCK_OLED_H
#define MOCK_OLED_H

#include <stdbool.h>
#include <stdint.h>

#define MOCK_OLED_WIDTH 128
#define MOCK_OLED_HEIGHT 64
#define MOCK_OLED_FB_SIZE (MOCK_OLED_WIDTH * MOCK_OLED_HEIGHT / 8)

extern uint8_t mock_oled_fb[MOCK_OLED_FB_SIZE];
extern uint8_t mock_oled_last_contrast;
extern bool mock_oled_power_on;

void mock_oled_reset(void);
bool mock_oled_get_pixel(int x, int y);
int mock_oled_count_lit(int x0, int y0, int x1, int y1);

#endif // MOCK_OLED_H
