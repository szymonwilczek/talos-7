#ifndef SCREENSAVER_MANAGER_H
#define SCREENSAVER_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  SCREENSAVER_MATRIX = 0,
  SCREENSAVER_BOUNCING_LOGO = 1,
  SCREENSAVER_STARFIELD = 2,
  SCREENSAVER_BREAKOUT = 3,
  SCREENSAVER_INVADERS = 4,

  SCREENSAVER_COUNT,
  SCREENSAVER_CYCLE = 255 // cycling
} ScreensaverType;

typedef struct {
  uint8_t speed;
  uint8_t density;
} MatrixConfig;

typedef struct {
  uint8_t speed_x;
  uint8_t speed_y;
} BouncingLogoConfig;

typedef struct {
  uint8_t speed;
  uint8_t star_count;
} StarfieldConfig;

typedef struct {
  uint16_t generation_delay;
  uint8_t population_density;
} GameOfLifeConfig;

typedef struct {
  uint8_t speed;
} PacmanConfig;

void screensaver_init(void);
void screensaver_set_mode(ScreensaverType mode);
void screensaver_set_config_mode(ScreensaverType mode);
void screensaver_start_new_session(void);
void screensaver_next_mode(void);
void screensaver_reset(void);
void screensaver_update(void);

#endif
