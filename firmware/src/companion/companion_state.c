#include "companion/companion_state.h"

#include <stdio.h>
#include <string.h>

static void safe_copy_str(char *dst, size_t dst_size, const char *src)
{
	if (!dst || dst_size == 0)
		return;
	if (!src) {
		dst[0] = '\0';
		return;
	}
	snprintf(dst, dst_size, "%s", src);
}

static uint8_t clamp_pct(int val)
{
	if (val < 0)
		return 0;
	if (val > 100)
		return 100;
	return (uint8_t)val;
}

static uint32_t default_event_duration_ms(companion_event_id_t evt)
{
	switch (evt) {
	case COMP_EVT_BOOT_HELLO:
		return 1500;
	case COMP_EVT_EXIT_BYE:
		return 900;
	case COMP_EVT_PET:
	case COMP_EVT_COFFEE:
	case COMP_EVT_PING:
		return 3000;
	case COMP_EVT_AGENT_DONE:
	case COMP_EVT_GIT_COMMIT:
		return 4000;
	case COMP_EVT_AGENT_ERR:
	case COMP_EVT_NEEDS_INPUT:
		return 5000;
	default:
		return 0;
	}
}

static void pomo_load_phase(companion_pomodoro_t *pomo,
			    companion_pomo_phase_t phase)
{
	pomo->phase = phase;
	pomo->total_s = (phase == COMP_POMO_WORK) ? pomo->work_duration_s :
						    pomo->break_duration_s;
	pomo->remaining_s = pomo->total_s;
	pomo->running = false;
	pomo->phase_completed_alert = false;
}

void companion_state_init(companion_state_t *state, uint32_t now_ms)
{
	if (!state)
		return;

	memset(state, 0, sizeof(*state));
	state->submode = COMP_SUBMODE_HOME;
	state->wake_submode = COMP_SUBMODE_HOME;
	state->host_ctx = COMP_CTX_IDLE;
	state->sleep_stage = COMP_SLEEP_AWAKE;
	state->last_activity_ms = now_ms;
	state->happiness = 80;
	state->energy = 80;
	state->vol_level = 4;

	state->pomo.work_duration_s = COMP_POMO_DEFAULT_WORK_S;
	state->pomo.break_duration_s = COMP_POMO_DEFAULT_BREAK_S;
	pomo_load_phase(&state->pomo, COMP_POMO_WORK);

	companion_state_trigger_event(state, COMP_EVT_BOOT_HELLO, 1500,
				      "TALOS READY", now_ms);
}

void companion_state_touch_activity(companion_state_t *state, uint32_t now_ms)
{
	if (!state)
		return;
	state->last_activity_ms = now_ms;
	if (state->sleep_stage == COMP_SLEEP_DROWSY) {
		state->sleep_stage = COMP_SLEEP_AWAKE;
	}
}

bool companion_state_wake(companion_state_t *state, uint32_t now_ms)
{
	if (!state)
		return false;

	bool was_sleeping = companion_state_is_sleeping(state) ||
			    state->sleep_stage == COMP_SLEEP_DROWSY;
	state->sleep_stage = COMP_SLEEP_AWAKE;
	state->last_activity_ms = now_ms;
	if (state->submode == COMP_SUBMODE_SLEEP) {
		state->submode = (state->wake_submode == COMP_SUBMODE_SLEEP) ?
					 COMP_SUBMODE_HOME :
					 state->wake_submode;
	}
	return was_sleeping;
}

void companion_state_sleep_now(companion_state_t *state, uint32_t now_ms)
{
	if (!state)
		return;

	if (state->submode != COMP_SUBMODE_SLEEP) {
		state->wake_submode = state->submode;
	}
	state->submode = COMP_SUBMODE_SLEEP;
	state->sleep_stage = COMP_SLEEP_Z_ANIM;
	state->sleep_stage_since_ms = now_ms;
	state->active_event = COMP_EVT_NONE;
	state->vol_overlay_until_ms = 0;
	state->stats_overlay_until_ms = 0;
	state->led_alert_until_ms = 0;
	state->exit_holding = false;
	state->exit_hold_pct = 0;
}

