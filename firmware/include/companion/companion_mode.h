#ifndef COMPANION_MODE_H
#define COMPANION_MODE_H

#include "companion/companion_state.h"
#include <stdbool.h>
#include <stdint.h>

// Standard USB HID keycodes forwarded to Sway/xdg-desktop-portal for Vesktop
#define COMP_HID_KEY_MUTE_MIC 0x68 // F13
#define COMP_HID_KEY_DEAFEN 0x69 // F14

/**
 * @brief Enters Companion Mode, resetting state and seeding button edge states.
 */
void companion_mode_enter(uint32_t now_ms);

/**
 * @brief Forces immediate exit from Companion Mode and restores normal LEDs.
 */
void companion_mode_exit(void);

/**
 * @brief Returns true while the device is running in Companion Mode.
 */
bool companion_mode_is_active(void);

/**
 * @brief Non-blocking tick called from the main loop while Companion Mode is active.
 */
void companion_mode_task(uint32_t now_ms);

/**
 * @brief Handles a debounced button edge inside Companion Mode.
 */
void companion_mode_handle_button_event(uint8_t button_idx, bool pressed,
					uint32_t now_ms);

/**
 * @brief Computes the 7-bit per-key LED mask for the current Companion state.
 */
uint8_t companion_mode_compute_led_mask(const companion_state_t *state,
					uint32_t now_ms);

/**
 * @brief Returns mutable pointer to the active Companion state for CDC updates/tests.
 */
companion_state_t *companion_mode_get_state(void);

#endif // COMPANION_MODE_H
