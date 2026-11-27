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
	       snap->vesktop_voice_active ||
	       app_is_video_player(snap->focused_app) ||
	       focused_firefox_is_video(snap);
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

void bridge_resolve_state(const host_snapshot_t *snap, int hour,
			  uint64_t now_ms, bridge_resolved_state_t *out)
{
	if (!snap || !out)
		return;
	memset(out, 0, sizeof(*out));

	uint32_t flags = 0U;
	if (snap->spotify_playing || snap->firefox_mpris_playing ||
	    app_is_video_player(snap->focused_app)) {
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

	if (snap->agent_thinking) {
		out->ctx = BRIDGE_CTX_AGENT_THINKING;
		const char *task =
			snap->agent_task[0] != '\0' ? snap->agent_task : "AI";
		bridge_sanitize_field(task, top, sizeof(top), BRIDGE_TEXT_MAX);
		snprintf(sub, sizeof(sub), "Thinking...");
	} else if (app_is_video_player(snap->focused_app) ||
		   focused_firefox_is_video(snap)) {
		out->ctx = BRIDGE_CTX_KINO;
		flags |= BRIDGE_FLAG_MEDIA_ACTIVE;
		if (app_is_firefox(snap->focused_app) &&
		    !snap->firefox_mpris_playing) {
			flags |= BRIDGE_FLAG_VIDEO_PAUSED;
			snprintf(sub, sizeof(sub), "Paused");
		} else {
			snprintf(sub, sizeof(sub), "Watching");
		}
		pick_kino_title(snap, top, sizeof(top));
	} else if (app_is_vesktop(snap->focused_app)) {
		out->ctx = BRIDGE_CTX_VESKTOP;
		snprintf(top, sizeof(top), "Vesktop");
		if (snap->deafened) {
			snprintf(sub, sizeof(sub), "Deafened");
		} else if (snap->mic_muted) {
			snprintf(sub, sizeof(sub), "Mic Muted");
		} else if (snap->vesktop_voice_active) {
			snprintf(sub, sizeof(sub), "Voice On");
		}
	} else if (app_is_emacs(snap->focused_app)) {
		out->ctx = BRIDGE_CTX_EMACS;
		const char *buf = snap->emacs_buffer[0] != '\0' ?
					  snap->emacs_buffer :
					  snap->focused_title;
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
	} else if (snap->focused_fullscreen || app_is_game(snap->focused_app)) {
		out->ctx = BRIDGE_CTX_GAMING;
		const char *gm = snap->focused_title[0] != '\0' ?
					 snap->focused_title :
					 snap->focused_app;
		bridge_sanitize_field(gm[0] != '\0' ? gm : "Gaming", top,
				      sizeof(top), BRIDGE_TEXT_MAX);
		snprintf(sub, sizeof(sub), "Fullscreen");
	} else if (snap->firefox_mpris_playing) {
		out->ctx = BRIDGE_CTX_KINO;
		pick_kino_title(snap, top, sizeof(top));
		snprintf(sub, sizeof(sub), "Playing");
	} else if (snap->spotify_playing) {
		out->ctx = BRIDGE_CTX_SPOTIFY;
		char combined[140];
		if (snap->spotify_artist[0] != '\0' &&
		    snap->spotify_title[0] != '\0') {
			snprintf(combined, sizeof(combined), "%s - %s",
				 snap->spotify_artist, snap->spotify_title);
		} else if (snap->spotify_title[0] != '\0') {
			snprintf(combined, sizeof(combined), "%s",
				 snap->spotify_title);
		} else {
			snprintf(combined, sizeof(combined), "Spotify");
		}
		bridge_sanitize_field(combined, top, sizeof(top),
				      BRIDGE_TEXT_MAX);
		snprintf(sub, sizeof(sub), "Spotify");
	} else if (snap->vesktop_voice_active) {
		out->ctx = BRIDGE_CTX_VESKTOP;
		snprintf(top, sizeof(top), "Vesktop");
		snprintf(sub, sizeof(sub),
			 snap->deafened ? "Deafened" :
					  (snap->mic_muted ? "Muted" : "Call"));
	} else {
		out->ctx = BRIDGE_CTX_IDLE;
		bridge_sanitize_field(snap->focused_app, sub, sizeof(sub),
				      BRIDGE_SUB_MAX);
	}

	out->flags = flags;
	snprintf(out->top_text, sizeof(out->top_text), "%s", top);
	snprintf(out->sub_text, sizeof(out->sub_text), "%s", sub);
}
