#include "oled/screensaver/animations/anim_invaders.h"
#include "assets_graphics.h"
#include "oled/oled_display.h"
#include "oled/screensaver/screensaver_utils.h"
#include <math.h>

#define INV_COLS 6
#define INV_ROWS 2

typedef struct {
  float cannon_x;
  float bullet_x, bullet_y;
  bool bullet_active;
  float group_x, group_y;
  float group_dx;
  bool invaders[INV_ROWS][INV_COLS];
  int inv_count;
  uint32_t frame_counter;
} InvadersState;

static InvadersState invaders;

void anim_invaders_init(void) {
  invaders.cannon_x = 60;
  invaders.bullet_active = false;
  invaders.group_x = 10;
  invaders.group_y = 2;
  invaders.group_dx = 0.5f;
  invaders.inv_count = 0;
  invaders.frame_counter = 0;

  for (int r = 0; r < INV_ROWS; r++) {
    for (int c = 0; c < INV_COLS; c++) {
      invaders.invaders[r][c] = true;
      invaders.inv_count++;
    }
  }
}

void anim_invaders_update(void) {
  oled_clear();
  invaders.frame_counter++;

  // AI
  float target_x = invaders.cannon_x;
  bool found_target = false;
  for (int c = 0; c < INV_COLS; c++) {
    if (invaders.invaders[1][c] || invaders.invaders[0][c]) {
      float alien_real_x = invaders.group_x + (c * 12) + 4;
      if (!found_target || fabs(alien_real_x - invaders.cannon_x) <
                               fabs(target_x - invaders.cannon_x)) {
        target_x = alien_real_x;
        found_target = true;
      }
    }
  }

  if (found_target) {
    if (invaders.cannon_x < target_x)
      invaders.cannon_x += 3.0f;
    if (invaders.cannon_x > target_x)
      invaders.cannon_x -= 3.0f;
  }

  if (!invaders.bullet_active && found_target &&
      fabs(invaders.cannon_x - target_x) < 4.0f) {
    invaders.bullet_active = true;
    invaders.bullet_x = (float)((int)invaders.cannon_x + 3);
    invaders.bullet_y = 54.0f;
  }

  // movement
  invaders.group_x += invaders.group_dx;
  if (invaders.group_x < 2 || invaders.group_x > 50) {
    invaders.group_dx = -invaders.group_dx;
    invaders.group_y += 2;
  }

  if (invaders.group_y > 45 || invaders.inv_count == 0) {
    anim_invaders_init();
    return;
  }

  // bullet
  if (invaders.bullet_active) {
    invaders.bullet_y -= 6.0f;
    oled_draw_pixel((int)invaders.bullet_x, (int)invaders.bullet_y, 1);
    oled_draw_pixel((int)invaders.bullet_x, (int)invaders.bullet_y + 1, 1);

    if (invaders.bullet_y < 0)
      invaders.bullet_active = false;

    // collision
    for (int r = 0; r < INV_ROWS; r++) {
      for (int c = 0; c < INV_COLS; c++) {
        if (invaders.invaders[r][c]) {
          float ax = invaders.group_x + (c * 12);
          float ay = invaders.group_y + (r * 10);

          if (invaders.bullet_x >= ax && invaders.bullet_x <= ax + 8 &&
              invaders.bullet_y >= ay && invaders.bullet_y <= ay + 8) {
            invaders.invaders[r][c] = false;
            invaders.bullet_active = false;
            invaders.inv_count--;
            draw_filled_rect((int)ax, (int)ay, 8, 8, 0); // explosion
          }
        }
      }
    }
  }

  // draw
  oled_draw_bitmap((int)invaders.cannon_x, 56, 8, 8, cannon_bitmap);

  int anim_frame = (invaders.frame_counter / 10) % 2;
  const uint8_t *sprite = (anim_frame == 0) ? invader_frame1 : invader_frame2;

  for (int r = 0; r < INV_ROWS; r++) {
    for (int c = 0; c < INV_COLS; c++) {
      if (invaders.invaders[r][c]) {
        int ax = (int)(invaders.group_x + (c * 12));
        int ay = (int)(invaders.group_y + (r * 10));
        oled_draw_bitmap(ax, ay, 8, 8, sprite);
      }
    }
  }
}
