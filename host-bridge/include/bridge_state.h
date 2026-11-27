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

	bool agent_thinking;
	char agent_task[64];

	bool sleeping;
	bool typing_active;
	uint64_t typing_expires_ms;
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
 * Resolve the current host snapshot into a single Talos COMP_STATE descriptor.
 */
void bridge_resolve_state(const host_snapshot_t *snap, int hour,
			  uint64_t now_ms, bridge_resolved_state_t *out);

#endif /* BRIDGE_STATE_H */
