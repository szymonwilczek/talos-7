#ifndef EXEC_HID_CORE_H
#define EXEC_HID_CORE_H

#include "macro_config.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Checks if the cancel button is pressed.
 * If it is pressed, optionally waits for its release.
 * @param btn_index Index of the cancel button to check.
 * @param wait_for_release Pointer to a boolean that indicates whether to wait
 * for release.
 * @return true if the cancel button was pressed, false otherwise.
 */
bool check_cancel(uint8_t btn_index, bool *wait_for_release);

/**
 * @brief Sends a key with specified modifiers for a given duration.
 * @param modifiers Modifier keys to hold down.
 * @param keycode Keycode to send.
 * @param press_ms Duration in milliseconds to hold the key down.
 */
void send_key_with_modifiers(uint8_t modifiers, uint8_t keycode,
                             uint16_t press_ms);

/**
 * @brief Presses a key with specified modifiers.
 * @param modifiers Modifier keys to hold down.
 * @param keycode Keycode to send.
 */
void press_sequence(uint8_t modifiers, uint8_t keycode);

/**
 * @brief Executes a sequence of key steps.
 * @param sequence Pointer to an array of key steps.
 * @param len Length of the key steps array.
 */
void exec_key_sequence(const key_step_t *sequence, uint8_t len);

/**
 * @brief Repeats a key press a specified number of times with a given interval.
 * @param keycode Keycode to repeat.
 * @param count Number of times to repeat the key press.
 * @param interval Interval in milliseconds between each key press.
 */
void exec_key_repeat(uint8_t keycode, uint16_t count, uint16_t interval);

/**
 * @brief Repeats a key press quickly a specified number of times with a given
 * interval.
 * @param keycode Keycode to repeat.
 * @param count Number of times to repeat the key press.
 * @param interval Interval in milliseconds between each key press.
 */
void exec_key_repeat_fast(uint8_t keycode, uint16_t count, uint16_t interval);

#endif // EXEC_HID_CORE_H
