#ifndef BRIDGE_MONITORS_H
#define BRIDGE_MONITORS_H

#include "bridge_state.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define SWAY_IPC_MAGIC "i3-ipc"
#define SWAY_IPC_MAGIC_LEN 6U
#define SWAY_IPC_SUBSCRIBE 2U
#define SWAY_IPC_GET_TREE 4U
#define SWAY_IPC_EVENT_WINDOW 0x80000000U

typedef struct {
	int sway_fd;
	uint8_t sway_rx[16384];
	size_t sway_rx_len;

	int dbus_fd;
	pid_t dbus_pid;
	char dbus_rx[2048];
	size_t dbus_rx_len;

	int pactl_fd;
	pid_t pactl_pid;
	char pactl_rx[2048];
	size_t pactl_rx_len;

	bool need_mpris_refresh;
	bool need_audio_refresh;
	bool need_sway_reconnect;
	uint64_t next_reconnect_ms;
	uint64_t last_mpris_poll_ms;
	uint64_t last_audio_poll_ms;
} bridge_monitors_t;

/**
 * Initialize monitor watchers (Sway IPC, D-Bus MPRIS, PipeWire/pactl).
 * Missing host daemons degrade gracefully rather than failing startup.
 */
void bridge_monitors_init(bridge_monitors_t *m, host_snapshot_t *snap);

/**
 * Tear down child watcher processes and close monitor descriptors cleanly.
 */
void bridge_monitors_close(bridge_monitors_t *m);

/**
 * Parse a Sway GET_TREE JSON payload and extract the currently focused window.
 */
bool bridge_parse_sway_tree(const char *json, char *out_app, size_t app_size,
			    char *out_title, size_t title_size,
			    bool *out_fullscreen);

/**
 * Parse a Sway window event JSON payload. Returns true if focus/active state
 * changed and outputs were updated.
 */
bool bridge_parse_sway_event(const char *json, char *out_app, size_t app_size,
			     char *out_title, size_t title_size,
			     bool *out_fullscreen);

/**
 * Parse `gdbus call ... Properties.GetAll` output for an MPRIS player.
 */
bool bridge_parse_gdbus_mpris(const char *text, bool *out_playing,
			      char *out_artist, size_t artist_size,
			      char *out_title, size_t title_size);

/**
 * Parse `pw-dump` JSON output to detect an active Vesktop/Discord voice stream.
 */
bool bridge_parse_pw_dump(const char *json, bool *out_voice_active);

/**
 * Parse `pactl get-source-mute` / `get-sink-mute` text output.
 */
bool bridge_parse_pactl_mute(const char *text, bool *out_muted);

/**
 * Parse one i3-ipc binary frame if complete. Consumes bytes from buf via *len.
 * Returns 1 if a frame was decoded, 0 if incomplete, -1 on corrupt framing.
 */
int bridge_sway_consume_frame(uint8_t *buf, size_t *len, uint32_t *out_type,
			      char *out_payload, size_t payload_cap);

/**
 * Drain readable watcher FDs and run coalesced MPRIS / PipeWire refreshes.
 */
void bridge_monitors_process_events(bridge_monitors_t *m, host_snapshot_t *snap,
				    uint64_t now_ms, bool sway_readable,
				    bool dbus_readable, bool pactl_readable,
				    bool *out_activity);

#endif /* BRIDGE_MONITORS_H */
