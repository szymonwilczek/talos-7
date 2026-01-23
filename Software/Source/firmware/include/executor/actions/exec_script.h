#ifndef EXEC_SCRIPT_H
#define EXEC_SCRIPT_H

#include "macro_config.h"
#include <stdint.h>

/**
 * @brief Executes a script by opening a terminal and typing commands.
 * This function encapsulates the logic from the MACRO_TYPE_SCRIPT case.
 * @param script Script content to be typed.
 * @param platform Platform (0=Linux, 1=Win, 2=Mac).
 * @param shortcut Array of key steps for the terminal shortcut.
 * @param shortcut_len Length of the shortcut array.
 */
void exec_script(const char *script, uint8_t platform,
                 const key_step_t *shortcut, uint8_t shortcut_len);

#endif // EXEC_SCRIPT_H
