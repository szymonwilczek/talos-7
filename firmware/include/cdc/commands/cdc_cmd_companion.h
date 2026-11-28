#ifndef CDC_CMD_COMPANION_H
#define CDC_CMD_COMPANION_H

#include "companion/companion_state.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Parses COMP_STATE|<ctx>|<flags>|<top_text>|<sub_text> into state.
 */
bool companion_cdc_apply_state(const char *args, companion_state_t *state,
			       uint32_t now_ms);

/**
 * @brief Parses COMP_EVT|<evt>|<duration_ms>|<label> into state.
 */
bool companion_cdc_apply_event(const char *args, companion_state_t *state,
			       uint32_t now_ms);

/**
 * @brief Parses COMP_TIME|<hour>|<minute> into state.
 */
bool companion_cdc_apply_time(const char *args, companion_state_t *state);

/**
 * @brief Parses COMP_VOL|<pct>|<muted> into state.
 */
bool companion_cdc_apply_volume(const char *args, companion_state_t *state,
				uint32_t now_ms);

/**
 * @brief Parses COMP_MEDIA|<playing>|<title> into state.
 */
bool companion_cdc_apply_media(const char *args, companion_state_t *state);

/**
 * @brief Parses COMP_CI|<active>|<label> into sticky CI alert queue.
 */
bool companion_cdc_apply_ci(const char *args, companion_state_t *state,
			    uint32_t now_ms);

void cmd_handle_comp_state(const char *args);
void cmd_handle_comp_evt(const char *args);
void cmd_handle_comp_time(const char *args);
void cmd_handle_comp_vol(const char *args);
void cmd_handle_comp_media(const char *args);
void cmd_handle_comp_ci(const char *args);
void cmd_handle_comp_wake(void);
void cmd_handle_comp_sleep(void);

#endif // CDC_CMD_COMPANION_H
