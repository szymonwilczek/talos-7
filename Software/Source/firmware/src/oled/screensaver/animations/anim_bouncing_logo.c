#include "oled/screensaver/animations/anim_bouncing_logo.h"
#include "assets_graphics.h"
#include "oled/oled_display.h"

static struct {
  float x, y;
  float dx, dy;
} logo_state;

void anim_bouncing_logo_init(void) {
  logo_state.x = 60;
  logo_state.y = 28;
  logo_state.dx = 1;
  logo_state.dy = 1;
}

void anim_bouncing_logo_update(void) {
  oled_clear();
  logo_state.x += logo_state.dx;
  logo_state.y += logo_state.dy;

  if (logo_state.x <= 0) {
    logo_state.x = 0;
    logo_state.dx = -logo_state.dx;
  } else if (logo_state.x >= 128 - 8) {
    logo_state.x = 128 - 8;
    logo_state.dx = -logo_state.dx;
  }

  if (logo_state.y <= 0) {
    logo_state.y = 0;
    logo_state.dy = -logo_state.dy;
  } else if (logo_state.y >= 64 - 8) {
    logo_state.y = 64 - 8;
    logo_state.dy = -logo_state.dy;
  }

  oled_draw_bitmap((int)logo_state.x, (int)logo_state.y, 8, 8, talos_logo_8x8);
}
