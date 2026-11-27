#include "unity/unity.h"

#include "cdc/commands/cdc_cmd_companion.h"
#include "cdc/commands/cdc_cmd_system.h"
#include "companion/companion_state.h"
#include "macro_config.h"
#include "mocks/mock_tusb.h"
#include <stddef.h>
#include <string.h>

void test_comp_cdc_parse_state_named_and_numeric(void)
{
	companion_state_t st;
	companion_state_init(&st, 0);
	companion_state_tick(&st, 2000);

	TEST_ASSERT_TRUE(companion_cdc_apply_state(
		"EMACS|32|EMACS init.el|elisp", &st, 2100));
	TEST_ASSERT_EQUAL(COMP_CTX_EMACS, st.host_ctx);
	TEST_ASSERT_EQUAL_UINT32(32U, st.host_flags);
	TEST_ASSERT_EQUAL_STRING("EMACS init.el", st.top_text);
	TEST_ASSERT_EQUAL_STRING("elisp", st.sub_text);
	TEST_ASSERT_EQUAL_UINT32(2100U, st.last_typing_pulse_ms);

	TEST_ASSERT_TRUE(
		companion_cdc_apply_state("6|65|NETFLIX|Playing", &st, 2200));
	TEST_ASSERT_EQUAL(COMP_CTX_KINO, st.host_ctx);
	TEST_ASSERT_EQUAL_UINT32(65U, st.host_flags);
	TEST_ASSERT_EQUAL_STRING("NETFLIX", st.top_text);
}

void test_comp_cdc_parse_state_rejects_malformed(void)
{
	companion_state_t st;
	companion_state_init(&st, 0);

	TEST_ASSERT_FALSE(companion_cdc_apply_state("EMACS", &st, 100));
	TEST_ASSERT_FALSE(companion_cdc_apply_state("UNKNOWN|0|x|y", &st, 100));
	TEST_ASSERT_FALSE(companion_cdc_apply_state("99|0|x|y", &st, 100));
	TEST_ASSERT_FALSE(
		companion_cdc_apply_state("EMACS|badflags|x|y", &st, 100));
}

void test_comp_cdc_parse_event_and_time(void)
{
	companion_state_t st;
	companion_state_init(&st, 0);
	companion_state_tick(&st, 2000);

	TEST_ASSERT_TRUE(
		companion_cdc_apply_event("GIT_COMMIT|0|main", &st, 2500));
	TEST_ASSERT_EQUAL(COMP_EVT_GIT_COMMIT, st.active_event);
	TEST_ASSERT_EQUAL_UINT32(6500U, st.event_until_ms);
	TEST_ASSERT_EQUAL_STRING("main", st.event_label);

	TEST_ASSERT_TRUE(
		companion_cdc_apply_event("AGENT_ERR|1500|ci", &st, 2600));
	TEST_ASSERT_EQUAL(COMP_EVT_AGENT_ERR, st.active_event);
	TEST_ASSERT_EQUAL_UINT32(4100U, st.event_until_ms);

	TEST_ASSERT_FALSE(companion_cdc_apply_event("NOPE|1000|x", &st, 2700));

	TEST_ASSERT_TRUE(companion_cdc_apply_time("02|45", &st));
	TEST_ASSERT_EQUAL_UINT8(2, st.time_hour);
	TEST_ASSERT_EQUAL_UINT8(45, st.time_minute);
	TEST_ASSERT_TRUE(st.time_valid);

	TEST_ASSERT_FALSE(companion_cdc_apply_time("24|00", &st));
	TEST_ASSERT_FALSE(companion_cdc_apply_time("12|60", &st));
	TEST_ASSERT_FALSE(companion_cdc_apply_time("12", &st));
}

void test_companion_autostart_after_crc_and_cdc_handler(void)
{
	TEST_ASSERT_EQUAL(12, MACRO_TYPE_COMPANION);
	TEST_ASSERT_TRUE(offsetof(config_data_t, companion_autostart) >
			 offsetof(config_data_t, crc32));

	mock_cdc_reset_output();
	config_get()->companion_autostart = 0;

	cmd_handle_set_companion_autostart("1");
	TEST_ASSERT_EQUAL_UINT8(1, config_get()->companion_autostart);
	TEST_ASSERT_EQUAL_STRING("OK", mock_cdc_output);

	mock_cdc_reset_output();
	cmd_handle_set_companion_autostart("0");
	TEST_ASSERT_EQUAL_UINT8(0, config_get()->companion_autostart);
	TEST_ASSERT_EQUAL_STRING("OK", mock_cdc_output);

	mock_cdc_reset_output();
	cmd_handle_set_companion_autostart("2");
	TEST_ASSERT_EQUAL_STRING("ERROR|Invalid parameters", mock_cdc_output);

	mock_cdc_reset_output();
	cmd_handle_set_companion_autostart("");
	TEST_ASSERT_EQUAL_STRING("ERROR|Invalid parameters", mock_cdc_output);
}

void run_companion_cdc_tests(void)
{
	printf("\n=== Companion CDC & Config Tests ===\n");
	RUN_TEST(test_comp_cdc_parse_state_named_and_numeric);
	RUN_TEST(test_comp_cdc_parse_state_rejects_malformed);
	RUN_TEST(test_comp_cdc_parse_event_and_time);
	RUN_TEST(test_companion_autostart_after_crc_and_cdc_handler);
}
