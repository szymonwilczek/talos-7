#include "unity/unity.h"

#include "companion/companion_mode.h"
#include "companion/companion_state.h"
#include "executor/actions/exec_hid_core.h"
#include "mocks/mock_companion_hw.h"

static void setup_fresh(void)
{
	mock_companion_hw_reset();
	mock_time_ms = 2000;
}

void test_init_boots_with_hello_and_expires_to_neutral(void)
{
	setup_fresh();
	companion_state_t st;
	companion_state_init(&st, 100);

	TEST_ASSERT_EQUAL(COMP_SUBMODE_HOME, st.submode);
	TEST_ASSERT_EQUAL(COMP_EXPR_BOOT_HELLO,
			  companion_state_resolve_expr(&st));

	companion_state_tick(&st, 1600);
	TEST_ASSERT_EQUAL(COMP_EXPR_NEUTRAL, companion_state_resolve_expr(&st));
}

void test_submode_cycle_skips_sleep(void)
{
	setup_fresh();
	companion_state_t st;
	companion_state_init(&st, 0);
	companion_state_tick(&st, 2000);

	companion_state_cycle_submode(&st, 2000);
	TEST_ASSERT_EQUAL(COMP_SUBMODE_POMODORO, st.submode);

	companion_state_cycle_submode(&st, 2010);
	TEST_ASSERT_EQUAL(COMP_SUBMODE_MEDIA, st.submode);

	companion_state_cycle_submode(&st, 2020);
	TEST_ASSERT_EQUAL(COMP_SUBMODE_VOICE, st.submode);

	companion_state_cycle_submode(&st, 2030);
	TEST_ASSERT_EQUAL(COMP_SUBMODE_HOME, st.submode);
}

void test_priority_transient_event_overrides_and_restores_context(void)
{
	setup_fresh();
	companion_state_t st;
	companion_state_init(&st, 0);
	companion_state_tick(&st, 2000);

	companion_state_set_host_context(&st, COMP_CTX_EMACS,
					 COMP_FLAG_TYPING_PULSE,
					 "EMACS init.el", "elisp", 2100);
	TEST_ASSERT_EQUAL(COMP_EXPR_CODING_EMACS,
			  companion_state_resolve_expr(&st));

	companion_state_trigger_event(&st, COMP_EVT_GIT_COMMIT, 4000,
				      "committed", 2200);
	TEST_ASSERT_EQUAL(COMP_EXPR_GIT_COMMIT,
			  companion_state_resolve_expr(&st));

	companion_state_tick(&st, 6200);
	TEST_ASSERT_EQUAL(COMP_EXPR_CODING_EMACS,
			  companion_state_resolve_expr(&st));
}

void test_deep_focus_suppresses_ping_only_while_working(void)
{
	setup_fresh();
	companion_state_t st;
	companion_state_init(&st, 0);
	companion_pomo_start(&st, 2000);
	companion_pomo_toggle_deep_focus(&st);

	companion_state_trigger_event(&st, COMP_EVT_PING, 3000, "DISCORD",
				      2500);
	TEST_ASSERT_EQUAL(COMP_EXPR_POMO_WORK,
			  companion_state_resolve_expr(&st));

	companion_pomo_toggle_deep_focus(&st);
	companion_state_trigger_event(&st, COMP_EVT_PING, 3000, "DISCORD",
				      2600);
	TEST_ASSERT_EQUAL(COMP_EXPR_PING, companion_state_resolve_expr(&st));
}

void test_idle_guard_blocks_sleep_during_media_or_voice(void)
{
	setup_fresh();
	companion_state_t st;
	companion_state_init(&st, 0);
	companion_state_tick(&st, 2000);

	companion_state_set_host_context(&st, COMP_CTX_KINO,
					 COMP_FLAG_MEDIA_ACTIVE, "NETFLIX", "",
					 2000);
	companion_state_tick(&st, 2000UL + 45UL * 60UL * 1000UL);
	TEST_ASSERT_EQUAL(COMP_SLEEP_AWAKE, st.sleep_stage);
	TEST_ASSERT_EQUAL(COMP_EXPR_KINO, companion_state_resolve_expr(&st));

	companion_state_set_host_context(&st, COMP_CTX_VESKTOP,
					 COMP_FLAG_VOICE_ACTIVE, "VESKTOP", "",
					 2000UL + 45UL * 60UL * 1000UL);
	companion_state_tick(&st, 2000UL + 90UL * 60UL * 1000UL);
	TEST_ASSERT_EQUAL(COMP_SLEEP_AWAKE, st.sleep_stage);
}

