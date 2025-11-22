#ifndef EXEC_MOUSE_H
#define EXEC_MOUSE_H

#include <stdint.h>

/**
 * @brief Performs mouse click actions.
 * @param buttons Mouse buttons to click (bitmask).
 * @param count Number of times to click.
 * @param interval Interval in milliseconds between clicks.
 * @param trigger_btn Button that triggered the action (for cancellation).
 */
void exec_mouse_click(uint8_t buttons, uint16_t count, uint16_t interval,
                      uint8_t trigger_btn);

/**
 * @brief Moves the mouse cursor.
 * @param total_x Total distance to move in the X direction.
 * @param total_y Total distance to move in the Y direction.
 * @param count Number of steps to divide the movement into.
 * @param interval Interval in milliseconds between each movement step.
 * @param trigger_btn Button that triggered the action (for cancellation).
 */
void exec_mouse_move(int16_t total_x, int16_t total_y, uint16_t count,
                     uint16_t interval, uint8_t trigger_btn);

/**
 * @brief Scrolls the mouse wheel.
 * @param val Amount to scroll the wheel.
 */
void exec_mouse_wheel(int8_t val);

#endif // EXEC_MOUSE_H
