#ifndef CDC_CMD_SYSTEM_H
#define CDC_CMD_SYSTEM_H

#include <stdint.h>

/**
 * @brief Handles the SAVE_FLASH command.
 * @note Usage: SAVE_FLASH
 * Saves the current configuration to flash memory.
 */
void cmd_handle_save_flash(void);

/**
 * @brief Handles the RELOAD_CONFIG command.
 * @note Usage: RELOAD_CONFIG
 * Reloads the configuration from flash memory.
 */
void cmd_handle_reload_config(void);

/**
 * @brief Handles the SET_CONFIG_MODE|value command.
 * @note Usage: SET_CONFIG_MODE|1 or SET_CONFIG_MODE|0
 * @param args Pointer to the argument (1 to enable, 0 to disable).
 */
void cmd_handle_set_config_mode(const char *args);

/**
 * @brief Handles the SET_OLED_TIMEOUT|seconds command.
 * @note Usage: SET_OLED_TIMEOUT|seconds
 * @param args Pointer to the argument (timeout in seconds).
 */
void cmd_handle_set_oled_timeout(const char *args);

#endif // CDC_CMD_SYSTEM_H
