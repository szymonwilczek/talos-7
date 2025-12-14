/*
 * mock for hardware/gpio.h
 */

#ifndef MOCK_GPIO_H
#define MOCK_GPIO_H

#include <stdbool.h>
#include <stdint.h>

// GPIO state storage for tests
#define MOCK_MAX_GPIOS 32
extern bool mock_gpio_state[MOCK_MAX_GPIOS];
extern bool mock_gpio_direction[MOCK_MAX_GPIOS]; // true = output

// GPIO functions
static inline void gpio_init(uint gpio) { (void)gpio; }
static inline void gpio_set_dir(uint gpio, bool out) {
  if (gpio < MOCK_MAX_GPIOS)
    mock_gpio_direction[gpio] = out;
}
static inline void gpio_put(uint gpio, bool value) {
  if (gpio < MOCK_MAX_GPIOS)
    mock_gpio_state[gpio] = value;
}
static inline bool gpio_get(uint gpio) {
  if (gpio < MOCK_MAX_GPIOS)
    return mock_gpio_state[gpio];
  return false;
}
static inline void gpio_pull_up(uint gpio) { (void)gpio; }
static inline void gpio_pull_down(uint gpio) { (void)gpio; }
static inline void gpio_set_function(uint gpio, uint func) {
  (void)gpio;
  (void)func;
}

// PWM mock
static inline void gpio_set_pwm_enabled(uint gpio, bool enabled) {
  (void)gpio;
  (void)enabled;
}

// direction constants
#define GPIO_OUT true
#define GPIO_IN false

// function constants
#define GPIO_FUNC_SPI 1
#define GPIO_FUNC_PWM 4

#endif // MOCK_GPIO_H
