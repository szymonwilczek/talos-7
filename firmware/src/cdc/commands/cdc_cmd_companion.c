#include "cdc/commands/cdc_cmd_companion.h"

#include "companion/companion_mode.h"
#include "pico/stdlib.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
	const char *name;
	int id;
} token_map_t;

static const token_map_t CTX_MAP[] = {
	{ "IDLE", COMP_CTX_IDLE },
	{ "EMACS", COMP_CTX_EMACS },
	{ "TERM", COMP_CTX_TERM },
	{ "AGENT_THINKING", COMP_CTX_AGENT_THINKING },
	{ "VESKTOP", COMP_CTX_VESKTOP },
	{ "SPOTIFY", COMP_CTX_SPOTIFY },
	{ "KINO", COMP_CTX_KINO },
	{ "GAMING", COMP_CTX_GAMING },
	{ "LOAD", COMP_CTX_LOAD },
};

static const token_map_t EVT_MAP[] = {
	{ "BOOT_HELLO", COMP_EVT_BOOT_HELLO },
	{ "EXIT_BYE", COMP_EVT_EXIT_BYE },
	{ "PET", COMP_EVT_PET },
	{ "COFFEE", COMP_EVT_COFFEE },
	{ "AGENT_DONE", COMP_EVT_AGENT_DONE },
	{ "AGENT_ERR", COMP_EVT_AGENT_ERR },
	{ "NEEDS_INPUT", COMP_EVT_NEEDS_INPUT },
	{ "GIT_COMMIT", COMP_EVT_GIT_COMMIT },
	{ "PING", COMP_EVT_PING },
};

static bool parse_token_or_int(const char *tok, const token_map_t *map,
			       size_t map_len, int min_val, int max_val,
			       int *out)
{
	if (!tok || *tok == '\0')
		return false;

	for (size_t i = 0; i < map_len; i++) {
		if (strcmp(tok, map[i].name) == 0) {
			*out = map[i].id;
			return true;
		}
	}

	char *endptr = NULL;
	long val = strtol(tok, &endptr, 10);
	if (endptr == tok || *endptr != '\0' || val < min_val ||
	    val > max_val) {
		return false;
	}
	*out = (int)val;
	return true;
}

static void split_field(const char **cursor, char *buf, size_t buf_size,
			bool *has_more)
{
	const char *p = *cursor;
	const char *pipe = strchr(p, '|');
	size_t len = pipe ? (size_t)(pipe - p) : strlen(p);
	if (len >= buf_size)
		len = buf_size - 1;
	memcpy(buf, p, len);
	buf[len] = '\0';

	if (pipe) {
		*cursor = pipe + 1;
		*has_more = true;
	} else {
		*cursor = p + strlen(p);
		*has_more = false;
	}
}

bool companion_cdc_apply_state(const char *args, companion_state_t *state,
			       uint32_t now_ms)
{
	if (!args || !state)
		return false;

	const char *cur = args;
	bool more = false;
	char ctx_buf[24];
	char flags_buf[16];
	char top_buf[COMP_TEXT_MAX_LEN] = { 0 };
	char sub_buf[COMP_SUB_MAX_LEN] = { 0 };

	split_field(&cur, ctx_buf, sizeof(ctx_buf), &more);
	if (!more)
		return false;

	int ctx_id = 0;
	if (!parse_token_or_int(ctx_buf, CTX_MAP,
				sizeof(CTX_MAP) / sizeof(CTX_MAP[0]),
				COMP_CTX_IDLE, COMP_CTX_LOAD, &ctx_id)) {
		return false;
	}

	split_field(&cur, flags_buf, sizeof(flags_buf), &more);
	char *endptr = NULL;
	unsigned long flags = strtoul(flags_buf, &endptr, 0);
	if (flags_buf[0] == '\0' || endptr == flags_buf || *endptr != '\0')
		return false;

	if (more) {
		split_field(&cur, top_buf, sizeof(top_buf), &more);
	}
	if (more) {
		split_field(&cur, sub_buf, sizeof(sub_buf), &more);
	}

	companion_state_set_host_context(state, (companion_host_ctx_t)ctx_id,
					 (uint32_t)flags, top_buf, sub_buf,
					 now_ms);
	return true;
}

bool companion_cdc_apply_event(const char *args, companion_state_t *state,
			       uint32_t now_ms)
{
	if (!args || !state)
		return false;

	const char *cur = args;
	bool more = false;
	char evt_buf[24];
	char dur_buf[16] = { 0 };
	char label_buf[COMP_SUB_MAX_LEN] = { 0 };

	split_field(&cur, evt_buf, sizeof(evt_buf), &more);
	int evt_id = 0;
	if (!parse_token_or_int(evt_buf, EVT_MAP,
				sizeof(EVT_MAP) / sizeof(EVT_MAP[0]),
				COMP_EVT_BOOT_HELLO, COMP_EVT_PING, &evt_id)) {
		return false;
	}

	uint32_t dur_ms = 0;
	if (more) {
		split_field(&cur, dur_buf, sizeof(dur_buf), &more);
		if (dur_buf[0] != '\0') {
			char *endptr = NULL;
			unsigned long parsed = strtoul(dur_buf, &endptr, 10);
			if (endptr == dur_buf || *endptr != '\0')
				return false;
			dur_ms = (uint32_t)parsed;
		}
	}
	if (more) {
		split_field(&cur, label_buf, sizeof(label_buf), &more);
	}

	companion_state_trigger_event(state, (companion_event_id_t)evt_id,
				      dur_ms, label_buf, now_ms);
	return true;
}

