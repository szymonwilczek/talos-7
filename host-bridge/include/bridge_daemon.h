#ifndef BRIDGE_DAEMON_H
#define BRIDGE_DAEMON_H

#include "bridge_idle.h"
#include "bridge_monitors.h"
#include "bridge_serial.h"
#include "bridge_state.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct {
	bridge_serial_t serial;
	host_snapshot_t snap;
	bridge_idle_t idle;
	bridge_monitors_t monitors;
	uint64_t last_time_sync_ms;
	int8_t hour_override;
	uint8_t minute_override;
} bridge_daemon_t;

/**
 * Initialize daemon state, idle tracker and one-way serial transport.
 */
void bridge_daemon_init(bridge_daemon_t *daemon, const char *device_override,
			uint32_t idle_timeout_sec,
			const bridge_serial_ops_t *serial_ops, uint64_t now_ms);

/**
 * Parse and apply one newline-free event/control command line into daemon state.
 */
bool bridge_daemon_handle_line(bridge_daemon_t *daemon, const char *line,
			       uint64_t now_ms);

/**
 * Advance daemon timers, idle guard, wall-clock sync, and emit diff-only state.
 */
void bridge_daemon_tick(bridge_daemon_t *daemon, uint64_t now_ms,
			time_t wall_time, bool activity_occurred);

/**
 * Run the event-driven poll() loop until terminated by SIGINT/SIGTERM.
 */
int bridge_daemon_run(bridge_daemon_t *daemon);

#endif /* BRIDGE_DAEMON_H */
