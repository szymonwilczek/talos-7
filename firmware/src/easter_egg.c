#include "easter_egg.h"
#include "hardware/watchdog.h"
#include "hardware_interface.h"
#include "oled/oled_display.h"
#include "oled/screensaver/screensaver_utils.h"
#include "pico/stdlib.h"
#include "pin_definitions.h"
#include "tusb.h"
#include <math.h>
#include <stdio.h>

// game constants
#define BRICK_ROWS 3
#define BRICK_COLS 8
#define BRICK_W 14
#define BRICK_H 4
#define PADDLE_W 24
#define PADDLE_SPEED 4.0

typedef struct {
  float ball_x, ball_y;
  float ball_dx, ball_dy;
  float paddle_x;
  bool bricks[BRICK_ROWS][BRICK_COLS];
  int bricks_remaining;
} BreakoutGame;

static BreakoutGame game;

static void game_init(void) {
  game.paddle_x = 52;
  game.ball_x = 64;
  game.ball_y = 60;
  game.ball_dx = 1.5;
  game.ball_dy = -1.5;
  game.bricks_remaining = 0;

  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLS; c++) {
      game.bricks[r][c] = true;
      game.bricks_remaining++;
    }
  }
}

static void game_update(bool left, bool right) {
  oled_clear();

  // paddle movement
  if (left)
    game.paddle_x -= PADDLE_SPEED;
  if (right)
    game.paddle_x += PADDLE_SPEED;

  if (game.paddle_x < 0)
    game.paddle_x = 0;
  if (game.paddle_x > 128 - PADDLE_W)
    game.paddle_x = 128 - PADDLE_W;

  // ball physics
  game.ball_x += game.ball_dx;
  game.ball_y += game.ball_dy;

  // wall collisions
  if (game.ball_x <= 0 || game.ball_x >= 127)
    game.ball_dx *= -1;
  if (game.ball_y <= 0)
    game.ball_dy *= -1;

  // paddle collision
  if (game.ball_y >= 58 && game.ball_y <= 62) {
    if (game.ball_x >= game.paddle_x &&
        game.ball_x <= game.paddle_x + PADDLE_W) {
      game.ball_dy = -fabs(game.ball_dy);

      // ensure it doesnt get stuck inside paddle
      game.ball_y = 57;
    }
  }

  // bottom wall (reset)
  if (game.ball_y > 64) {
    game_init();
  }

  // brick collision
  int col = (int)(game.ball_x) / (BRICK_W + 2);
  int row = (int)(game.ball_y) / (BRICK_H + 2);

  if (row >= 0 && row < BRICK_ROWS && col >= 0 && col < BRICK_COLS) {
    if (game.bricks[row][col]) {
      game.bricks[row][col] = false;
      game.ball_dy *= -1;
      game.bricks_remaining--;
      if (game.bricks_remaining == 0)
        game_init(); // win & reset
    }
  }

  // exit hint
  oled_draw_string(0, 0, "1:EXIT");

  // bricks
  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLS; c++) {
      if (game.bricks[r][c]) {
        draw_filled_rect(c * (BRICK_W + 2) + 1, r * (BRICK_H + 2) + 8, BRICK_W,
                         BRICK_H, 1);
      }
    }
  }

  // paddle & ball
  draw_filled_rect((int)game.paddle_x, 60, PADDLE_W, 2, 1);
  draw_filled_rect((int)game.ball_x, (int)game.ball_y, 2, 2, 1);
}

void run_game_breakout(void) {
  oled_wake_up();
  oled_clear();
  oled_update();

  game_init();

  // blocking loop
  while (true) {
    // check exit
    if (button_is_pressed(0)) {
      // debounce exit
      sleep_ms(200);
      while (button_is_pressed(0))
        tight_loop_contents();
      return;
    }

    // input
    // 4 -> index 3 (left)
    // 6 -> index 5 (right)
    bool left = button_is_pressed(3);
    bool right = button_is_pressed(5);

    game_update(left, right);
    oled_update();

    watchdog_update(); // feed watchdog
    oled_wake_up();    // keep oled awake
    tud_task();        // keep usb alive

    sleep_ms(16); // ~60fps
  }
}
