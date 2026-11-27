#ifndef BRIDGE_SERIAL_H
#define BRIDGE_SERIAL_H

#include "bridge_protocol.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
	int (*find_device)(const char *vid, const char *pid, char *out_path,
			   size_t out_size, void *ctx);
	int (*open_port)(const char *path, void *ctx);
	int (*write_port)(int fd, const char *data, size_t len, void *ctx);
	void (*close_port)(int fd, void *ctx);
	void *ctx;
} bridge_serial_ops_t;

typedef struct {
	char device_override[128];
	char active_path[128];
	int fd;
	bool paused;

	bool has_last_state;
	bridge_ctx_t last_ctx;
	uint32_t last_base_flags;
	char last_top[BRIDGE_TEXT_MAX + 1];
	char last_sub[BRIDGE_SUB_MAX + 1];

	bool has_typing_sent;
	uint64_t last_typing_ms;

	bool has_last_time;
	uint8_t last_hour;
	uint8_t last_minute;

	bool has_last_vol;
	uint8_t last_vol_pct;
	bool last_vol_muted;

	bridge_serial_ops_t ops;
} bridge_serial_t;

/**
 * Initialize a write-only Talos serial transport handle.
 * Pass NULL for ops to use real POSIX /sys discovery + O_WRONLY termios I/O.
 */
void bridge_serial_init(bridge_serial_t *serial, const char *device_override,
			const bridge_serial_ops_t *ops);

/**
 * Close any open serial descriptor cleanly (leaving paused state intact).
 */
void bridge_serial_close(bridge_serial_t *serial);

/**
 * Clear deduplication caches so next frames are transmitted even if unchanged.
 */
void bridge_serial_reset_cache(bridge_serial_t *serial);

/**
 * Sanitize an arbitrary host UTF-8/control string into single-line printable
 * ASCII safe for pipe-delimited CDC frames.
 */
void bridge_sanitize_field(const char *src, char *dst, size_t dst_size,
			   size_t max_chars);

/**
 * Pause or resume serial transmission. Pausing immediately closes /dev/ttyACM*
 * so tools like the web configurator can claim the port.
 */
void bridge_serial_set_paused(bridge_serial_t *serial, bool paused);

/**
 * Ensure the write-only port is opened (if unpaused and Talos is attached).
 */
bool bridge_serial_ensure_open(bridge_serial_t *serial);

/**
 * Send a COMP_STATE frame if content changed or a typing pulse cleared cooldown.
 */
bool bridge_serial_send_state(bridge_serial_t *serial, bridge_ctx_t ctx,
			      uint32_t flags, const char *top_text,
			      const char *sub_text, uint64_t now_ms);

/**
 * Send COMP_EVT|<evt>|<duration_ms>|<label>.
 */
bool bridge_serial_send_event(bridge_serial_t *serial, const char *evt_name,
			      uint32_t duration_ms, const char *label);

/**
 * Send COMP_TIME|<hour>|<minute> if hour or minute changed.
 */
bool bridge_serial_send_time(bridge_serial_t *serial, uint8_t hour,
			     uint8_t minute);

/**
 * Send COMP_VOL|<pct>|<muted> if volume percentage or mute state changed.
 */
bool bridge_serial_send_volume(bridge_serial_t *serial, uint8_t pct,
			       bool muted);

/**
 * Send an uncached single-line command such as "COMP_WAKE" or "COMP_SLEEP".
 */
bool bridge_serial_send_command(bridge_serial_t *serial, const char *cmd);

/**
 * Discover a matching /dev/ttyACM* device path from a sysfs tty root directory.
 */
int bridge_find_tty_in_sysfs(const char *sysfs_tty_root, const char *dev_prefix,
			     const char *vid, const char *pid, char *out_path,
			     size_t out_size);

#endif /* BRIDGE_SERIAL_H */
