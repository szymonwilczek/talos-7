#include "companion/companion_mode.h"

#include "companion/companion_renderer.h"
#include "executor/actions/exec_hid_core.h"
#include "hardware_interface.h"
#include "pin_definitions.h"
#include <string.h>

#define COMP_BTN_ESC_IDX 6
#define COMP_DEBOUNCE_MS 12UL
#define COMP_RENDER_INTERVAL_MS 33UL
#define COMP_EXIT_BYE_MS 650UL

static companion_state_t s_state;
static bool s_active = false;
static bool s_exiting = false;
static uint32_t s_exit_done_ms = 0;
static uint32_t s_last_render_ms = 0;
static bool s_btn_prev[NUM_BUTTONS];
static uint32_t s_btn_last_edge_ms[NUM_BUTTONS];
static bool s_ignore_esc_release = false;
static uint8_t s_last_led_mask = 0xFF;
static uint8_t s_last_rgb_code = 0xFF;

static uint8_t led_mask_n_segments(uint8_t count)
{
	if (count >= 7U)
		return 0x7F;
	if (count == 0U)
		return 0x00;
	return (uint8_t)((1U << count) - 1U);
}

uint8_t companion_mode_compute_led_mask(const companion_state_t *state,
					uint32_t now_ms)
{
	if (!state)
		return 0;

	companion_expr_t expr = companion_state_resolve_expr(state);
	if (expr == COMP_EXPR_OLED_OFF || expr == COMP_EXPR_SLEEP) {
		return 0;
	}

	// Transient volume bar shows briefly even in cinema mode
	if (state->vol_overlay_until_ms != 0 &&
	    (int32_t)(state->vol_overlay_until_ms - now_ms) > 0) {
		return led_mask_n_segments(state->vol_level);
	}

	// Keep cinema dark outside volume adjustments
	if (expr == COMP_EXPR_KINO) {
		return 0;
	}

	// Transient event alerts (blink on errors/input requests)
	if (state->led_alert_until_ms != 0 &&
	    (int32_t)(state->led_alert_until_ms - now_ms) > 0) {
		if (expr == COMP_EXPR_AGENT_ERR ||
		    expr == COMP_EXPR_NEEDS_INPUT) {
			return (((now_ms / 150UL) % 2U) == 0U) ?
				       state->led_alert_mask :
				       0;
		}
		return state->led_alert_mask;
	}

	// Pomodoro progress bar on 7 red LEDs during work
	if (state->submode == COMP_SUBMODE_POMODORO &&
	    state->pomo.phase == COMP_POMO_WORK) {
		uint8_t count = companion_pomo_led_count(&state->pomo);
		uint8_t mask = led_mask_n_segments(count);
		if (!state->pomo.running && count > 0 &&
		    ((now_ms / 450UL) % 2U) == 1U) {
			mask &= (uint8_t)~(1U << (count - 1U));
		}
		return mask;
	}

	// Unmistakable mute/deafen status LEDs for Vesktop
	if (expr == COMP_EXPR_VESKTOP_DEAF) {
		return 0x03; // LED #1 + LED #2
	}
	if (expr == COMP_EXPR_VESKTOP_MUTED || expr == COMP_EXPR_VESKTOP_BRB) {
		return 0x01; // LED #1
	}

	return 0;
}

static void update_companion_leds(uint32_t now_ms)
{
	uint8_t mask = companion_mode_compute_led_mask(&s_state, now_ms);
	if (mask != s_last_led_mask) {
		s_last_led_mask = mask;
		for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
			led_set(i, (mask & (1U << i)) != 0);
		}
	}

	companion_expr_t expr = companion_state_resolve_expr(&s_state);
	bool g = false;
	bool b = false;

	switch (expr) {
	case COMP_EXPR_AGENT_DONE:
	case COMP_EXPR_GIT_COMMIT:
	case COMP_EXPR_VESKTOP_ON_AIR:
	case COMP_EXPR_POMO_BREAK:
	case COMP_EXPR_COFFEE:
		g = true;
		break;
	case COMP_EXPR_CODING_EMACS:
	case COMP_EXPR_SPOTIFY:
	case COMP_EXPR_PET:
	case COMP_EXPR_BOOT_HELLO:
	case COMP_EXPR_PING:
		g = true;
		b = true;
		break;
	case COMP_EXPR_POMO_WORK:
	case COMP_EXPR_CODING_TERM:
	case COMP_EXPR_VESKTOP_BRB:
		b = true;
		break;
	case COMP_EXPR_AGENT_THINKING:
		b = true;
		g = ((now_ms / 600UL) % 2U) == 0U;
		break;
	case COMP_EXPR_AGENT_ERR:
	case COMP_EXPR_NEEDS_INPUT:
		b = ((now_ms / 180UL) % 2U) == 0U;
		break;
	default:
		break;
	}

	// Note: RGB red channel is broken on hardware, use green/blue/cyan only
	uint8_t rgb_code = (uint8_t)((g ? 2U : 0U) | (b ? 1U : 0U));
	if (rgb_code != s_last_rgb_code) {
		s_last_rgb_code = rgb_code;
		led_rgb_set(false, g, b);
	}
}

