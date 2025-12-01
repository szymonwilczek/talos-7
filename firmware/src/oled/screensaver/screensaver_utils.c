#include "oled/screensaver/screensaver_utils.h"
#include "oled/oled_display.h"

void draw_filled_rect(int x, int y, int w, int h, uint8_t color) {
  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h; j++) {
      oled_draw_pixel(x + i, y + j, color);
    }
  }
}

void draw_filled_circle(int cx, int cy, int radius, uint8_t color) {
  for (int y = -radius; y <= radius; y++) {
    for (int x = -radius; x <= radius; x++) {
      if (x * x + y * y <= radius * radius) {
        oled_draw_pixel(cx + x, cy + y, color);
      }
    }
  }
}