bool companion_state_is_sleeping(const companion_state_t *state)
{
	if (!state)
		return false;
	return state->sleep_stage == COMP_SLEEP_Z_ANIM ||
	       state->sleep_stage == COMP_SLEEP_OLED_OFF;
}

void companion_state_cycle_submode(companion_state_t *state, uint32_t now_ms)
{
	if (!state)
		return;

	companion_state_wake(state, now_ms);

	// Short esc press cycles only interactive submodes; SLEEP stays explicit.
	switch (state->submode) {
	case COMP_SUBMODE_HOME:
		state->submode = COMP_SUBMODE_POMODORO;
		break;
	case COMP_SUBMODE_POMODORO:
		state->submode = COMP_SUBMODE_MEDIA;
		break;
	case COMP_SUBMODE_MEDIA:
		state->submode = COMP_SUBMODE_VOICE;
		break;
	case COMP_SUBMODE_VOICE:
	case COMP_SUBMODE_SLEEP:
	default:
		state->submode = COMP_SUBMODE_HOME;
		break;
	}
	state->wake_submode = state->submode;
}

static void tick_pomodoro(companion_state_t *state, uint32_t now_ms)
{
	companion_pomodoro_t *pomo = &state->pomo;
	if (!pomo->running)
		return;

	while ((uint32_t)(now_ms - pomo->last_second_ms) >= 1000UL) {
		pomo->last_second_ms += 1000UL;
		if (pomo->remaining_s > 0) {
			pomo->remaining_s--;
		}
		if (pomo->remaining_s == 0) {
			companion_pomo_phase_t next_phase =
				(pomo->phase == COMP_POMO_WORK) ?
					COMP_POMO_BREAK :
					COMP_POMO_WORK;
			pomo_load_phase(pomo, next_phase);
			pomo->phase_completed_alert = true;
			state->led_alert_mask = 0x7F;
			state->led_alert_until_ms = now_ms + 2000UL;
			break;
		}
	}
}

static void tick_sleep_pipeline(companion_state_t *state, uint32_t now_ms)
{
	if (state->submode == COMP_SUBMODE_SLEEP &&
	    state->sleep_stage == COMP_SLEEP_AWAKE) {
		state->sleep_stage = COMP_SLEEP_Z_ANIM;
		state->sleep_stage_since_ms = now_ms;
	}

	// Active media playback or voice calls block the 30-min inactivity timer.
	bool guarded = (state->submode == COMP_SUBMODE_POMODORO &&
			state->pomo.running) ||
		       ((state->host_flags & (COMP_FLAG_MEDIA_ACTIVE |
					      COMP_FLAG_VOICE_ACTIVE)) != 0);

	if (guarded && state->sleep_stage == COMP_SLEEP_AWAKE) {
		state->last_activity_ms = now_ms;
		return;
	}

	switch (state->sleep_stage) {
	case COMP_SLEEP_AWAKE:
		if ((uint32_t)(now_ms - state->last_activity_ms) >=
		    COMP_IDLE_DROWSY_MS) {
			state->sleep_stage = COMP_SLEEP_DROWSY;
			state->sleep_stage_since_ms = now_ms;
		}
		break;

	case COMP_SLEEP_DROWSY:
		if ((uint32_t)(now_ms - state->sleep_stage_since_ms) >=
		    COMP_DROWSY_DURATION_MS) {
			if (state->submode != COMP_SUBMODE_SLEEP) {
				state->wake_submode = state->submode;
			}
			state->submode = COMP_SUBMODE_SLEEP;
			state->sleep_stage = COMP_SLEEP_Z_ANIM;
			state->sleep_stage_since_ms = now_ms;
		}
		break;

	case COMP_SLEEP_Z_ANIM:
		if ((uint32_t)(now_ms - state->sleep_stage_since_ms) >=
		    COMP_SLEEP_BEFORE_OFF_MS) {
			state->sleep_stage = COMP_SLEEP_OLED_OFF;
			state->sleep_stage_since_ms = now_ms;
		}
		break;

	case COMP_SLEEP_OLED_OFF:
	default:
		break;
	}
}

