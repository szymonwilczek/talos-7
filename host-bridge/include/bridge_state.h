#ifndef BRIDGE_STATE_H
#define BRIDGE_STATE_H

#include "bridge_protocol.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
	char focused_app[64];
	char focused_title[128];
	bool focused_fullscreen;

	char emacs_buffer[64];
	char emacs_mode[32];

	bool spotify_playing;
	char spotify_artist[64];
	char spotify_title[64];

	bool firefox_mpris_playing;
	char firefox_mpris_title[128];

	bool vesktop_voice_active;
	bool mic_muted;
	bool deafened;

	uint8_t sink_vol_pct;
	bool sink_muted;
	bool sink_vol_valid;

	bool agent_thinking;
	char agent_task[64];

	bool sleeping;
	bool typing_active;
	uint64_t typing_expires_ms;

	uint8_t cpu_temp_c;
	uint8_t ram_used_pct;
	uint8_t load_tier;
	bool load_override;
} host_snapshot_t;

typedef struct {
	bridge_ctx_t ctx;
	uint32_t flags;
	char top_text[BRIDGE_TEXT_MAX + 1];
	char sub_text[BRIDGE_SUB_MAX + 1];
} bridge_resolved_state_t;

/**
 * Return true if the given hour (0..23) falls inside the Late Night window.
 */
bool bridge_is_late_night(int hour);

/**
 * Return true if the snapshot currently holds active media playback or a voice
 * call that must inhibit the 30-minute auto-sleep timer.
 */
bool bridge_is_media_or_voice_busy(const host_snapshot_t *snap);

/**
 * Check whether an arbitrary string matches a known video site/player title.
 */
bool bridge_title_matches_video(const char *title);

/**
 * Format active Spotify track/artist title cleanly for COMP_MEDIA / HUD.
 */
void bridge_format_spotify_title(const host_snapshot_t *snap, char *dst,
				 size_t dst_size);

/**
 * Update compile/overheat load tier (0..3) from CPU temp (C) OR RAM usage (%)
 * with hysteresis so borders do not flicker.
 */
uint8_t bridge_update_load_tier(uint8_t prev_tier, uint8_t cpu_temp_c,
				uint8_t ram_used_pct);

/**
 * Resolve the current host snapshot into a single Talos COMP_STATE descriptor.
 */
void bridge_resolve_state(const host_snapshot_t *snap, int hour,
			  uint64_t now_ms, bridge_resolved_state_t *out);

#endif /* BRIDGE_STATE_H */
