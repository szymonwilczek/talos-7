#include "oled/screensaver/animations/anim_matrix.h"

#include "oled/oled_display.h"
#include "oled/screensaver/screensaver_utils.h"
#include <stdlib.h>

#define MATRIX_COL_WIDTH 6
#define MATRIX_COLS (128 / MATRIX_COL_WIDTH)

typedef struct {
  int16_t y;
  bool active;
} MatrixColumn;

static MatrixColumn matrix_cols[MATRIX_COLS];

void anim_matrix_init(void) {
  oled_clear();

  for (int i = 0; i < MATRIX_COLS; i++) {
    matrix_cols[i].y = -100;
    matrix_cols[i].active = false;
  }
}

void anim_matrix_update(void) {
  if (rand() % 2 == 0) {
    int col = rand() % MATRIX_COLS;
    if (!matrix_cols[col].active) {
      matrix_cols[col].active = true;
      matrix_cols[col].y = -6;
    }
  }

  for (int i = 0; i < MATRIX_COLS; i++) {
    if (!matrix_cols[i].active)
      continue;

    int x = i * MATRIX_COL_WIDTH;

    // head
    if (matrix_cols[i].y >= 0 && matrix_cols[i].y < 8) {
      char c = (rand() % (122 - 33 + 1)) + 33;
      oled_draw_char(x, matrix_cols[i].y * 8, c);
    }

    // tail
    int tail_y = matrix_cols[i].y - 5;
    if (tail_y >= 0 && tail_y < 8) {
      draw_filled_rect(x, tail_y * 8, MATRIX_COL_WIDTH, 8, 0);
    }

    matrix_cols[i].y++;

    if (matrix_cols[i].y - 5 > 8)
      matrix_cols[i].active = false;
  }
}