void companion_state_tick(companion_state_t *state, uint32_t now_ms)
{
	if (!state)
		return;

	if (state->active_event != COMP_EVT_NONE &&
	    (int32_t)(now_ms - state->event_until_ms) >= 0) {
		state->active_event = COMP_EVT_NONE;
		state->event_label[0] = '\0';
	}

	if (state->vol_overlay_until_ms != 0 &&
	    (int32_t)(now_ms - state->vol_overlay_until_ms) >= 0) {
		state->vol_overlay_until_ms = 0;
	}

	if (state->stats_overlay_until_ms != 0 &&
	    (int32_t)(now_ms - state->stats_overlay_until_ms) >= 0) {
		state->stats_overlay_until_ms = 0;
	}

	if (state->led_alert_until_ms != 0 &&
	    (int32_t)(now_ms - state->led_alert_until_ms) >= 0) {
		state->led_alert_until_ms = 0;
		state->led_alert_mask = 0;
		state->pomo.phase_completed_alert = false;
	}

	if (state->exit_holding) {
		uint32_t elapsed = now_ms - state->exit_hold_start_ms;
		uint32_t pct = (elapsed * 100UL) / COMP_EXIT_HOLD_MS;
		state->exit_hold_pct = (pct > 100U) ? 100U : (uint8_t)pct;
	} else {
		state->exit_hold_pct = 0;
	}

	tick_pomodoro(state, now_ms);
	tick_sleep_pipeline(state, now_ms);
}

void companion_state_set_host_context(companion_state_t *state,
				      companion_host_ctx_t ctx, uint32_t flags,
				      const char *top_text,
				      const char *sub_text, uint32_t now_ms)
{
	if (!state)
		return;

	bool meaningful_change =
		(state->host_ctx != ctx) ||
		((flags & (COMP_FLAG_MEDIA_ACTIVE | COMP_FLAG_VOICE_ACTIVE |
			   COMP_FLAG_TYPING_PULSE)) != 0);

	state->host_ctx = ctx;
	state->host_flags = flags;
	safe_copy_str(state->top_text, sizeof(state->top_text), top_text);
	safe_copy_str(state->sub_text, sizeof(state->sub_text), sub_text);

	if (flags & COMP_FLAG_TYPING_PULSE) {
		state->last_typing_pulse_ms = now_ms;
	}

	if ((flags & COMP_FLAG_MIC_MUTED) == 0 && state->brb_active &&
	    ctx == COMP_CTX_VESKTOP) {
		state->brb_active = false;
	}

	if (meaningful_change) {
		companion_state_wake(state, now_ms);
	}
}

void companion_state_trigger_event(companion_state_t *state,
				   companion_event_id_t evt,
				   uint32_t duration_ms, const char *label,
				   uint32_t now_ms)
{
	if (!state || evt == COMP_EVT_NONE)
		return;

	// Deep Focus suppresses non-critical notifications while working.
	if (evt == COMP_EVT_PING && state->submode == COMP_SUBMODE_POMODORO &&
	    state->pomo.running && state->pomo.deep_focus) {
		return;
	}

	companion_state_wake(state, now_ms);

	uint32_t dur = (duration_ms > 0) ? duration_ms :
					   default_event_duration_ms(evt);
	state->active_event = evt;
	state->event_until_ms = now_ms + dur;
	safe_copy_str(state->event_label, sizeof(state->event_label), label);

	switch (evt) {
	case COMP_EVT_AGENT_ERR:
	case COMP_EVT_NEEDS_INPUT:
		state->led_alert_mask = 0x7F;
		state->led_alert_until_ms = now_ms + 1200UL;
		break;
	case COMP_EVT_GIT_COMMIT:
		state->led_alert_mask = 0x7F;
		state->led_alert_until_ms = now_ms + 900UL;
		break;
	case COMP_EVT_PING:
		state->led_alert_mask = (1U << 5); // LED #6
		state->led_alert_until_ms = now_ms + 800UL;
		break;
	case COMP_EVT_AGENT_DONE:
		state->led_alert_mask = 0x2A;
		state->led_alert_until_ms = now_ms + 800UL;
		break;
	default:
		break;
	}
}

void companion_state_set_time(companion_state_t *state, uint8_t hour,
			      uint8_t minute)
{
	if (!state || hour > 23 || minute > 59)
		return;
	state->time_hour = hour;
	state->time_minute = minute;
	state->time_valid = true;
}

