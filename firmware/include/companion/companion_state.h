#ifndef COMPANION_STATE_H
#define COMPANION_STATE_H

#include <stdbool.h>
#include <stdint.h>

#define COMP_TEXT_MAX_LEN 48
#define COMP_SUB_MAX_LEN 24

#define COMP_IDLE_DROWSY_MS (30UL * 60UL * 1000UL)
#define COMP_DROWSY_DURATION_MS (30UL * 1000UL)
#define COMP_SLEEP_BEFORE_OFF_MS (150UL * 1000UL)
#define COMP_VOL_OVERLAY_MS 1500UL
#define COMP_STATS_OVERLAY_MS 3000UL
#define COMP_EXIT_HOLD_MS 1500UL
#define COMP_CI_QUEUE_CAP 4U

#define COMP_POMO_DEFAULT_WORK_S (25U * 60U)
#define COMP_POMO_DEFAULT_BREAK_S (5U * 60U)
#define COMP_POMO_MIN_S (5U * 60U)
#define COMP_POMO_MAX_S (60U * 60U)
#define COMP_POMO_STEP_S (5U * 60U)

#define COMP_FLAG_MEDIA_ACTIVE (1U << 0)
#define COMP_FLAG_VOICE_ACTIVE (1U << 1)
#define COMP_FLAG_MIC_MUTED (1U << 2)
#define COMP_FLAG_DEAFENED (1U << 3)
#define COMP_FLAG_LATE_NIGHT (1U << 4)
#define COMP_FLAG_TYPING_PULSE (1U << 5)
#define COMP_FLAG_VIDEO_PAUSED (1U << 6)
#define COMP_FLAG_LOAD_T2 (1U << 7)
#define COMP_FLAG_LOAD_T3 (1U << 8)

typedef enum {
	COMP_SUBMODE_HOME = 0,
	COMP_SUBMODE_POMODORO,
	COMP_SUBMODE_MEDIA,
	COMP_SUBMODE_VOICE,
	COMP_SUBMODE_SLEEP
} companion_submode_t;

typedef enum {
	COMP_CTX_IDLE = 0,
	COMP_CTX_EMACS,
	COMP_CTX_TERM,
	COMP_CTX_AGENT_THINKING,
	COMP_CTX_VESKTOP,
	COMP_CTX_SPOTIFY,
	COMP_CTX_KINO,
	COMP_CTX_GAMING,
	COMP_CTX_LOAD
} companion_host_ctx_t;

typedef enum {
	COMP_EVT_NONE = 0,
	COMP_EVT_BOOT_HELLO,
	COMP_EVT_EXIT_BYE,
	COMP_EVT_PET,
	COMP_EVT_COFFEE,
	COMP_EVT_AGENT_DONE,
	COMP_EVT_AGENT_ERR,
	COMP_EVT_NEEDS_INPUT,
	COMP_EVT_GIT_COMMIT,
	COMP_EVT_PING
} companion_event_id_t;

typedef enum {
	COMP_SLEEP_AWAKE = 0,
	COMP_SLEEP_DROWSY,
	COMP_SLEEP_Z_ANIM,
	COMP_SLEEP_OLED_OFF
} companion_sleep_stage_t;

typedef enum {
	COMP_POMO_WORK = 0,
	COMP_POMO_BREAK
} companion_pomo_phase_t;

typedef enum {
	COMP_EXPR_BOOT_HELLO = 0,
	COMP_EXPR_EXIT_BYE,
	COMP_EXPR_NEUTRAL,
	COMP_EXPR_CODING_EMACS,
	COMP_EXPR_CODING_TERM,
	COMP_EXPR_AGENT_THINKING,
	COMP_EXPR_AGENT_DONE,
	COMP_EXPR_AGENT_ERR,
	COMP_EXPR_NEEDS_INPUT,
	COMP_EXPR_GIT_COMMIT,
	COMP_EXPR_PING,
	COMP_EXPR_VESKTOP_ON_AIR,
	COMP_EXPR_VESKTOP_MUTED,
	COMP_EXPR_VESKTOP_DEAF,
	COMP_EXPR_VESKTOP_BRB,
	COMP_EXPR_SPOTIFY,
	COMP_EXPR_KINO,
	COMP_EXPR_GAMING,
	COMP_EXPR_LOAD_WARM,
	COMP_EXPR_LOAD_HOT,
	COMP_EXPR_LOAD_CRITICAL,
	COMP_EXPR_CI_GREEN,
	COMP_EXPR_POMO_WORK,
	COMP_EXPR_POMO_BREAK,
	COMP_EXPR_PET,
	COMP_EXPR_COFFEE,
	COMP_EXPR_LATE_NIGHT,
	COMP_EXPR_IDLE_DROWSY,
	COMP_EXPR_SLEEP,
	COMP_EXPR_OLED_OFF
} companion_expr_t;