static void handle_home_button(uint8_t btn, uint32_t now_ms)
{
	switch (btn) {
	case 0:
		companion_state_pet(&s_state, now_ms);
		break;
	case 1:
		companion_state_coffee(&s_state, now_ms);
		break;
	case 2:
		companion_pomo_start(&s_state, now_ms);
		break;
	case 3:
		s_state.local_mic_muted = !s_state.local_mic_muted;
		press_sequence(0, COMP_HID_KEY_MUTE_MIC);
		companion_state_touch_activity(&s_state, now_ms);
		break;
	case 4:
		exec_hid_consumer_press(TALOS_CONSUMER_PLAY_PAUSE);
		companion_state_touch_activity(&s_state, now_ms);
		break;
	case 5:
		companion_state_sleep_now(&s_state, now_ms);
		break;
	default:
		break;
	}
}

static void handle_pomo_button(uint8_t btn, uint32_t now_ms)
{
	switch (btn) {
	case 0:
		companion_pomo_toggle_run(&s_state, now_ms);
		break;
	case 1:
		companion_pomo_reset(&s_state);
		companion_state_touch_activity(&s_state, now_ms);
		break;
	case 2:
		companion_pomo_skip_phase(&s_state);
		companion_state_touch_activity(&s_state, now_ms);
		break;
	case 3:
		companion_pomo_adjust_minutes(&s_state, -5);
		companion_state_touch_activity(&s_state, now_ms);
		break;
	case 4:
		companion_pomo_adjust_minutes(&s_state, 5);
		companion_state_touch_activity(&s_state, now_ms);
		break;
	case 5:
		companion_pomo_toggle_deep_focus(&s_state);
		companion_state_touch_activity(&s_state, now_ms);
		break;
	default:
		break;
	}
}

static void handle_media_button(uint8_t btn, uint32_t now_ms)
{
	switch (btn) {
	case 0:
		exec_hid_consumer_press(TALOS_CONSUMER_SCAN_PREV);
		companion_state_touch_activity(&s_state, now_ms);
		break;
	case 1:
		exec_hid_consumer_press(TALOS_CONSUMER_PLAY_PAUSE);
		companion_state_touch_activity(&s_state, now_ms);
		break;
	case 2:
		exec_hid_consumer_press(TALOS_CONSUMER_SCAN_NEXT);
		companion_state_touch_activity(&s_state, now_ms);
		break;
	case 3:
		exec_hid_consumer_press(TALOS_CONSUMER_VOLUME_DOWN);
		companion_state_show_volume(&s_state, false, false, now_ms);
		break;
	case 4:
		exec_hid_consumer_press(TALOS_CONSUMER_MUTE);
		companion_state_show_volume(&s_state, false, true, now_ms);
		break;
	case 5:
		exec_hid_consumer_press(TALOS_CONSUMER_VOLUME_UP);
		companion_state_show_volume(&s_state, true, false, now_ms);
		break;
	default:
		break;
	}
}

static void handle_voice_button(uint8_t btn, uint32_t now_ms)
{
	switch (btn) {
	case 0:
		s_state.local_mic_muted = !s_state.local_mic_muted;
		if (!s_state.local_mic_muted) {
			s_state.brb_active = false;
		}
		press_sequence(0, COMP_HID_KEY_MUTE_MIC);
		companion_state_touch_activity(&s_state, now_ms);
		break;
	case 1:
		s_state.local_deafened = !s_state.local_deafened;
		press_sequence(0, COMP_HID_KEY_DEAFEN);
		companion_state_touch_activity(&s_state, now_ms);
		break;
	case 2:
		exec_hid_consumer_press(TALOS_CONSUMER_PLAY_PAUSE);
		companion_state_touch_activity(&s_state, now_ms);
		break;
	case 3:
		exec_hid_consumer_press(TALOS_CONSUMER_VOLUME_DOWN);
		companion_state_show_volume(&s_state, false, false, now_ms);
		break;
	case 4:
		exec_hid_consumer_press(TALOS_CONSUMER_VOLUME_UP);
		companion_state_show_volume(&s_state, true, false, now_ms);
		break;
	case 5: {
		s_state.brb_active = !s_state.brb_active;
		bool already_muted =
			s_state.local_mic_muted ||
			((s_state.host_flags & COMP_FLAG_MIC_MUTED) != 0);
		if (s_state.brb_active && !already_muted) {
			s_state.local_mic_muted = true;
			press_sequence(0, COMP_HID_KEY_MUTE_MIC);
		}
		companion_state_touch_activity(&s_state, now_ms);
		break;
	}
	default:
		break;
	}
}