void test_30min_inactivity_progresses_to_oled_off(void)
{
	setup_fresh();
	companion_state_t st;
	companion_state_init(&st, 0);
	companion_state_tick(&st, 2000);

	// Just before 30 min -> still awake
	companion_state_tick(&st, COMP_IDLE_DROWSY_MS - 1UL);
	TEST_ASSERT_EQUAL(COMP_SLEEP_AWAKE, st.sleep_stage);

	// At 30 min -> DROWSY
	companion_state_tick(&st, COMP_IDLE_DROWSY_MS);
	TEST_ASSERT_EQUAL(COMP_SLEEP_DROWSY, st.sleep_stage);
	TEST_ASSERT_EQUAL(COMP_EXPR_IDLE_DROWSY,
			  companion_state_resolve_expr(&st));

	// +30s -> SLEEP Zzz
	uint32_t sleep_t = COMP_IDLE_DROWSY_MS + COMP_DROWSY_DURATION_MS;
	companion_state_tick(&st, sleep_t);
	TEST_ASSERT_EQUAL(COMP_SLEEP_Z_ANIM, st.sleep_stage);
	TEST_ASSERT_EQUAL(COMP_EXPR_SLEEP, companion_state_resolve_expr(&st));

	// +150s -> OLED_OFF
	companion_state_tick(&st, sleep_t + COMP_SLEEP_BEFORE_OFF_MS);
	TEST_ASSERT_EQUAL(COMP_SLEEP_OLED_OFF, st.sleep_stage);
	TEST_ASSERT_EQUAL(COMP_EXPR_OLED_OFF,
			  companion_state_resolve_expr(&st));
}

void test_late_night_window_01_to_04(void)
{
	setup_fresh();
	companion_state_t st;
	companion_state_init(&st, 0);
	companion_state_tick(&st, 2000);

	companion_state_set_time(&st, 0, 59);
	TEST_ASSERT_FALSE(companion_state_is_late_night(&st));
	TEST_ASSERT_EQUAL(COMP_EXPR_NEUTRAL, companion_state_resolve_expr(&st));

	companion_state_set_time(&st, 1, 0);
	TEST_ASSERT_TRUE(companion_state_is_late_night(&st));
	TEST_ASSERT_EQUAL(COMP_EXPR_LATE_NIGHT,
			  companion_state_resolve_expr(&st));

	companion_state_set_time(&st, 3, 59);
	TEST_ASSERT_EQUAL(COMP_EXPR_LATE_NIGHT,
			  companion_state_resolve_expr(&st));

	companion_state_set_time(&st, 4, 0);
	TEST_ASSERT_FALSE(companion_state_is_late_night(&st));
	TEST_ASSERT_EQUAL(COMP_EXPR_NEUTRAL, companion_state_resolve_expr(&st));
}

void test_wake_consumes_first_press_without_triggering_action(void)
{
	setup_fresh();
	companion_mode_enter(0);
	companion_state_t *st = companion_mode_get_state();
	companion_state_tick(st, 2000);

	// Put Companion to sleep via HOME button 6 (index 5)
	companion_mode_handle_button_event(5, true, 2100);
	companion_mode_handle_button_event(5, false, 2150);
	TEST_ASSERT_TRUE(companion_state_is_sleeping(st));
	TEST_ASSERT_EQUAL(COMP_EXPR_SLEEP, companion_state_resolve_expr(st));

	// Advance until OLED_OFF
	companion_state_tick(st, 2100 + COMP_SLEEP_BEFORE_OFF_MS);
	TEST_ASSERT_EQUAL(COMP_EXPR_OLED_OFF, companion_state_resolve_expr(st));

	// First click on key [4] (index 3, Pet) wakes up, does NOT trigger PET event
	// Keys [1..3] (indices 0..2) without keycaps are ignored completely.
	uint8_t hap_before = st->happiness;
	companion_mode_handle_button_event(0, true, 199000);
	TEST_ASSERT_TRUE(companion_state_is_sleeping(st));

	companion_mode_handle_button_event(3, true, 200000);
	companion_mode_handle_button_event(3, false, 200050);
	TEST_ASSERT_FALSE(companion_state_is_sleeping(st));
	TEST_ASSERT_EQUAL(COMP_SUBMODE_HOME, st->submode);
	TEST_ASSERT_EQUAL(COMP_EVT_NONE, st->active_event);
	TEST_ASSERT_EQUAL(hap_before, st->happiness);

	// Second click on [4] (index 3) now pets normally; [5] triggers Coffee
	companion_mode_handle_button_event(3, true, 200100);
	TEST_ASSERT_EQUAL(COMP_EVT_PET, st->active_event);
	TEST_ASSERT_TRUE(st->happiness > hap_before);

	companion_mode_handle_button_event(4, true, 200200);
	TEST_ASSERT_EQUAL(COMP_EVT_COFFEE, st->active_event);
	companion_mode_exit();
}

