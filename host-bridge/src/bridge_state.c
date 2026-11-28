#include "bridge_state.h"

#include "bridge_serial.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static bool contains_ci(const char *haystack, const char *needle)
{
	if (!haystack || !needle || needle[0] == '\0')
		return false;

	size_t nlen = strlen(needle);
	for (const char *h = haystack; *h != '\0'; h++) {
		size_t i = 0;
		while (i < nlen && h[i] != '\0' &&
		       tolower((unsigned char)h[i]) ==
			       tolower((unsigned char)needle[i])) {
			i++;
		}
		if (i == nlen)
			return true;
	}
	return false;
}

static bool equals_ci(const char *a, const char *b)
{
	if (!a || !b)
		return false;
	while (*a != '\0' && *b != '\0') {
		if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
			return false;
		a++;
		b++;
	}
	return *a == '\0' && *b == '\0';
}

static bool app_is_in(const char *app, const char *const *list, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		if (equals_ci(app, list[i]) || contains_ci(app, list[i]))
			return true;
	}
	return false;
}

static bool app_is_firefox(const char *app)
{
	return contains_ci(app, "firefox");
}

static bool app_is_video_player(const char *app)
{
	static const char *const PLAYERS[] = { "mpv", "vlc", "celluloid" };
	return app_is_in(app, PLAYERS, sizeof(PLAYERS) / sizeof(PLAYERS[0]));
}

static bool app_is_vesktop(const char *app)
{
	static const char *const VOICE_APPS[] = { "vesktop", "discord" };
	return app_is_in(app, VOICE_APPS,
			 sizeof(VOICE_APPS) / sizeof(VOICE_APPS[0]));
}

static bool app_is_emacs(const char *app)
{
	return contains_ci(app, "emacs");
}

static bool app_is_terminal(const char *app)
{
	static const char *const TERMS[] = { "foot", "alacritty", "kitty",
					     "wezterm", "ghostty" };
	return app_is_in(app, TERMS, sizeof(TERMS) / sizeof(TERMS[0]));
}

static bool app_is_game(const char *app)
{
	return contains_ci(app, "steam_app") || contains_ci(app, "gamescope") ||
	       contains_ci(app, "retroarch");
}

static bool focused_firefox_is_video(const host_snapshot_t *snap)
{
	if (!app_is_firefox(snap->focused_app))
		return false;
	return bridge_title_matches_video(snap->focused_title) ||
	       snap->firefox_mpris_playing;
}

bool bridge_is_late_night(int hour)
{
	return hour >= BRIDGE_LATE_NIGHT_START_HOUR &&
	       hour < BRIDGE_LATE_NIGHT_END_HOUR;
}

bool bridge_title_matches_video(const char *title)
{
	static const char *const KEYWORDS[] = {
		"youtube", "netflix",	  "twitch", "prime video", "jellyfin",
		"plex",	   "crunchyroll", "vimeo",  "mpv"
	};
	if (!title)
		return false;
	for (size_t i = 0; i < sizeof(KEYWORDS) / sizeof(KEYWORDS[0]); i++) {
		if (contains_ci(title, KEYWORDS[i]))
			return true;
	}
	return false;
}

bool bridge_is_media_or_voice_busy(const host_snapshot_t *snap)
{
	if (!snap)
		return false;
	return snap->spotify_playing || snap->firefox_mpris_playing ||
	       snap->vesktop_voice_active || snap->load_tier > 0 ||
	       snap->cpu_temp_c >= 80U || snap->ram_used_pct >= 80U ||
	       app_is_video_player(snap->focused_app) ||
	       focused_firefox_is_video(snap);
}

uint8_t bridge_update_load_tier(uint8_t prev_tier, uint8_t cpu_temp_c,
				uint8_t ram_used_pct)
{
	uint8_t peak = (cpu_temp_c > ram_used_pct) ? cpu_temp_c : ram_used_pct;

	if (peak >= 90U)
		return 3U;
	if (prev_tier >= 3U && peak >= 88U)
		return 3U;
	if (peak >= 85U)
		return 2U;
	if (prev_tier >= 2U && peak >= 83U)
		return 2U;
	if (peak >= 80U)
		return 1U;
	if (prev_tier >= 1U && peak >= 76U)
		return 1U;
	return 0U;
}

static void pick_kino_title(const host_snapshot_t *snap, char *dst,
			    size_t dst_size)
{
	const char *src = snap->firefox_mpris_title[0] != '\0' ?
				  snap->firefox_mpris_title :
				  snap->focused_title;
	if (!src || src[0] == '\0')
		src = "Cinema";
	bridge_sanitize_field(src, dst, dst_size, BRIDGE_TEXT_MAX);
}

void bridge_format_spotify_title(const host_snapshot_t *snap, char *dst,
				 size_t dst_size)
{
	if (!dst || dst_size == 0)
		return;
	if (!snap || !snap->spotify_playing) {
		dst[0] = '\0';
		return;
	}

	char combined[140];
	if (snap->spotify_artist[0] != '\0' && snap->spotify_title[0] != '\0') {
		snprintf(combined, sizeof(combined), "%s - %s",
			 snap->spotify_artist, snap->spotify_title);
	} else if (snap->spotify_title[0] != '\0') {
		snprintf(combined, sizeof(combined), "%s", snap->spotify_title);
	} else {
		snprintf(combined, sizeof(combined), "Spotify");
	}
	bridge_sanitize_field(combined, dst, dst_size, BRIDGE_TEXT_MAX);
}

