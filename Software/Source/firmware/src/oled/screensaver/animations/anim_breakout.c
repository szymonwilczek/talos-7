#include "oled/screensaver/animations/anim_breakout.h"
#include "oled/oled_display.h"
#include "oled/screensaver/screensaver_utils.h"
#include <math.h>

#define BRICK_ROWS 3
#define BRICK_COLS 8
#define BRICK_W 14
#define BRICK_H 4
#define PADDLE_W 24

typedef struct {
  float ball_x, ball_y;
  float ball_dx, ball_dy;
  float paddle_x;
  bool bricks[BRICK_ROWS][BRICK_COLS];
  int bricks_remaining;
} BreakoutState;

static BreakoutState breakout;

void anim_breakout_init(void) {
  breakout.paddle_x = 52;
  breakout.ball_x = 64;
  breakout.ball_y = 40;
  breakout.ball_dx = 2.0;
  breakout.ball_dy = -2.0;
  breakout.bricks_remaining = 0;

  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLS; c++) {
      breakout.bricks[r][c] = true;
      breakout.bricks_remaining++;
    }
  }
}

void anim_breakout_update(void) {
  oled_clear();

  // AI
  float target = breakout.ball_x - (PADDLE_W / 2);
  if (breakout.paddle_x < target)
    breakout.paddle_x += 3.5;
  if (breakout.paddle_x > target)
    breakout.paddle_x -= 3.5;
  if (breakout.paddle_x < 0)
    breakout.paddle_x = 0;
  if (breakout.paddle_x > 128 - PADDLE_W)
    breakout.paddle_x = 128 - PADDLE_W;

  // ball physics
  breakout.ball_x += breakout.ball_dx;
  breakout.ball_y += breakout.ball_dy;

  if (breakout.ball_x <= 0 || breakout.ball_x >= 127)
    breakout.ball_dx *= -1;
  if (breakout.ball_y <= 0)
    breakout.ball_dy *= -1;

  // paddle hit
  if (breakout.ball_y >= 58 && breakout.ball_y <= 62) {
    if (breakout.ball_x >= breakout.paddle_x &&
        breakout.ball_x <= breakout.paddle_x + PADDLE_W) {
      breakout.ball_dy = -fabs(breakout.ball_dy);
    }
  }

  // reset condition
  if (breakout.ball_y > 64)
    anim_breakout_init();

  // bricks hit
  int col = (int)(breakout.ball_x) / (BRICK_W + 2);
  int row = (int)(breakout.ball_y) / (BRICK_H + 2);
  if (row >= 0 && row < BRICK_ROWS && col >= 0 && col < BRICK_COLS) {
    if (breakout.bricks[row][col]) {
      breakout.bricks[row][col] = false;
      breakout.ball_dy *= -1;
      breakout.bricks_remaining--;
      if (breakout.bricks_remaining == 0)
        anim_breakout_init();
    }
  }

  // draw
  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLS; c++) {
      if (breakout.bricks[r][c]) {
        draw_filled_rect(c * (BRICK_W + 2) + 1, r * (BRICK_H + 2) + 1, BRICK_W,
                         BRICK_H, 1);
      }
    }
  }
  draw_filled_rect((int)breakout.paddle_x, 60, PADDLE_W, 2, 1);
  draw_filled_rect((int)breakout.ball_x, (int)breakout.ball_y, 2, 2, 1);
}