void test_led_masks_follow_rules_and_defaults_off(void)
{
	setup_fresh();
	companion_state_t st;
	companion_state_init(&st, 0);
	companion_state_tick(&st, 2000);

	// Default HOME = all 7 red LEDs OFF
	TEST_ASSERT_EQUAL_UINT8(0x00,
				companion_mode_compute_led_mask(&st, 2000));

	// Pomodoro WORK shows full 7-LED bar at start
	companion_pomo_start(&st, 2000);
	TEST_ASSERT_EQUAL_UINT8(0x7F,
				companion_mode_compute_led_mask(&st, 2000));

	// Cinema mode forces LEDs OFF outside brief volume overlay
	st.submode = COMP_SUBMODE_MEDIA;
	companion_state_set_host_context(&st, COMP_CTX_KINO,
					 COMP_FLAG_MEDIA_ACTIVE, "NETFLIX", "",
					 2100);
	TEST_ASSERT_EQUAL_UINT8(0x00,
				companion_mode_compute_led_mask(&st, 2100));

	// Volume overlay briefly shows segments then turns off again
	companion_state_show_volume(&st, true, false, 2200);
	TEST_ASSERT_NOT_EQUAL(0x00, companion_mode_compute_led_mask(&st, 2300));
	TEST_ASSERT_EQUAL_UINT8(0x00,
				companion_mode_compute_led_mask(&st, 3800));

	// Vesktop muted lights LED #1, deafened lights LED #1 + #2
	st.submode = COMP_SUBMODE_VOICE;
	st.local_mic_muted = true;
	TEST_ASSERT_EQUAL_UINT8(0x01,
				companion_mode_compute_led_mask(&st, 4000));
	st.local_deafened = true;
	TEST_ASSERT_EQUAL_UINT8(0x03,
				companion_mode_compute_led_mask(&st, 4000));
}

