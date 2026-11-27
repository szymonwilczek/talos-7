#include "unity/unity.h"

#include "companion/companion_renderer.h"
#include "companion/companion_state.h"
#include "mocks/mock_companion_hw.h"
#include "mocks/mock_oled.h"
#include "oled/oled_display.h"

static void init_oled_and_renderer(companion_state_t *st)
{
	mock_companion_hw_reset();
	oled_init();
	companion_renderer_init();
	companion_state_init(st, 0);
	companion_state_tick(st,
			     2000); // Expire BOOT_HELLO; pixel-shift dx=0,dy=0
}

void test_oled_off_clears_buffer_and_powers_down(void)
{
	companion_state_t st;
	init_oled_and_renderer(&st);

	companion_renderer_render(&st, 2000);
	TEST_ASSERT_TRUE(mock_oled_power_on);
	TEST_ASSERT_TRUE(mock_oled_count_lit(0, 0, 127, 63) > 0);

	companion_state_sleep_now(&st, 2100);
	companion_state_tick(&st, 2100 + COMP_SLEEP_BEFORE_OFF_MS);
	TEST_ASSERT_EQUAL(COMP_EXPR_OLED_OFF,
			  companion_state_resolve_expr(&st));

	companion_renderer_render(&st, 2100 + COMP_SLEEP_BEFORE_OFF_MS);
	TEST_ASSERT_FALSE(mock_oled_power_on);
	TEST_ASSERT_FALSE(oled_is_active());
	TEST_ASSERT_EQUAL(0, mock_oled_count_lit(0, 0, 127, 63));
}

void test_cinema_mode_keeps_page1_blank_and_dims_contrast(void)
{
	companion_state_t st;
	init_oled_and_renderer(&st);

	companion_state_set_host_context(&st, COMP_CTX_KINO,
					 COMP_FLAG_MEDIA_ACTIVE, "NETFLIX", "",
					 2000);
	companion_renderer_render(&st, 2000);

	TEST_ASSERT_EQUAL_UINT8(0x28, mock_oled_last_contrast);
	TEST_ASSERT_TRUE(mock_oled_count_lit(0, 0, 127, 7) > 0);
	TEST_ASSERT_EQUAL(0, mock_oled_count_lit(0, 8, 127, 15));
	TEST_ASSERT_TRUE(mock_oled_count_lit(0, 16, 127, 63) > 0);
}

void test_all_expressions_respect_yellow_blue_seam_y15(void)
{
	companion_state_t st;
	init_oled_and_renderer(&st);

	const companion_host_ctx_t ctxs[] = {
		COMP_CTX_IDLE,		 COMP_CTX_EMACS,   COMP_CTX_TERM,
		COMP_CTX_AGENT_THINKING, COMP_CTX_VESKTOP, COMP_CTX_SPOTIFY,
		COMP_CTX_KINO,		 COMP_CTX_GAMING
	};

	for (size_t i = 0; i < sizeof(ctxs) / sizeof(ctxs[0]); i++) {
		companion_state_set_host_context(&st, ctxs[i], 0, "CTX", "",
						 2000);
		companion_renderer_render(&st, 2000);
		TEST_ASSERT_EQUAL(0, mock_oled_count_lit(0, 15, 127, 15));
	}

	companion_pomo_start(&st, 2000);
	companion_renderer_render(&st, 2000);
	TEST_ASSERT_EQUAL(0, mock_oled_count_lit(0, 15, 127, 15));
}

void test_emacs_mouth_and_glasses_centered_at_x64(void)
{
	companion_state_t st;
	init_oled_and_renderer(&st);

	companion_state_set_host_context(&st, COMP_CTX_EMACS, 0, "EMACS",
					 "elisp", 2000);
	companion_renderer_render(&st, 2000);

	// Mouth bounding box around center x=64
	int min_mx = 127, max_mx = 0;
	for (int y = 48; y <= 58; y++) {
		for (int x = 40; x <= 87; x++) {
			if (mock_oled_get_pixel(x, y)) {
				if (x < min_mx)
					min_mx = x;
				if (x > max_mx)
					max_mx = x;
			}
		}
	}
	TEST_ASSERT_TRUE(min_mx < max_mx);
	TEST_ASSERT_EQUAL(127, min_mx + max_mx);

	// Glasses outer frame symmetry
	int min_gx = 127, max_gx = 0;
	for (int y = 19; y <= 40; y++) {
		for (int x = 23; x <= 104; x++) {
			if (mock_oled_get_pixel(x, y)) {
				if (x < min_gx)
					min_gx = x;
				if (x > max_gx)
					max_gx = x;
			}
		}
	}
	TEST_ASSERT_EQUAL(127, min_gx + max_gx);
}