typedef struct {
	companion_pomo_phase_t phase;
	bool running;
	bool deep_focus;
	bool phase_completed_alert;
	uint16_t work_duration_s;
	uint16_t break_duration_s;
	uint16_t total_s;
	uint16_t remaining_s;
	uint32_t last_second_ms;
} companion_pomodoro_t;

typedef struct {
	companion_submode_t submode;
	companion_submode_t wake_submode;
	companion_host_ctx_t host_ctx;
	uint32_t host_flags;
	char top_text[COMP_TEXT_MAX_LEN];
	char sub_text[COMP_SUB_MAX_LEN];
	bool media_playing;
	char media_title[COMP_TEXT_MAX_LEN];

	companion_event_id_t active_event;
	uint32_t event_until_ms;
	char event_label[COMP_SUB_MAX_LEN];

	companion_sleep_stage_t sleep_stage;
	uint32_t sleep_stage_since_ms;
	uint32_t last_activity_ms;
	uint32_t last_typing_pulse_ms;

	uint8_t happiness; // 0..100
	uint8_t energy; // 0..100
	bool brb_active;
	bool local_mic_muted;
	bool local_deafened;
	uint8_t gaming_variant;
	uint8_t gaming_nonce;

	char ci_queue[COMP_CI_QUEUE_CAP][COMP_TEXT_MAX_LEN];
	uint8_t ci_queue_count;
	uint32_t ci_since_ms;

	uint8_t time_hour;
	uint8_t time_minute;
	bool time_valid;

	uint8_t vol_level; // 0..7 segments
	uint8_t vol_pct; // 0..100 synchronized system volume
	bool vol_muted;
	uint32_t vol_overlay_until_ms;
	uint32_t stats_overlay_until_ms;
	uint32_t led_alert_until_ms;
	uint8_t led_alert_mask;

	bool exit_holding;
	uint32_t exit_hold_start_ms;
	uint8_t exit_hold_pct;

	companion_pomodoro_t pomo;
} companion_state_t;

void companion_state_init(companion_state_t *state, uint32_t now_ms);
void companion_state_tick(companion_state_t *state, uint32_t now_ms);
void companion_state_cycle_submode(companion_state_t *state, uint32_t now_ms);
void companion_state_touch_activity(companion_state_t *state, uint32_t now_ms);
bool companion_state_wake(companion_state_t *state, uint32_t now_ms);
void companion_state_sleep_now(companion_state_t *state, uint32_t now_ms);
bool companion_state_is_sleeping(const companion_state_t *state);

void companion_state_set_host_context(companion_state_t *state,
				      companion_host_ctx_t ctx, uint32_t flags,
				      const char *top_text,
				      const char *sub_text, uint32_t now_ms);
void companion_state_set_media(companion_state_t *state, bool playing,
			       const char *title);
void companion_state_push_ci(companion_state_t *state, const char *label,
			     uint32_t now_ms);
bool companion_state_dismiss_ci(companion_state_t *state, uint32_t now_ms);
void companion_state_clear_ci(companion_state_t *state);
void companion_state_trigger_event(companion_state_t *state,
				   companion_event_id_t evt,
				   uint32_t duration_ms, const char *label,
				   uint32_t now_ms);
void companion_state_set_time(companion_state_t *state, uint8_t hour,
			      uint8_t minute);

void companion_state_pet(companion_state_t *state, uint32_t now_ms);
void companion_state_coffee(companion_state_t *state, uint32_t now_ms);
void companion_state_show_volume(companion_state_t *state, bool up, bool mute,
				 uint32_t now_ms);
void companion_state_set_volume(companion_state_t *state, uint8_t pct,
				bool muted, uint32_t now_ms, bool show_overlay);

void companion_pomo_toggle_run(companion_state_t *state, uint32_t now_ms);
void companion_pomo_start(companion_state_t *state, uint32_t now_ms);
void companion_pomo_reset(companion_state_t *state);
void companion_pomo_skip_phase(companion_state_t *state);
void companion_pomo_adjust_minutes(companion_state_t *state, int delta_min);
void companion_pomo_toggle_deep_focus(companion_state_t *state);
uint8_t companion_pomo_led_count(const companion_pomodoro_t *pomo);

companion_expr_t companion_state_resolve_expr(const companion_state_t *state);
bool companion_state_is_late_night(const companion_state_t *state);

#endif // COMPANION_STATE_H