void bridge_resolve_state(const host_snapshot_t *snap, int hour,
			  uint64_t now_ms, bridge_resolved_state_t *out)
{
	if (!snap || !out)
		return;
	memset(out, 0, sizeof(*out));

	uint32_t flags = 0U;
	if (snap->spotify_playing) {
		flags |= BRIDGE_FLAG_MEDIA_ACTIVE;
	}
	if (snap->vesktop_voice_active)
		flags |= BRIDGE_FLAG_VOICE_ACTIVE;
	if (snap->mic_muted)
		flags |= BRIDGE_FLAG_MIC_MUTED;
	if (snap->deafened)
		flags |= BRIDGE_FLAG_DEAFENED;
	if (bridge_is_late_night(hour))
		flags |= BRIDGE_FLAG_LATE_NIGHT;
	if (snap->typing_active && now_ms <= snap->typing_expires_ms)
		flags |= BRIDGE_FLAG_TYPING_PULSE;

	char top[BRIDGE_TEXT_MAX + 1] = { 0 };
	char sub[BRIDGE_SUB_MAX + 1] = { 0 };

	uint8_t load_tier = (snap->load_tier > 0U) ?
				    snap->load_tier :
				    bridge_update_load_tier(0U,
							    snap->cpu_temp_c,
							    snap->ram_used_pct);
	if (load_tier >= 3U) {
		flags |= BRIDGE_FLAG_LOAD_T3;
	} else if (load_tier == 2U) {
		flags |= BRIDGE_FLAG_LOAD_T2;
	}

	if (load_tier > 0U) {
		out->ctx = BRIDGE_CTX_LOAD;
		if (snap->cpu_temp_c >= snap->ram_used_pct) {
			snprintf(top, sizeof(top), "CPU %uC %u%%",
				 (unsigned)snap->cpu_temp_c,
				 (unsigned)snap->ram_used_pct);
		} else {
			snprintf(top, sizeof(top), "RAM %u%% %uC",
				 (unsigned)snap->ram_used_pct,
				 (unsigned)snap->cpu_temp_c);
		}
		snprintf(sub, sizeof(sub), "%s",
			 (load_tier >= 3U) ?
				 "CRITICAL LOAD" :
				 ((load_tier == 2U) ? "OVERHEAT" :
						      "HEAVY LOAD"));
	} else if (snap->agent_thinking) {
		out->ctx = BRIDGE_CTX_AGENT_THINKING;
		const char *task =
			snap->agent_task[0] != '\0' ? snap->agent_task : "AI";
		bridge_sanitize_field(task, top, sizeof(top), BRIDGE_TEXT_MAX);
		snprintf(sub, sizeof(sub), "Thinking...");
	} else if (snap->vesktop_voice_active ||
		   app_is_vesktop(snap->focused_app)) {
		out->ctx = BRIDGE_CTX_VESKTOP;
		snprintf(top, sizeof(top), "Vesktop");
		if (snap->deafened) {
			snprintf(sub, sizeof(sub), "Deafened");
		} else if (snap->mic_muted) {
			snprintf(sub, sizeof(sub), "Mic Muted");
		} else if (snap->vesktop_voice_active) {
			snprintf(sub, sizeof(sub), "Voice On");
		}
	} else if (snap->focused_fullscreen || app_is_game(snap->focused_app)) {
		out->ctx = BRIDGE_CTX_GAMING;
		const char *gm = snap->focused_title[0] != '\0' ?
					 snap->focused_title :
					 snap->focused_app;
		bridge_sanitize_field(gm[0] != '\0' ? gm : "Gaming", top,
				      sizeof(top), BRIDGE_TEXT_MAX);
		snprintf(sub, sizeof(sub), "Fullscreen");
	} else if (app_is_video_player(snap->focused_app) ||
		   snap->firefox_mpris_playing) {
		out->ctx = BRIDGE_CTX_KINO;
		pick_kino_title(snap, top, sizeof(top));
		snprintf(sub, sizeof(sub), "Watching");
	} else if (snap->spotify_playing) {
		out->ctx = BRIDGE_CTX_SPOTIFY;
		bridge_format_spotify_title(snap, top, sizeof(top));
		snprintf(sub, sizeof(sub), "Spotify");
	} else if (app_is_emacs(snap->focused_app) ||
		   (app_is_terminal(snap->focused_app) && snap->emacs_open)) {
		out->ctx = BRIDGE_CTX_EMACS;
		const char *fallback = app_is_emacs(snap->focused_app) ?
					       snap->focused_title :
					       "Emacs";
		const char *buf = snap->emacs_buffer[0] != '\0' ?
					  snap->emacs_buffer :
					  fallback;
		bridge_sanitize_field(buf[0] != '\0' ? buf : "Emacs", top,
				      sizeof(top), BRIDGE_TEXT_MAX);
		bridge_sanitize_field(snap->emacs_mode, sub, sizeof(sub),
				      BRIDGE_SUB_MAX);
	} else if (app_is_terminal(snap->focused_app)) {
		out->ctx = BRIDGE_CTX_TERM;
		const char *ttl = snap->focused_title[0] != '\0' ?
					  snap->focused_title :
					  snap->focused_app;
		bridge_sanitize_field(ttl, top, sizeof(top), BRIDGE_TEXT_MAX);
		bridge_sanitize_field(snap->focused_app, sub, sizeof(sub),
				      BRIDGE_SUB_MAX);
	} else {
		out->ctx = BRIDGE_CTX_IDLE;
		bridge_sanitize_field(snap->focused_app, sub, sizeof(sub),
				      BRIDGE_SUB_MAX);
	}

	out->flags = flags;
	snprintf(out->top_text, sizeof(out->top_text), "%s", top);
	snprintf(out->sub_text, sizeof(out->sub_text), "%s", sub);
}
