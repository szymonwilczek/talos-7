#ifndef BRIDGE_IDLE_H
#define BRIDGE_IDLE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	BRIDGE_IDLE_NONE = 0,
	BRIDGE_IDLE_SLEEP,
	BRIDGE_IDLE_WAKE
} bridge_idle_transition_t;

typedef struct {
	uint64_t timeout_ms;
	uint64_t last_activity_ms;
	bool sleeping;
	bool initialized;
} bridge_idle_t;

/**
 * Initialize idle tracker with timeout_sec (0 defaults to BRIDGE_IDLE_TIMEOUT_SEC).
 */
void bridge_idle_init(bridge_idle_t *idle, uint32_t timeout_sec,
		      uint64_t now_ms);

/**
 * Record user/host activity. Returns BRIDGE_IDLE_WAKE if exiting sleep.
 */
bridge_idle_transition_t bridge_idle_record_activity(bridge_idle_t *idle,
						     uint64_t now_ms);

/**
 * Explicitly enter idle sleep unless already sleeping.
 */
bridge_idle_transition_t bridge_idle_force_sleep(bridge_idle_t *idle);

/**
 * Advance idle timer. Active media or voice calls inhibit and reset the clock.
 */
bridge_idle_transition_t bridge_idle_step(bridge_idle_t *idle,
					  bool media_or_voice_busy,
					  uint64_t now_ms);

#endif /* BRIDGE_IDLE_H */
