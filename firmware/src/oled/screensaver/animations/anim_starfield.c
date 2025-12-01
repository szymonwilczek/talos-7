#include "oled/screensaver/animations/anim_starfield.h"
#include "oled/oled_display.h"
#include <stdlib.h>

#define MAX_STARS 450

typedef struct {
  float x, y, z;
} Star;

static Star stars[MAX_STARS];

static void init_single_star(Star *s) {
  s->x = (float)((rand() % 4000) - 2000);
  s->y = (float)((rand() % 4000) - 2000);
  s->z = (float)((rand() % 800) + 100);
}

void anim_starfield_init(void) {
  for (int i = 0; i < MAX_STARS; i++) {
    init_single_star(&stars[i]);
  }
}

void anim_starfield_update(void) {
  oled_clear();
  const int CX = 64;
  const int CY = 32;

  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].z -= 6.0f; // warp speed

    if (stars[i].z <= 1.0f) {
      init_single_star(&stars[i]);
    }

    int sx = (int)((stars[i].x / stars[i].z) * 60.0f) + CX;
    int sy = (int)((stars[i].y / stars[i].z) * 60.0f) + CY;

    if (sx >= 0 && sx < 128 && sy >= 0 && sy < 64) {
      oled_draw_pixel(sx, sy, 1);
    }
  }
}