void test_vesktop_muted_zipper_mouth_centered_at_x64(void)
{
	companion_state_t st;
	init_oled_and_renderer(&st);

	st.submode = COMP_SUBMODE_VOICE;
	st.local_mic_muted = true;
	companion_renderer_render(&st, 2000);

	// Zipper horizontal bar row (y = 52 = MOUTH_Y + 3) spans [50..77] -> 50+77=127
	int min_zx = 127, max_zx = 0;
	for (int x = 40; x <= 88; x++) {
		if (mock_oled_get_pixel(x, 52)) {
			if (x < min_zx)
				min_zx = x;
			if (x > max_zx)
				max_zx = x;
		}
	}
	TEST_ASSERT_EQUAL(50, min_zx);
	TEST_ASSERT_EQUAL(77, max_zx);
	TEST_ASSERT_EQUAL(127, min_zx + max_zx);

	// Deafen and BRB both share centered zipper mouth (BRB also draws coffee cup)
	st.local_mic_muted = false;
	st.local_deafened = true;
	companion_renderer_render(&st, 2100);
	TEST_ASSERT_TRUE(mock_oled_count_lit(50, 52, 77, 52) > 20);
	TEST_ASSERT_EQUAL(0, mock_oled_count_lit(108, 49, 118, 59));

	st.brb_active = true;
	companion_renderer_render(&st, 2200);
	TEST_ASSERT_TRUE(mock_oled_count_lit(50, 52, 77, 52) > 20);
	TEST_ASSERT_TRUE(mock_oled_count_lit(108, 49, 118, 59) > 10);
}

void test_spotify_marquee_never_overlaps_mode_badge(void)
{
	companion_state_t st;
	init_oled_and_renderer(&st);

	st.submode = COMP_SUBMODE_MEDIA;
	companion_state_set_host_context(
		&st, COMP_CTX_SPOTIFY, COMP_FLAG_MEDIA_ACTIVE,
		"Very Long Artist Name - Extremely Long Song Title", "", 2000);
	companion_renderer_render(&st, 2000);

	// 2px clear gutter between text clip max_x (107) and badge [M] (110)
	TEST_ASSERT_EQUAL(0, mock_oled_count_lit(108, 0, 109, 7));
	TEST_ASSERT_TRUE(mock_oled_count_lit(110, 0, 127, 7) > 0);
}

void test_contrast_scales_across_states(void)
{
	companion_state_t st;
	init_oled_and_renderer(&st);

	companion_renderer_render(&st, 2000);
	TEST_ASSERT_EQUAL_UINT8(0xAF, mock_oled_last_contrast);

	companion_state_pet(&st, 2100);
	companion_renderer_render(&st, 2100);
	TEST_ASSERT_EQUAL_UINT8(0xCF, mock_oled_last_contrast);

	companion_state_sleep_now(&st, 2200);
	companion_renderer_render(&st, 2200);
	TEST_ASSERT_EQUAL_UINT8(0x10, mock_oled_last_contrast);
}

void run_companion_renderer_tests(void)
{
	printf("\n=== Companion Renderer & OLED Boundary Tests ===\n");
	RUN_TEST(test_oled_off_clears_buffer_and_powers_down);
	RUN_TEST(test_cinema_mode_keeps_page1_blank_and_dims_contrast);
	RUN_TEST(test_all_expressions_respect_yellow_blue_seam_y15);
	RUN_TEST(test_emacs_mouth_and_glasses_centered_at_x64);
	RUN_TEST(test_vesktop_muted_zipper_mouth_centered_at_x64);
	RUN_TEST(test_spotify_marquee_never_overlaps_mode_badge);
	RUN_TEST(test_contrast_scales_across_states);
}