void companion_mode_enter(uint32_t now_ms)
{
	companion_state_init(&s_state, now_ms);
	companion_renderer_init();
	s_active = true;
	s_exiting = false;
	s_exit_done_ms = 0;
	s_last_render_ms = 0;
	s_ignore_esc_release = false;
	s_last_led_mask = 0xFF;
	s_last_rgb_code = 0xFF;

	// Seed current physical button states so the entry press never fires a submode action
	for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
		s_btn_prev[i] = button_is_pressed(i);
		s_btn_last_edge_ms[i] = now_ms;
	}
}

void companion_mode_exit(void)
{
	s_active = false;
	s_exiting = false;
	s_state.exit_holding = false;
	s_state.exit_hold_pct = 0;
	for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
		led_set(i, false);
	}
	led_rgb_update_os(detect_platform());
	leds_refresh();
}

bool companion_mode_is_active(void)
{
	return s_active;
}

companion_state_t *companion_mode_get_state(void)
{
	return &s_state;
}

void companion_mode_handle_button_event(uint8_t button_idx, bool pressed,
					uint32_t now_ms)
{
	if (!s_active || s_exiting || button_idx >= NUM_BUTTONS)
		return;

	if (pressed && companion_state_is_sleeping(&s_state)) {
		// First click wakes Companion without triggering any submode action
		companion_state_wake(&s_state, now_ms);
		if (button_idx == COMP_BTN_ESC_IDX) {
			s_ignore_esc_release = true;
		}
		return;
	}

	if (button_idx == COMP_BTN_ESC_IDX) {
		if (pressed) {
			s_state.exit_holding = true;
			s_state.exit_hold_start_ms = now_ms;
			s_state.exit_hold_pct = 0;
		} else {
			s_state.exit_holding = false;
			s_state.exit_hold_pct = 0;
			if (s_ignore_esc_release) {
				s_ignore_esc_release = false;
			} else {
				companion_state_cycle_submode(&s_state, now_ms);
			}
		}
		return;
	}

	if (!pressed)
		return;

	switch (s_state.submode) {
	case COMP_SUBMODE_HOME:
		handle_home_button(button_idx, now_ms);
		break;
	case COMP_SUBMODE_POMODORO:
		handle_pomo_button(button_idx, now_ms);
		break;
	case COMP_SUBMODE_MEDIA:
		handle_media_button(button_idx, now_ms);
		break;
	case COMP_SUBMODE_VOICE:
		handle_voice_button(button_idx, now_ms);
		break;
	case COMP_SUBMODE_SLEEP:
	default:
		break;
	}
}

void companion_mode_task(uint32_t now_ms)
{
	if (!s_active)
		return;

	for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
		bool raw = button_is_pressed(i);
		if (raw != s_btn_prev[i] &&
		    (uint32_t)(now_ms - s_btn_last_edge_ms[i]) >=
			    COMP_DEBOUNCE_MS) {
			s_btn_prev[i] = raw;
			s_btn_last_edge_ms[i] = now_ms;
			companion_mode_handle_button_event(i, raw, now_ms);
		}
	}

	companion_state_tick(&s_state, now_ms);

	if (!s_exiting && s_state.exit_holding &&
	    s_state.exit_hold_pct >= 100U) {
		s_state.exit_holding = false;
		s_exiting = true;
		s_exit_done_ms = now_ms + COMP_EXIT_BYE_MS;
		companion_state_trigger_event(&s_state, COMP_EVT_EXIT_BYE,
					      COMP_EXIT_BYE_MS, "BYE", now_ms);
	}

	if (s_exiting && (int32_t)(now_ms - s_exit_done_ms) >= 0) {
		companion_mode_exit();
		return;
	}

	update_companion_leds(now_ms);

	if (s_last_render_ms == 0 ||
	    (uint32_t)(now_ms - s_last_render_ms) >= COMP_RENDER_INTERVAL_MS) {
		s_last_render_ms = now_ms;
		companion_renderer_render(&s_state, now_ms);
	}
}
