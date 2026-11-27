#include "bridge_idle.h"

#include "bridge_protocol.h"
#include <string.h>

void bridge_idle_init(bridge_idle_t *idle, uint32_t timeout_sec,
		      uint64_t now_ms)
{
	if (!idle)
		return;
	memset(idle, 0, sizeof(*idle));
	uint32_t secs = timeout_sec > 0 ? timeout_sec : BRIDGE_IDLE_TIMEOUT_SEC;
	idle->timeout_ms = (uint64_t)secs * 1000ULL;
	idle->last_activity_ms = now_ms;
	idle->initialized = true;
}

bridge_idle_transition_t bridge_idle_record_activity(bridge_idle_t *idle,
						     uint64_t now_ms)
{
	if (!idle)
		return BRIDGE_IDLE_NONE;
	if (!idle->initialized)
		bridge_idle_init(idle, 0, now_ms);

	idle->last_activity_ms = now_ms;
	if (idle->sleeping) {
		idle->sleeping = false;
		return BRIDGE_IDLE_WAKE;
	}
	return BRIDGE_IDLE_NONE;
}

bridge_idle_transition_t bridge_idle_force_sleep(bridge_idle_t *idle)
{
	if (!idle || idle->sleeping)
		return BRIDGE_IDLE_NONE;
	idle->sleeping = true;
	return BRIDGE_IDLE_SLEEP;
}

bridge_idle_transition_t
bridge_idle_step(bridge_idle_t *idle, bool media_or_voice_busy, uint64_t now_ms)
{
	if (!idle)
		return BRIDGE_IDLE_NONE;
	if (!idle->initialized)
		bridge_idle_init(idle, 0, now_ms);

	if (media_or_voice_busy) {
		return bridge_idle_record_activity(idle, now_ms);
	}

	if (!idle->sleeping && now_ms >= idle->last_activity_ms &&
	    (now_ms - idle->last_activity_ms) >= idle->timeout_ms) {
		idle->sleeping = true;
		return BRIDGE_IDLE_SLEEP;
	}
	return BRIDGE_IDLE_NONE;
}