void companion_state_pet(companion_state_t *state, uint32_t now_ms)
{
	if (!state)
		return;
	state->happiness = clamp_pct((int)state->happiness + 12);
	state->stats_overlay_until_ms = now_ms + COMP_STATS_OVERLAY_MS;
	companion_state_trigger_event(state, COMP_EVT_PET, 3000, "PET!",
				      now_ms);
}

void companion_state_coffee(companion_state_t *state, uint32_t now_ms)
{
	if (!state)
		return;
	state->energy = clamp_pct((int)state->energy + 18);
	state->happiness = clamp_pct((int)state->happiness + 5);
	state->stats_overlay_until_ms = now_ms + COMP_STATS_OVERLAY_MS;
	companion_state_trigger_event(state, COMP_EVT_COFFEE, 3000,
				      "ENERGIZED!", now_ms);
}

void companion_state_show_volume(companion_state_t *state, bool up, bool mute,
				 uint32_t now_ms)
{
	if (!state)
		return;
	if (mute) {
		state->vol_level = 0;
	} else if (up && state->vol_level < 7) {
		state->vol_level++;
	} else if (!up && state->vol_level > 0) {
		state->vol_level--;
	}
	state->vol_overlay_until_ms = now_ms + COMP_VOL_OVERLAY_MS;
	companion_state_touch_activity(state, now_ms);
}

void companion_pomo_toggle_run(companion_state_t *state, uint32_t now_ms)
{
	if (!state)
		return;
	state->pomo.running = !state->pomo.running;
	state->pomo.last_second_ms = now_ms;
	state->pomo.phase_completed_alert = false;
	companion_state_touch_activity(state, now_ms);
}

void companion_pomo_start(companion_state_t *state, uint32_t now_ms)
{
	if (!state)
		return;
	companion_state_wake(state, now_ms);
	state->submode = COMP_SUBMODE_POMODORO;
	state->wake_submode = COMP_SUBMODE_POMODORO;
	if (state->pomo.remaining_s == 0) {
		pomo_load_phase(&state->pomo, COMP_POMO_WORK);
	}
	state->pomo.running = true;
	state->pomo.last_second_ms = now_ms;
}

void companion_pomo_reset(companion_state_t *state)
{
	if (!state)
		return;
	pomo_load_phase(&state->pomo, state->pomo.phase);
}

void companion_pomo_skip_phase(companion_state_t *state)
{
	if (!state)
		return;
	companion_pomo_phase_t next_phase =
		(state->pomo.phase == COMP_POMO_WORK) ? COMP_POMO_BREAK :
							COMP_POMO_WORK;
	pomo_load_phase(&state->pomo, next_phase);
}

void companion_pomo_adjust_minutes(companion_state_t *state, int delta_min)
{
	if (!state)
		return;

	int32_t delta_s = (int32_t)delta_min * 60;
	int32_t next_rem = (int32_t)state->pomo.remaining_s + delta_s;
	if (next_rem < (int32_t)COMP_POMO_MIN_S)
		next_rem = (int32_t)COMP_POMO_MIN_S;
	if (next_rem > (int32_t)COMP_POMO_MAX_S)
		next_rem = (int32_t)COMP_POMO_MAX_S;

	state->pomo.remaining_s = (uint16_t)next_rem;
	if (state->pomo.remaining_s > state->pomo.total_s) {
		state->pomo.total_s = state->pomo.remaining_s;
	}
}

void companion_pomo_toggle_deep_focus(companion_state_t *state)
{
	if (!state)
		return;
	state->pomo.deep_focus = !state->pomo.deep_focus;
}

uint8_t companion_pomo_led_count(const companion_pomodoro_t *pomo)
{
	if (!pomo || pomo->total_s == 0 || pomo->remaining_s == 0)
		return 0;

	uint32_t num = (uint32_t)pomo->remaining_s * 7UL + pomo->total_s - 1UL;
	uint32_t count = num / pomo->total_s;
	return (count > 7U) ? 7U : (uint8_t)count;
}

bool companion_state_is_late_night(const companion_state_t *state)
{
	if (!state)
		return false;
	if ((state->host_flags & COMP_FLAG_LATE_NIGHT) != 0)
		return true;
	return state->time_valid &&
	       (state->time_hour >= 1 && state->time_hour < 4);
}

