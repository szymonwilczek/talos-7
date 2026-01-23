#include "oled/screensaver/screensaver_manager.h"

#include "oled/oled_display.h"
#include "oled/screensaver/animations/anim_bouncing_logo.h"
#include "oled/screensaver/animations/anim_breakout.h"
#include "oled/screensaver/animations/anim_invaders.h"
#include "oled/screensaver/animations/anim_matrix.h"
#include "oled/screensaver/animations/anim_starfield.h"

static ScreensaverType config_mode = SCREENSAVER_CYCLE;
static ScreensaverType current_running_mode = SCREENSAVER_MATRIX;

void screensaver_init(void) {
  config_mode = SCREENSAVER_CYCLE;
  current_running_mode = SCREENSAVER_MATRIX;
}

void screensaver_set_config_mode(ScreensaverType mode) { config_mode = mode; }

static void reset_animation_state(ScreensaverType mode) {
  switch (mode) {
  case SCREENSAVER_MATRIX:
    anim_matrix_init();
    break;
  case SCREENSAVER_BOUNCING_LOGO:
    anim_bouncing_logo_init();
    break;
  case SCREENSAVER_STARFIELD:
    anim_starfield_init();
    break;
  case SCREENSAVER_BREAKOUT:
    anim_breakout_init();
    break;
  case SCREENSAVER_INVADERS:
    anim_invaders_init();
    break;
  default:
    break;
  }
}

void screensaver_start_new_session(void) {
  if (config_mode == SCREENSAVER_CYCLE) {
    current_running_mode++;
    if (current_running_mode >= SCREENSAVER_COUNT) {
      current_running_mode = 0;
    }
  } else {
    if (config_mode < SCREENSAVER_COUNT) {
      current_running_mode = config_mode;
    } else {
      current_running_mode = SCREENSAVER_MATRIX;
    }
  }
  reset_animation_state(current_running_mode);
}

void screensaver_update(void) {
  switch (current_running_mode) {
  case SCREENSAVER_MATRIX:
    anim_matrix_update();
    break;
  case SCREENSAVER_BOUNCING_LOGO:
    anim_bouncing_logo_update();
    break;
  case SCREENSAVER_STARFIELD:
    anim_starfield_update();
    break;
  case SCREENSAVER_BREAKOUT:
    anim_breakout_update();
    break;
  case SCREENSAVER_INVADERS:
    anim_invaders_update();
    break;
  default:
    break;
  }

  oled_update();
}