bool companion_cdc_apply_time(const char *args, companion_state_t *state)
{
	if (!args || !state)
		return false;

	const char *cur = args;
	bool more = false;
	char hour_buf[8];
	char min_buf[8];

	split_field(&cur, hour_buf, sizeof(hour_buf), &more);
	if (!more || hour_buf[0] == '\0')
		return false;

	split_field(&cur, min_buf, sizeof(min_buf), &more);
	if (min_buf[0] == '\0')
		return false;

	char *end_h = NULL;
	char *end_m = NULL;
	long h = strtol(hour_buf, &end_h, 10);
	long m = strtol(min_buf, &end_m, 10);
	if (*end_h != '\0' || *end_m != '\0' || h < 0 || h > 23 || m < 0 ||
	    m > 59) {
		return false;
	}

	companion_state_set_time(state, (uint8_t)h, (uint8_t)m);
	return true;
}

bool companion_cdc_apply_volume(const char *args, companion_state_t *state,
				uint32_t now_ms)
{
	if (!args || !state)
		return false;

	const char *cur = args;
	bool more = false;
	char pct_buf[8];
	char mute_buf[8];

	split_field(&cur, pct_buf, sizeof(pct_buf), &more);
	if (!more || pct_buf[0] == '\0')
		return false;

	split_field(&cur, mute_buf, sizeof(mute_buf), &more);
	if (mute_buf[0] == '\0')
		return false;

	char *end_p = NULL;
	char *end_m = NULL;
	long pct = strtol(pct_buf, &end_p, 10);
	long muted = strtol(mute_buf, &end_m, 10);
	if (*end_p != '\0' || *end_m != '\0' || pct < 0 || pct > 100 ||
	    (muted != 0 && muted != 1)) {
		return false;
	}

	companion_state_set_volume(state, (uint8_t)pct, muted != 0, now_ms,
				   true);
	return true;
}

bool companion_cdc_apply_media(const char *args, companion_state_t *state)
{
	if (!args || !state)
		return false;

	const char *cur = args;
	bool more = false;
	char play_buf[8];
	char title_buf[COMP_TEXT_MAX_LEN] = { 0 };

	split_field(&cur, play_buf, sizeof(play_buf), &more);
	if (play_buf[0] == '\0')
		return false;

	char *end_p = NULL;
	long playing = strtol(play_buf, &end_p, 10);
	if (*end_p != '\0' || (playing != 0 && playing != 1))
		return false;

	if (more) {
		split_field(&cur, title_buf, sizeof(title_buf), &more);
	}

	companion_state_set_media(state, playing != 0, title_buf);
	return true;
}

bool companion_cdc_apply_ci(const char *args, companion_state_t *state,
			    uint32_t now_ms)
{
	if (!args || !state)
		return false;

	const char *cur = args;
	bool more = false;
	char active_buf[8];
	char label_buf[COMP_TEXT_MAX_LEN] = { 0 };

	split_field(&cur, active_buf, sizeof(active_buf), &more);
	if (active_buf[0] == '\0')
		return false;

	char *end_a = NULL;
	long active = strtol(active_buf, &end_a, 10);
	if (*end_a != '\0' || (active != 0 && active != 1))
		return false;

	if (more) {
		split_field(&cur, label_buf, sizeof(label_buf), &more);
	}

	if (active != 0) {
		companion_state_push_ci(state, label_buf, now_ms);
	} else {
		companion_state_clear_ci(state);
	}
	return true;
}

void cmd_handle_comp_state(const char *args)
{
	uint32_t now_ms = to_ms_since_boot(get_absolute_time());
	companion_cdc_apply_state(args, companion_mode_get_state(), now_ms);
}

void cmd_handle_comp_evt(const char *args)
{
	uint32_t now_ms = to_ms_since_boot(get_absolute_time());
	companion_cdc_apply_event(args, companion_mode_get_state(), now_ms);
}

void cmd_handle_comp_time(const char *args)
{
	companion_cdc_apply_time(args, companion_mode_get_state());
}

void cmd_handle_comp_vol(const char *args)
{
	uint32_t now_ms = to_ms_since_boot(get_absolute_time());
	companion_cdc_apply_volume(args, companion_mode_get_state(), now_ms);
}

void cmd_handle_comp_media(const char *args)
{
	companion_cdc_apply_media(args, companion_mode_get_state());
}

void cmd_handle_comp_ci(const char *args)
{
	uint32_t now_ms = to_ms_since_boot(get_absolute_time());
	companion_cdc_apply_ci(args, companion_mode_get_state(), now_ms);
}

void cmd_handle_comp_wake(void)
{
	uint32_t now_ms = to_ms_since_boot(get_absolute_time());
	companion_state_wake(companion_mode_get_state(), now_ms);
}

void cmd_handle_comp_sleep(void)
{
	uint32_t now_ms = to_ms_since_boot(get_absolute_time());
	companion_state_sleep_now(companion_mode_get_state(), now_ms);
}