static companion_expr_t resolve_voice_expr(const companion_state_t *state)
{
	if (state->brb_active)
		return COMP_EXPR_VESKTOP_BRB;
	if ((state->host_flags & COMP_FLAG_DEAFENED) != 0 ||
	    state->local_deafened)
		return COMP_EXPR_VESKTOP_DEAF;
	if ((state->host_flags & COMP_FLAG_MIC_MUTED) != 0 ||
	    state->local_mic_muted)
		return COMP_EXPR_VESKTOP_MUTED;
	return COMP_EXPR_VESKTOP_ON_AIR;
}

companion_expr_t companion_state_resolve_expr(const companion_state_t *state)
{
	if (!state)
		return COMP_EXPR_NEUTRAL;

	// Priority 4 sleep states override screen rendering immediately
	if (state->sleep_stage == COMP_SLEEP_OLED_OFF)
		return COMP_EXPR_OLED_OFF;
	if (state->submode == COMP_SUBMODE_SLEEP ||
	    state->sleep_stage == COMP_SLEEP_Z_ANIM)
		return COMP_EXPR_SLEEP;
	if (state->sleep_stage == COMP_SLEEP_DROWSY)
		return COMP_EXPR_IDLE_DROWSY;

	// Priority 1: transient event reactions
	switch (state->active_event) {
	case COMP_EVT_BOOT_HELLO:
		return COMP_EXPR_BOOT_HELLO;
	case COMP_EVT_EXIT_BYE:
		return COMP_EXPR_EXIT_BYE;
	case COMP_EVT_PET:
		return COMP_EXPR_PET;
	case COMP_EVT_COFFEE:
		return COMP_EXPR_COFFEE;
	case COMP_EVT_AGENT_DONE:
		return COMP_EXPR_AGENT_DONE;
	case COMP_EVT_AGENT_ERR:
		return COMP_EXPR_AGENT_ERR;
	case COMP_EVT_NEEDS_INPUT:
		return COMP_EXPR_NEEDS_INPUT;
	case COMP_EVT_GIT_COMMIT:
		return COMP_EXPR_GIT_COMMIT;
	case COMP_EVT_PING:
		return COMP_EXPR_PING;
	case COMP_EVT_NONE:
	default:
		break;
	}

	// Priority 2: explicit interactive submode
	if (state->submode == COMP_SUBMODE_POMODORO) {
		return (state->pomo.phase == COMP_POMO_WORK) ?
			       COMP_EXPR_POMO_WORK :
			       COMP_EXPR_POMO_BREAK;
	}
	if (state->submode == COMP_SUBMODE_VOICE) {
		return resolve_voice_expr(state);
	}
	if (state->submode == COMP_SUBMODE_MEDIA) {
		return (state->host_ctx == COMP_CTX_KINO) ? COMP_EXPR_KINO :
							    COMP_EXPR_SPOTIFY;
	}

	// Priority 3: live desktop context in HOME
	if (state->brb_active)
		return COMP_EXPR_VESKTOP_BRB;

	switch (state->host_ctx) {
	case COMP_CTX_VESKTOP:
		return resolve_voice_expr(state);
	case COMP_CTX_KINO:
		return COMP_EXPR_KINO;
	case COMP_CTX_SPOTIFY:
		return COMP_EXPR_SPOTIFY;
	case COMP_CTX_AGENT_THINKING:
		return COMP_EXPR_AGENT_THINKING;
	case COMP_CTX_EMACS:
		return COMP_EXPR_CODING_EMACS;
	case COMP_CTX_TERM:
		return COMP_EXPR_CODING_TERM;
	case COMP_CTX_GAMING:
		return COMP_EXPR_GAMING;
	case COMP_CTX_IDLE:
	default:
		if ((state->host_flags & COMP_FLAG_VOICE_ACTIVE) != 0)
			return resolve_voice_expr(state);
		if ((state->host_flags & COMP_FLAG_MEDIA_ACTIVE) != 0)
			return COMP_EXPR_SPOTIFY;
		if (companion_state_is_late_night(state))
			return COMP_EXPR_LATE_NIGHT;
		return COMP_EXPR_NEUTRAL;
	}
}