void test_media_and_voice_buttons_emit_hid_actions(void)
{
	setup_fresh();
	companion_mode_enter(0);
	companion_state_t *st = companion_mode_get_state();
	companion_state_tick(st, 2000);

	// Cycle to POMODORO: test tap Start/Pause vs 700ms hold Reset on [4] (index 3)
	companion_mode_handle_button_event(6, true, 2000);
	companion_mode_handle_button_event(6, false, 2050);
	TEST_ASSERT_EQUAL(COMP_SUBMODE_POMODORO, st->submode);

	companion_mode_handle_button_event(3, true, 2060);
	companion_mode_handle_button_event(3, false,
					   2120); // short tap -> Start
	TEST_ASSERT_TRUE(st->pomo.running);
	companion_mode_handle_button_event(3, true, 2200);
	companion_mode_handle_button_event(3, false,
					   2950); // hold >= 700ms -> Reset
	TEST_ASSERT_FALSE(st->pomo.running);

	// Cycle to MEDIA
	companion_mode_handle_button_event(6, true, 3000);
	companion_mode_handle_button_event(6, false, 3050);
	TEST_ASSERT_EQUAL(COMP_SUBMODE_MEDIA, st->submode);

	// Short tap [5] (index 4) -> Play/Pause
	companion_mode_handle_button_event(4, true, 3100);
	companion_mode_handle_button_event(4, false, 3160);
	TEST_ASSERT_EQUAL(TALOS_CONSUMER_PLAY_PAUSE, mock_last_consumer_usage);
	TEST_ASSERT_EQUAL(1, mock_consumer_count);

	// Hold [5] (index 4) >= 700ms -> Mute toggles vol_muted WITHOUT wiping vol_pct
	companion_state_set_volume(st, 65, false, 3200, false);
	companion_mode_handle_button_event(4, true, 3300);
	companion_mode_handle_button_event(4, false, 4050);
	TEST_ASSERT_EQUAL(TALOS_CONSUMER_MUTE, mock_last_consumer_usage);
	TEST_ASSERT_TRUE(st->vol_muted);
	TEST_ASSERT_EQUAL_UINT8(65, st->vol_pct);
	TEST_ASSERT_EQUAL_UINT8(0, st->vol_level);

	// Second Mute hold restores unmuted state and original vol_pct
	companion_mode_handle_button_event(4, true, 4100);
	companion_mode_handle_button_event(4, false, 4850);
	TEST_ASSERT_FALSE(st->vol_muted);
	TEST_ASSERT_EQUAL_UINT8(65, st->vol_pct);
	TEST_ASSERT_TRUE(st->vol_level > 0);

	// Short [6] (index 5) -> Vol+, Hold [6] -> Next Track
	companion_mode_handle_button_event(5, true, 4900);
	companion_mode_handle_button_event(5, false, 4950);
	TEST_ASSERT_EQUAL(TALOS_CONSUMER_VOLUME_UP, mock_last_consumer_usage);
	TEST_ASSERT_EQUAL_UINT8(70, st->vol_pct);

	companion_mode_handle_button_event(5, true, 5000);
	companion_mode_handle_button_event(5, false, 5750);
	TEST_ASSERT_EQUAL(TALOS_CONSUMER_SCAN_NEXT, mock_last_consumer_usage);

	// Cycle to VOICE
	companion_mode_handle_button_event(6, true, 5800);
	companion_mode_handle_button_event(6, false, 5850);
	TEST_ASSERT_EQUAL(COMP_SUBMODE_VOICE, st->submode);

	// [4]=Mute Mic (F13), [5]=Deafen (F14), [6]=BRB (index 5)
	companion_mode_handle_button_event(3, true, 5900);
	TEST_ASSERT_TRUE(st->local_mic_muted);
	TEST_ASSERT_EQUAL(COMP_HID_KEY_MUTE_MIC, mock_last_hid_keycode);

	companion_mode_handle_button_event(3, true, 5950); // unmute mic
	TEST_ASSERT_FALSE(st->local_mic_muted);

	companion_mode_handle_button_event(4, true, 6000);
	TEST_ASSERT_TRUE(st->local_deafened);
	TEST_ASSERT_EQUAL(COMP_HID_KEY_DEAFEN, mock_last_hid_keycode);

	// BRB button (index 5) enables BRB and auto-mutes mic via F13
	companion_mode_handle_button_event(5, true, 6100);
	TEST_ASSERT_TRUE(st->brb_active);
	TEST_ASSERT_TRUE(st->local_mic_muted);
	TEST_ASSERT_EQUAL(COMP_HID_KEY_MUTE_MIC, mock_last_hid_keycode);
	companion_mode_exit();
}

void run_companion_state_tests(void)
{
	printf("\n=== Companion State & Mode Tests ===\n");
	RUN_TEST(test_init_boots_with_hello_and_expires_to_neutral);
	RUN_TEST(test_submode_cycle_skips_sleep);
	RUN_TEST(test_priority_transient_event_overrides_and_restores_context);
	RUN_TEST(test_deep_focus_suppresses_ping_only_while_working);
	RUN_TEST(test_idle_guard_blocks_sleep_during_media_or_voice);
	RUN_TEST(test_30min_inactivity_progresses_to_oled_off);
	RUN_TEST(test_late_night_window_01_to_04);
	RUN_TEST(test_wake_consumes_first_press_without_triggering_action);
	RUN_TEST(test_led_masks_follow_rules_and_defaults_off);
	RUN_TEST(test_media_and_voice_buttons_emit_hid_actions);
}
