#ifndef SCREENSAVER_UTILS_H
#define SCREENSAVER_UTILS_H

#include <stdbool.h>
#include <stdint.h>

// Helpery graficzne używane przez różne wygaszacze
void draw_filled_rect(int x, int y, int w, int h, uint8_t color);
void draw_filled_circle(int cx, int cy, int radius, uint8_t color);
// Dodaj inne jeśli będą potrzebne (np. draw_ellipse)

#endif
