#ifndef MOCK_COMPANION_HW_H
#define MOCK_COMPANION_HW_H

#include <stdbool.h>
#include <stdint.h>

extern uint32_t mock_time_ms;
extern bool mock_buttons[7];
extern bool mock_leds[7];
extern bool mock_rgb[3];
extern uint16_t mock_last_consumer_usage;
extern int mock_consumer_count;
extern uint8_t mock_last_hid_keycode;
extern int mock_keyboard_count;

void mock_companion_hw_reset(void);

#endif // MOCK_COMPANION_HW_H
