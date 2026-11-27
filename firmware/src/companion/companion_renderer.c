#include "companion/companion_renderer.h"

#include "companion/companion_assets.h"
#include "oled/oled_display.h"
#include "oled/screensaver/screensaver_utils.h"
#include <stdio.h>
#include <string.h>

#define FACE_CENTER_X 64
#define LEFT_EYE_X 29
#define RIGHT_EYE_X 77
#define EYE_Y 20
#define EYE_W 22
#define EYE_H 22
#define EYE_RADIUS 5
#define MOUTH_Y 49

static uint8_t s_last_contrast = 0xFF;
static bool s_contrast_valid = false;

void companion_renderer_init(void)
{
	s_last_contrast = 0xFF;
	s_contrast_valid = false;
}

static void apply_contrast_if_changed(uint8_t contrast)
{
	if (!s_contrast_valid || s_last_contrast != contrast) {
		oled_set_contrast(contrast);
		s_last_contrast = contrast;
		s_contrast_valid = true;
	}
}

static uint8_t contrast_for_expr(companion_expr_t expr)
{
	switch (expr) {
	case COMP_EXPR_BOOT_HELLO:
	case COMP_EXPR_PET:
	case COMP_EXPR_COFFEE:
	case COMP_EXPR_AGENT_DONE:
	case COMP_EXPR_AGENT_ERR:
	case COMP_EXPR_NEEDS_INPUT:
	case COMP_EXPR_GIT_COMMIT:
	case COMP_EXPR_PING:
		return 0xCF;
	case COMP_EXPR_SPOTIFY:
	case COMP_EXPR_VESKTOP_ON_AIR:
	case COMP_EXPR_VESKTOP_MUTED:
		return 0x9F;
	case COMP_EXPR_VESKTOP_DEAF:
	case COMP_EXPR_VESKTOP_BRB:
		return 0x7F;
	case COMP_EXPR_GAMING:
	case COMP_EXPR_LATE_NIGHT:
		return 0x45;
	case COMP_EXPR_IDLE_DROWSY:
		return 0x30;
	case COMP_EXPR_KINO:
		return 0x28;
	case COMP_EXPR_SLEEP:
		return 0x10;
	case COMP_EXPR_OLED_OFF:
		return 0x00;
	default:
		return 0xAF;
	}
}

static void draw_sprite(int x, int y, int w, int h, const uint16_t *rows)
{
	for (int r = 0; r < h; r++) {
		uint16_t row_bits = rows[r];
		for (int c = 0; c < w; c++) {
			if (row_bits & (1U << (w - 1 - c))) {
				oled_draw_pixel(x + c, y + r, 1);
			}
		}
	}
}

static companion_icon_id_t hud_icon_for_state(const companion_state_t *state,
					      companion_expr_t expr)
{
	switch (expr) {
	case COMP_EXPR_BOOT_HELLO:
	case COMP_EXPR_EXIT_BYE:
		return COMP_ICON_AI;
	case COMP_EXPR_CODING_EMACS:
		return COMP_ICON_EMACS;
	case COMP_EXPR_CODING_TERM:
		return COMP_ICON_TERM;
	case COMP_EXPR_AGENT_THINKING:
		return COMP_ICON_AI;
	case COMP_EXPR_AGENT_DONE:
	case COMP_EXPR_GIT_COMMIT:
		return COMP_ICON_CHECK;
	case COMP_EXPR_AGENT_ERR:
	case COMP_EXPR_NEEDS_INPUT:
		return COMP_ICON_ALERT;
	case COMP_EXPR_PING:
		return COMP_ICON_BELL;
	case COMP_EXPR_VESKTOP_ON_AIR:
		return COMP_ICON_VESKTOP;
	case COMP_EXPR_VESKTOP_MUTED:
	case COMP_EXPR_VESKTOP_DEAF:
		return COMP_ICON_MIC_OFF;
	case COMP_EXPR_VESKTOP_BRB:
		return COMP_ICON_COFFEE;
	case COMP_EXPR_SPOTIFY:
		return COMP_ICON_MUSIC;
	case COMP_EXPR_KINO:
		return COMP_ICON_VIDEO;
	case COMP_EXPR_GAMING:
		return COMP_ICON_GAME;
	case COMP_EXPR_POMO_WORK:
		return COMP_ICON_HOURGLASS;
	case COMP_EXPR_POMO_BREAK:
	case COMP_EXPR_COFFEE:
		return COMP_ICON_COFFEE;
	case COMP_EXPR_LATE_NIGHT:
	case COMP_EXPR_IDLE_DROWSY:
	case COMP_EXPR_SLEEP:
		return COMP_ICON_MOON;
	case COMP_EXPR_PET:
	case COMP_EXPR_NEUTRAL:
	default:
		(void)state;
		return COMP_ICON_HEART;
	}
}

static char submode_badge_char(companion_submode_t submode)
{
	switch (submode) {
	case COMP_SUBMODE_POMODORO:
		return 'P';
	case COMP_SUBMODE_MEDIA:
		return 'M';
	case COMP_SUBMODE_VOICE:
		return 'V';
	case COMP_SUBMODE_SLEEP:
		return 'S';
	case COMP_SUBMODE_HOME:
	default:
		return 'H';
	}
}

static void hud_title_for_state(const companion_state_t *state,
				companion_expr_t expr, char *out,
				size_t out_size)
{
	switch (expr) {
	case COMP_EXPR_BOOT_HELLO:
		snprintf(out, out_size, "TALOS READY!");
		return;
	case COMP_EXPR_EXIT_BYE:
		snprintf(out, out_size, "BACK TO MACROS");
		return;
	case COMP_EXPR_PET:
		snprintf(out, out_size, "HAPPY MOCHI!");
		return;
	case COMP_EXPR_COFFEE:
		snprintf(out, out_size, "ENERGIZED!");
		return;
	case COMP_EXPR_AGENT_DONE:
		snprintf(out, out_size, "AI READY!");
		return;
	case COMP_EXPR_AGENT_ERR:
		snprintf(out, out_size, "CHECK ERROR!");
		return;
	case COMP_EXPR_NEEDS_INPUT:
		snprintf(out, out_size, "NEEDS INPUT!");
		return;
	case COMP_EXPR_GIT_COMMIT:
		snprintf(out, out_size, "COMMITTED!");
		return;
	case COMP_EXPR_PING:
		snprintf(out, out_size, "PING:%s",
			 state->event_label[0] ? state->event_label : "MSG");
		return;
	case COMP_EXPR_IDLE_DROWSY:
		snprintf(out, out_size, "SLEEPY...");
		return;
	case COMP_EXPR_SLEEP:
		snprintf(out, out_size, "SLEEPING zZz");
		return;
	case COMP_EXPR_POMO_WORK:
	case COMP_EXPR_POMO_BREAK: {
		uint16_t m = state->pomo.remaining_s / 60U;
		uint16_t s = state->pomo.remaining_s % 60U;
		const char *tag = !state->pomo.running		? "PAUSE" :
				  (expr == COMP_EXPR_POMO_WORK) ? "WORK" :
								  "BRK";
		if (state->pomo.deep_focus) {
			snprintf(out, out_size, "%s %02u:%02u [D]", tag,
				 (unsigned)m, (unsigned)s);
		} else {
			snprintf(out, out_size, "%s %02u:%02u", tag,
				 (unsigned)m, (unsigned)s);
		}
		return;
	}
	case COMP_EXPR_VESKTOP_ON_AIR:
		snprintf(out, out_size, "VESKTOP ON AIR");
		return;
	case COMP_EXPR_VESKTOP_MUTED:
		snprintf(out, out_size, "MIC MUTED");
		return;
	case COMP_EXPR_VESKTOP_DEAF:
		snprintf(out, out_size, "DEAFENED");
		return;
	case COMP_EXPR_VESKTOP_BRB:
		snprintf(out, out_size, "VESKTOP BRB");
		return;
	case COMP_EXPR_KINO:
		snprintf(out, out_size, "%s",
			 state->top_text[0] ? state->top_text : "CINEMA");
		return;
	case COMP_EXPR_SPOTIFY:
		snprintf(out, out_size, "%s",
			 state->top_text[0] ? state->top_text : "SPOTIFY");
		return;
	case COMP_EXPR_CODING_EMACS:
		snprintf(out, out_size, "%s",
			 state->top_text[0] ? state->top_text : "EMACS");
		return;
	case COMP_EXPR_CODING_TERM:
		snprintf(out, out_size, "%s",
			 state->top_text[0] ? state->top_text : "TERMINAL");
		return;
	case COMP_EXPR_AGENT_THINKING:
		snprintf(out, out_size, "AI THINKING...");
		return;
	case COMP_EXPR_GAMING:
		snprintf(out, out_size, "GAMING");
		return;
	case COMP_EXPR_LATE_NIGHT:
		snprintf(out, out_size, "LATE NIGHT");
		return;
	case COMP_EXPR_NEUTRAL:
	default:
		snprintf(out, out_size, "TALOS MOCHI");
		return;
	}
}

static void draw_progress_bar(int x, int y, int w, int h, uint8_t pct)
{
	if (pct > 100U)
		pct = 100U;
	oled_draw_rect(x, y, w, h, 1);
	if (w > 2 && h > 2 && pct > 0U) {
		int fill_w = ((w - 2) * (int)pct) / 100;
		if (fill_w > 0) {
			draw_filled_rect(x + 1, y + 1, fill_w, h - 2, 1);
		}
	}
}

static void render_yellow_hud(const companion_state_t *state,
			      companion_expr_t expr, uint32_t now_ms)
{
	if (state->exit_holding) {
		oled_draw_string_clipped(2, 0, 2, 50, "EXITING");
		draw_progress_bar(52, 1, 74, 6, state->exit_hold_pct);
		oled_draw_line(14);
		return;
	}

	if (state->vol_overlay_until_ms != 0 &&
	    (int32_t)(state->vol_overlay_until_ms - now_ms) > 0) {
		oled_draw_bitmap(2, 0, 8, 8,
				 companion_hud_icons[COMP_ICON_SPEAKER]);
		if (state->vol_muted) {
			oled_draw_string_clipped(12, 0, 12, 40, "MUTE");
			draw_progress_bar(42, 1, 60, 6, 0);
			oled_draw_string_clipped(105, 0, 105, 127, "0%");
		} else {
			oled_draw_string_clipped(12, 0, 12, 34, "VOL");
			uint8_t pct = (state->vol_pct <= 100U) ?
					      state->vol_pct :
					      100U;
			draw_progress_bar(36, 1, 66, 6, pct);
			char pct_buf[8];
			snprintf(pct_buf, sizeof(pct_buf), "%u%%",
				 (unsigned)pct);
			oled_draw_string_clipped(105, 0, 105, 127, pct_buf);
		}
		oled_draw_line(14);
		return;
	}

	companion_icon_id_t icon = hud_icon_for_state(state, expr);
	oled_draw_bitmap(2, 0, 8, 8, companion_hud_icons[icon]);

	char title[COMP_TEXT_MAX_LEN];
	hud_title_for_state(state, expr, title, sizeof(title));

	// Page 0 text clipped cleanly before right-hand mode badge [X]
	const int text_min_x = 12;
	const int text_max_x = 107;
	int title_px = (int)strlen(title) * 6;
	int box_w = text_max_x - text_min_x + 1;
	int draw_x = text_min_x;

	if (expr == COMP_EXPR_SPOTIFY && title_px > box_w) {
		int overflow = title_px - box_w + 18;
		int offset =
			(int)((now_ms / 110UL) % (uint32_t)(overflow + 12));
		if (offset > 6) {
			int shift = offset - 6;
			if (shift > overflow)
				shift = overflow;
			draw_x -= shift;
		}
	}
	oled_draw_string_clipped(draw_x, 0, text_min_x, text_max_x, title);

	char badge[4];
	snprintf(badge, sizeof(badge), "[%c]",
		 submode_badge_char(state->submode));
	oled_draw_string_clipped(110, 0, 110, 127, badge);

	// Page 1 (y = 8..15)
	if (expr == COMP_EXPR_KINO) {
		return; // Leave Page 1 dark in cinema mode
	}

	if (state->stats_overlay_until_ms != 0 &&
	    (int32_t)(state->stats_overlay_until_ms - now_ms) > 0) {
		oled_draw_string_clipped(2, 8, 2, 14, "H");
		draw_progress_bar(10, 9, 48, 5, state->happiness);
		oled_draw_string_clipped(66, 8, 66, 76, "E");
		draw_progress_bar(74, 9, 48, 5, state->energy);
		return;
	}

	if (state->submode == COMP_SUBMODE_POMODORO &&
	    state->pomo.total_s > 0) {
		uint32_t elapsed = (uint32_t)(state->pomo.total_s -
					      state->pomo.remaining_s);
		uint8_t pct =
			(uint8_t)((elapsed * 100UL) / state->pomo.total_s);
		draw_progress_bar(2, 10, 124, 4, pct);
		return;
	}

	if (expr == COMP_EXPR_SPOTIFY) {
		uint32_t beat = (now_ms / 180UL) % 4U;
		for (int i = 0; i < 6; i++) {
			int dot_x = 16 + i * 16;
			int h = (((i + (int)beat) % 3) == 0) ? 3 : 1;
			draw_filled_rect(dot_x, 13 - h, 4, h, 1);
		}
		return;
	}

	oled_draw_line(14);
}

// ==================== PROCEDURAL FACE PRIMITIVES ====================

static uint8_t compute_blink_level(uint32_t now_ms)
{
	uint32_t phase = now_ms % 4000UL;
	if (phase < 3820UL)
		return 0;
	uint32_t b = phase - 3820UL;
	if (b < 45UL)
		return 50;
	if (b < 120UL)
		return 100;
	if (b < 165UL)
		return 50;
	return 0;
}

static void draw_eye_open(int x, int y, int look_dx, int look_dy,
			  uint8_t blink_pct, bool glint)
{
	if (blink_pct >= 90) {
		draw_filled_rect(x + 1, y + EYE_H / 2 - 1, EYE_W - 2, 2, 1);
		return;
	}

	int h = (blink_pct >= 40) ? 10 : EYE_H;
	int top_y = y + (EYE_H - h) / 2;
	oled_fill_rounded_rect(x, top_y, EYE_W, h, (h < 12) ? 2 : EYE_RADIUS,
			       1);

	if (glint && h >= 14) {
		int gx = x + EYE_W - 7 + look_dx;
		int gy = top_y + 3 + look_dy;
		if (gx < x + 2)
			gx = x + 2;
		if (gx > x + EYE_W - 5)
			gx = x + EYE_W - 5;
		if (gy < top_y + 2)
			gy = top_y + 2;
		if (gy > top_y + h - 5)
			gy = top_y + h - 5;
		draw_filled_rect(gx, gy, 3, 3, 0);
	}
}

static void draw_eye_happy_arc(int x, int y)
{
	for (int t = 0; t < 3; t++) {
		oled_draw_line_pts(x + 2, y + 14 + t, x + 7, y + 7 + t, 1);
		oled_draw_line_pts(x + 7, y + 7 + t, x + 14, y + 7 + t, 1);
		oled_draw_line_pts(x + 14, y + 7 + t, x + EYE_W - 3, y + 14 + t,
				   1);
	}
}

static void draw_eye_chevron(int x, int y, bool point_right)
{
	for (int t = 0; t < 3; t++) {
		if (point_right) {
			oled_draw_line_pts(x + 4, y + 4 + t, x + EYE_W - 5,
					   y + 10 + t, 1);
			oled_draw_line_pts(x + EYE_W - 5, y + 10 + t, x + 4,
					   y + 16 + t, 1);
		} else {
			oled_draw_line_pts(x + EYE_W - 5, y + 4 + t, x + 4,
					   y + 10 + t, 1);
			oled_draw_line_pts(x + 4, y + 10 + t, x + EYE_W - 5,
					   y + 16 + t, 1);
		}
	}
}

static void draw_eye_closed_sleep(int x, int y)
{
	for (int t = 0; t < 2; t++) {
		oled_draw_line_pts(x + 2, y + 10 + t, x + 7, y + 14 + t, 1);
		oled_draw_line_pts(x + 7, y + 14 + t, x + 14, y + 14 + t, 1);
		oled_draw_line_pts(x + 14, y + 14 + t, x + EYE_W - 3,
				   y + 10 + t, 1);
	}
}

static void draw_eye_drowsy(int x, int y)
{
	oled_fill_rounded_rect(x, y + 8, EYE_W, 12, 3, 1);
	draw_filled_rect(x + 1, y + 6, EYE_W - 2, 2, 1);
	draw_filled_rect(x + EYE_W - 7, y + 11, 3, 2, 0);
}

static void draw_mouth_smile(int cx, int y)
{
	draw_filled_rect(cx - 7, y + 5, 14, 2, 1);
	oled_draw_line_pts(cx - 10, y + 2, cx - 7, y + 5, 1);
	oled_draw_line_pts(cx - 10, y + 3, cx - 7, y + 6, 1);
	oled_draw_line_pts(cx + 6, y + 5, cx + 9, y + 2, 1);
	oled_draw_line_pts(cx + 6, y + 6, cx + 9, y + 3, 1);
}

static void draw_mouth_flat(int cx, int y)
{
	draw_filled_rect(cx - 7, y + 3, 14, 2, 1);
}

static void draw_mouth_open_happy(int cx, int y)
{
	oled_fill_rounded_rect(cx - 8, y, 16, 8, 3, 1);
	draw_filled_rect(cx - 8, y, 16, 2, 0);
	draw_filled_rect(cx - 7, y + 1, 14, 1, 1);
}

static void draw_mouth_wavy(int cx, int y)
{
	for (int i = -9; i <= 8; i++) {
		int wy = y + 3 + (((i + 10) / 3) % 2 == 0 ? 0 : 2);
		draw_filled_rect(cx + i, wy, 1, 2, 1);
	}
}

static void draw_mouth_zipper(int cx, int y)
{
	// Strictly centered across [cx - 14 .. cx + 13] around x = 64
	draw_filled_rect(cx - 14, y + 3, 28, 2, 1);
	for (int tx = cx - 11; tx <= cx + 10; tx += 4) {
		draw_filled_rect(tx, y + 1, 2, 6, 1);
	}
	// Zipper pull tab hanging down on the right side
	oled_draw_rect(cx + 12, y + 5, 4, 5, 1);
}

static void draw_mouth_cat(int cx, int y)
{
	oled_draw_line_pts(cx - 8, y + 2, cx - 4, y + 6, 1);
	oled_draw_line_pts(cx - 4, y + 6, cx, y + 2, 1);
	oled_draw_line_pts(cx, y + 2, cx + 4, y + 6, 1);
	oled_draw_line_pts(cx + 4, y + 6, cx + 8, y + 2, 1);
}

static void draw_mouth_yawn(int cx, int y)
{
	oled_draw_rounded_rect(cx - 5, y, 10, 9, 4, 1);
	oled_draw_rounded_rect(cx - 4, y + 1, 8, 7, 3, 1);
}

static void draw_mouth_sleep_breath(int cx, int y, uint32_t now_ms)
{
	bool wide = ((now_ms / 900UL) % 2U) == 0U;
	if (wide) {
		oled_draw_rounded_rect(cx - 3, y + 1, 6, 5, 2, 1);
	} else {
		draw_filled_rect(cx - 2, y + 2, 4, 3, 1);
	}
}

static void draw_blush_marks(int dx, int dy)
{
	for (int i = 0; i < 3; i++) {
		int lx = 21 + i * 3 + dx;
		int rx = 100 + i * 3 + dx;
		oled_draw_line_pts(lx, 44 + dy, lx - 1, 47 + dy, 1);
		oled_draw_line_pts(rx, 44 + dy, rx + 1, 47 + dy, 1);
	}
}

static void draw_eye_focused_slant(int x, int y, bool left_eye)
{
	oled_fill_rounded_rect(x, y + 2, EYE_W, EYE_H - 2, 4, 1);
	// Subtle angled top-inner bevel for determined focus
	for (int row = 0; row < 5; row++) {
		int cut = (5 - row) * 2;
		if (left_eye) {
			draw_filled_rect(x + EYE_W - cut, y + 2 + row, cut, 1,
					 0);
		} else {
			draw_filled_rect(x, y + 2 + row, cut, 1, 0);
		}
	}
	draw_filled_rect(x + EYE_W - 7, y + 8, 3, 3, 0);
}

static void draw_coffee_with_steam(int x, int y, uint32_t now_ms)
{
	int steam = (int)((now_ms / 240UL) % 2U);
	draw_sprite(x, y, PROP_COFFEE_MUG_W, PROP_COFFEE_MUG_H,
		    PROP_COFFEE_MUG);
	oled_draw_line_pts(x + 3, y - 2 - steam, x + 4, y - 5 - steam, 1);
	oled_draw_line_pts(x + 7, y - 3 + steam, x + 8, y - 6 + steam, 1);
}

static void draw_headset_shell(int dx, int dy, bool show_mic, bool mic_muted,
			       bool deaf, uint32_t now_ms)
{
	oled_draw_line_pts(12 + dx, 22 + dy, 20 + dx, 17 + dy, 1);
	oled_draw_line_pts(20 + dx, 17 + dy, 107 + dx, 17 + dy, 1);
	oled_draw_line_pts(107 + dx, 17 + dy, 115 + dx, 22 + dy, 1);

	oled_draw_rounded_rect(4 + dx, 22 + dy, 13, 21, 3, 1);
	oled_draw_rounded_rect(110 + dx, 22 + dy, 13, 21, 3, 1);
	draw_filled_rect(15 + dx, 25 + dy, 2, 15, 1);
	draw_filled_rect(110 + dx, 25 + dy, 2, 15, 1);

	if (deaf) {
		oled_draw_line_pts(7 + dx, 29 + dy, 13 + dx, 36 + dy, 1);
		oled_draw_line_pts(13 + dx, 29 + dy, 7 + dx, 36 + dy, 1);
		oled_draw_line_pts(113 + dx, 29 + dy, 119 + dx, 36 + dy, 1);
		oled_draw_line_pts(119 + dx, 29 + dy, 113 + dx, 36 + dy, 1);
	}

	if (!show_mic)
		return;

	oled_draw_line_pts(10 + dx, 43 + dy, 15 + dx, 50 + dy, 1);
	oled_draw_line_pts(15 + dx, 50 + dy, 22 + dx, 50 + dy, 1);

	if (mic_muted) {
		oled_draw_line_pts(24 + dx, 48 + dy, 28 + dx, 52 + dy, 1);
		oled_draw_line_pts(28 + dx, 48 + dy, 24 + dx, 52 + dy, 1);
	} else {
		draw_filled_circle(24 + dx, 50 + dy, 2, 1);
		uint32_t wave = (now_ms / 220UL) % 3U;
		oled_draw_line_pts(29 + dx, 48 + dy, 29 + dx, 52 + dy, 1);
		if (wave >= 1U) {
			oled_draw_line_pts(32 + dx, 47 + dy, 32 + dx, 53 + dy,
					   1);
		}
		if (wave >= 2U) {
			oled_draw_line_pts(35 + dx, 46 + dy, 35 + dx, 54 + dy,
					   1);
		}
	}
}

static void render_coding_face(companion_expr_t expr,
			       const companion_state_t *state, uint32_t now_ms,
			       int dx, int dy, uint8_t blink)
{
	int lx = LEFT_EYE_X + dx;
	int rx = RIGHT_EYE_X + dx;
	int ey = EYE_Y + dy;
	int cx = FACE_CENTER_X + dx;
	int my = MOUTH_Y + dy;

	if (expr == COMP_EXPR_CODING_EMACS) {
		int look_dx = 0;
		if (state->last_typing_pulse_ms != 0 &&
		    (uint32_t)(now_ms - state->last_typing_pulse_ms) < 2500UL) {
			static const int8_t track[4] = { -2, 0, 2, 0 };
			look_dx = track[(now_ms / 250UL) % 4U];
		}

		// Glasses frames symmetric around x = 64: [25..54] and [73..102], bridge [55..72]
		oled_draw_rounded_rect(25 + dx, 19 + dy, 30, 22, 3, 1);
		oled_draw_rounded_rect(73 + dx, 19 + dy, 30, 22, 3, 1);
		draw_filled_rect(55 + dx, 29 + dy, 18, 2, 1);
		oled_draw_line_pts(22 + dx, 27 + dy, 25 + dx, 27 + dy, 1);
		oled_draw_line_pts(103 + dx, 27 + dy, 106 + dx, 27 + dy, 1);

		// Eyes centered inside frames
		int eye_h = (blink >= 90) ? 2 : (blink >= 40 ? 8 : 16);
		int eye_y = 22 + (16 - eye_h) / 2 + dy;
		if (eye_h <= 2) {
			draw_filled_rect(31 + dx, 29 + dy, 18, 2, 1);
			draw_filled_rect(79 + dx, 29 + dy, 18, 2, 1);
		} else {
			oled_fill_rounded_rect(31 + dx, eye_y, 18, eye_h, 3, 1);
			oled_fill_rounded_rect(79 + dx, eye_y, 18, eye_h, 3, 1);
			if (eye_h >= 12) {
				draw_filled_rect(42 + look_dx + dx, eye_y + 3,
						 3, 3, 0);
				draw_filled_rect(90 + look_dx + dx, eye_y + 3,
						 3, 3, 0);
			}
		}
		draw_mouth_smile(cx, my);
		return;
	}

	if (expr == COMP_EXPR_CODING_TERM) {
		draw_eye_open(lx, ey, 0, 0, blink, true);
		draw_eye_open(rx, ey, 0, 0, blink, true);
		if (blink < 40) {
			// Tiny ">_" terminal glint inside left/right eyes
			oled_draw_line_pts(lx + 4, ey + 11, lx + 7, ey + 14, 0);
			oled_draw_line_pts(lx + 7, ey + 14, lx + 4, ey + 17, 0);
			draw_filled_rect(rx + 4, ey + 16, 6, 2, 0);
		}
		draw_mouth_flat(cx, my);
		return;
	}

	if (expr == COMP_EXPR_GAMING) {
		draw_filled_rect(25 + dx, 18 + dy, 78, 2, 1);
		draw_eye_focused_slant(lx, ey, true);
		draw_eye_focused_slant(rx, ey, false);
		draw_mouth_flat(cx, my);
		return;
	}

	// COMP_EXPR_LATE_NIGHT (01:00-04:00)
	bool yawning = (now_ms % 25000UL) < 1600UL;
	draw_eye_drowsy(lx, ey);
	draw_eye_drowsy(rx, ey);
	oled_draw_line_pts(lx + 3, ey + 22, lx + EYE_W - 4, ey + 22, 1);
	oled_draw_line_pts(rx + 3, ey + 22, rx + EYE_W - 4, ey + 22, 1);
	draw_coffee_with_steam(7 + dx, 46 + dy, now_ms);
	if (yawning) {
		draw_mouth_yawn(cx, my - 1);
	} else {
		draw_mouth_flat(cx, my);
	}
}

static void render_agent_git_ping(companion_expr_t expr, uint32_t now_ms,
				  int dx, int dy, uint8_t blink)
{
	int lx = LEFT_EYE_X + dx;
	int rx = RIGHT_EYE_X + dx;
	int ey = EYE_Y + dy;
	int cx = FACE_CENTER_X + dx;
	int my = MOUTH_Y + dy;

	switch (expr) {
	case COMP_EXPR_AGENT_THINKING: {
		oled_draw_rounded_rect(lx, ey, EYE_W, EYE_H, EYE_RADIUS, 1);
		oled_draw_rounded_rect(rx, ey, EYE_W, EYE_H, EYE_RADIUS, 1);
		int scan_step = (int)((now_ms / 45UL) % 24U);
		int scan_x = (scan_step <= 12) ? scan_step : (24 - scan_step);
		draw_filled_rect(lx + 2 + scan_x, ey + 3, 5, EYE_H - 6, 1);
		draw_filled_rect(rx + 2 + scan_x, ey + 3, 5, EYE_H - 6, 1);
		oled_draw_rounded_rect(cx - 3, my + 1, 6, 6, 2, 1);
		break;
	}

	case COMP_EXPR_AGENT_DONE: {
		bool spark = ((now_ms / 200UL) % 2U) == 0U;
		draw_eye_happy_arc(lx, ey);
		draw_eye_happy_arc(rx, ey);
		draw_mouth_open_happy(cx, my - 1);
		draw_sprite(10 + dx, (spark ? 22 : 25) + dy, PROP_SPARKLE_W,
			    PROP_SPARKLE_H, PROP_SPARKLE);
		draw_sprite(110 + dx, (spark ? 25 : 22) + dy, PROP_SPARKLE_W,
			    PROP_SPARKLE_H, PROP_SPARKLE);
		break;
	}

	case COMP_EXPR_AGENT_ERR:
	case COMP_EXPR_NEEDS_INPUT: {
		int shake = (((now_ms / 90UL) % 2U) == 0U) ? 1 : -1;
		oled_draw_rounded_rect(lx + shake, ey, EYE_W, EYE_H, EYE_RADIUS,
				       1);
		oled_draw_rounded_rect(lx + 1 + shake, ey + 1, EYE_W - 2,
				       EYE_H - 2, EYE_RADIUS - 1, 1);
		draw_filled_rect(lx + 9 + shake, ey + 9, 4, 4, 1);
		draw_eye_chevron(rx + shake, ey, false);
		draw_mouth_wavy(cx + shake, my);
		if (((now_ms / 240UL) % 2U) == 0U) {
			draw_filled_rect(112 + dx, 21 + dy, 3, 12, 1);
			draw_filled_rect(112 + dx, 36 + dy, 3, 3, 1);
		}
		break;
	}

	case COMP_EXPR_GIT_COMMIT: {
		oled_fill_rounded_rect(26 + dx, 20 + dy, 28, 17, 3, 1);
		oled_fill_rounded_rect(74 + dx, 20 + dy, 28, 17, 3, 1);
		draw_filled_rect(54 + dx, 22 + dy, 20, 4, 1);
		draw_filled_rect(20 + dx, 22 + dy, 6, 3, 1);
		draw_filled_rect(102 + dx, 22 + dy, 6, 3, 1);
		int glint_pos = (int)((now_ms / 50UL) % 24U);
		oled_draw_line_pts(29 + glint_pos / 2 + dx, 34 + dy,
				   35 + glint_pos / 2 + dx, 22 + dy, 0);
		oled_draw_line_pts(77 + glint_pos / 2 + dx, 34 + dy,
				   83 + glint_pos / 2 + dx, 22 + dy, 0);
		draw_mouth_smile(cx, my);
		break;
	}

	case COMP_EXPR_PING:
	default: {
		int swing = (((now_ms / 140UL) % 2U) == 0U) ? 1 : -1;
		draw_eye_open(lx, ey - 1, 2, -1, blink, true);
		draw_eye_open(rx, ey - 1, 2, -1, blink, true);
		oled_draw_rounded_rect(cx - 4, my, 8, 7, 3, 1);
		draw_sprite(108 + swing + dx, 20 + dy, PROP_BELL_W, PROP_BELL_H,
			    PROP_BELL);
		break;
	}
	}
}

static void render_media_voice_pomo(companion_expr_t expr,
				    const companion_state_t *state,
				    uint32_t now_ms, int dx, int dy,
				    uint8_t blink)
{
	int lx = LEFT_EYE_X + dx;
	int rx = RIGHT_EYE_X + dx;
	int ey = EYE_Y + dy;
	int cx = FACE_CENTER_X + dx;
	int my = MOUTH_Y + dy;

	switch (expr) {
	case COMP_EXPR_VESKTOP_ON_AIR:
		draw_headset_shell(dx, dy, true, false, false, now_ms);
		draw_eye_open(lx, ey + 1, 0, 0, blink, true);
		draw_eye_open(rx, ey + 1, 0, 0, blink, true);
		draw_mouth_smile(cx, my);
		break;

	case COMP_EXPR_VESKTOP_MUTED:
		draw_headset_shell(dx, dy, true, true, false, now_ms);
		draw_eye_drowsy(lx, ey + 1);
		draw_eye_drowsy(rx, ey + 1);
		draw_mouth_zipper(cx, my);
		break;

	case COMP_EXPR_VESKTOP_DEAF:
	case COMP_EXPR_VESKTOP_BRB:
		draw_headset_shell(dx, dy, true, true, true, now_ms);
		draw_eye_closed_sleep(lx, ey + 2);
		draw_eye_closed_sleep(rx, ey + 2);
		draw_mouth_zipper(cx, my);
		if (expr == COMP_EXPR_VESKTOP_BRB) {
			draw_coffee_with_steam(108 + dx, 49 + dy, now_ms);
		}
		break;

	case COMP_EXPR_SPOTIFY: {
		static const int8_t groove_x[4] = { -1, 0, 1, 0 };
		static const int8_t groove_y[4] = { 0, -1, 0, 1 };
		uint32_t step = (now_ms / 260UL) % 4U;
		int gx = dx + groove_x[step];
		int gy = dy + groove_y[step];
		bool wink = ((now_ms / 520UL) % 6U) == 5U;

		// Plump headphones
		oled_draw_line_pts(13 + gx, 23 + gy, 23 + gx, 17 + gy, 1);
		oled_draw_line_pts(23 + gx, 17 + gy, 104 + gx, 17 + gy, 1);
		oled_draw_line_pts(104 + gx, 17 + gy, 114 + gx, 23 + gy, 1);
		oled_fill_rounded_rect(4 + gx, 24 + gy, 12, 20, 4, 1);
		oled_fill_rounded_rect(111 + gx, 24 + gy, 12, 20, 4, 1);
		draw_filled_rect(14 + gx, 26 + gy, 2, 16, 0);
		draw_filled_rect(111 + gx, 26 + gy, 2, 16, 0);

		draw_eye_happy_arc(LEFT_EYE_X + gx, EYE_Y + gy + 1);
		if (wink) {
			draw_filled_rect(RIGHT_EYE_X + gx + 2, EYE_Y + gy + 11,
					 EYE_W - 4, 3, 1);
		} else {
			draw_eye_happy_arc(RIGHT_EYE_X + gx, EYE_Y + gy + 1);
		}
		draw_mouth_smile(FACE_CENTER_X + gx, MOUTH_Y + gy);

		// Mini corner equalizers + floating note
		for (int b = 0; b < 3; b++) {
			int bar_h = 2 + (int)(((now_ms / 130UL) +
					       (uint32_t)b * 2U) %
					      5U);
			draw_filled_rect(5 + b * 4, 60 - bar_h, 2, bar_h, 1);
			draw_filled_rect(112 + b * 4, 60 - bar_h, 2, bar_h, 1);
		}
		draw_sprite(108 + dx, 18 + (int)((now_ms / 260UL) % 2U) + dy,
			    PROP_NOTE_W, PROP_NOTE_H, PROP_NOTE);
		break;
	}

	case COMP_EXPR_KINO: {
		bool paused = (state->host_flags & COMP_FLAG_VIDEO_PAUSED) != 0;
		draw_eye_open(lx, ey, 0, -1, blink, true);
		draw_eye_open(rx, ey, 0, -1, blink, true);
		if (blink < 40) {
			draw_filled_rect(lx + 4, ey + 15, 2, 2, 0);
			draw_filled_rect(rx + 4, ey + 15, 2, 2, 0);
		}
		draw_mouth_smile(cx, my);

		// Striped popcorn bucket + jumping kernels
		int bx = 109 + dx;
		int by = 48 + dy;
		draw_sprite(bx, by, PROP_POPCORN_BUCKET_W,
			    PROP_POPCORN_BUCKET_H, PROP_POPCORN_BUCKET);
		if (!paused) {
			int pop = (int)((now_ms / 300UL) % 3U);
			draw_filled_rect(bx + 2, by - 3 - (pop == 0 ? 2 : 0), 2,
					 2, 1);
			draw_filled_rect(bx + 6, by - 4 - (pop == 1 ? 2 : 0), 2,
					 2, 1);
			draw_filled_rect(bx + 9, by - 2 - (pop == 2 ? 2 : 0), 2,
					 2, 1);
		}
		break;
	}

	case COMP_EXPR_POMO_WORK: {
		int ribbon_wave = (int)((now_ms / 240UL) % 2U);
		draw_filled_rect(22 + dx, 17 + dy, 84, 3, 1);
		draw_filled_rect(106 + dx, 16 + dy, 4, 4, 1);
		oled_draw_line_pts(110 + dx, 17 + dy, 120 + dx,
				   19 + ribbon_wave + dy, 1);
		oled_draw_line_pts(110 + dx, 19 + dy, 118 + dx,
				   23 - ribbon_wave + dy, 1);

		draw_eye_focused_slant(lx, ey + 2, true);
		draw_eye_focused_slant(rx, ey + 2, false);
		draw_mouth_flat(cx, my);

		// Live hourglass in right slot with falling sand pixel
		int hx = 109 + dx;
		int hy = 40 + dy;
		draw_sprite(hx, hy, PROP_HOURGLASS_FRAME_W,
			    PROP_HOURGLASS_FRAME_H, PROP_HOURGLASS_FRAME);
		uint8_t top_rows =
			(state->pomo.total_s > 0) ?
				(uint8_t)((state->pomo.remaining_s * 3UL) /
					  state->pomo.total_s) :
				0;
		for (uint8_t r = 0; r < top_rows; r++) {
			draw_filled_rect(hx + 3, hy + 4 - r, 5, 1, 1);
		}
		for (uint8_t r = 0; r < (uint8_t)(3U - top_rows); r++) {
			draw_filled_rect(hx + 3, hy + 10 - r, 5, 1, 1);
		}
		if (state->pomo.running) {
			int sand_y = hy + 6 + (int)((now_ms / 250UL) % 4U);
			oled_draw_pixel(hx + 5, sand_y, 1);
		}
		break;
	}

	case COMP_EXPR_POMO_BREAK:
	default:
		draw_eye_happy_arc(lx, ey);
		draw_eye_happy_arc(rx, ey);
		draw_mouth_smile(cx, my);
		draw_coffee_with_steam(108 + dx, 48 + dy, now_ms);
		break;
	}
}

static void render_base_face(companion_expr_t expr,
			     const companion_state_t *state, uint32_t now_ms,
			     int dx, int dy)
{
	int lx = LEFT_EYE_X + dx;
	int rx = RIGHT_EYE_X + dx;
	int ey = EYE_Y + dy;
	int cx = FACE_CENTER_X + dx;
	int my = MOUTH_Y + dy;
	uint8_t blink = compute_blink_level(now_ms);

	switch (expr) {
	case COMP_EXPR_BOOT_HELLO:
		draw_eye_happy_arc(lx, ey);
		draw_eye_happy_arc(rx, ey);
		draw_mouth_smile(cx, my);
		draw_sprite(12 + dx, 24 + dy, PROP_SPARKLE_W, PROP_SPARKLE_H,
			    PROP_SPARKLE);
		draw_sprite(108 + dx, 24 + dy, PROP_SPARKLE_W, PROP_SPARKLE_H,
			    PROP_SPARKLE);
		break;

	case COMP_EXPR_EXIT_BYE:
		draw_eye_closed_sleep(lx, ey);
		draw_eye_closed_sleep(rx, ey);
		draw_mouth_smile(cx, my);
		break;

	case COMP_EXPR_PET: {
		int float_up = (int)((now_ms / 180UL) % 4U);
		draw_eye_chevron(lx, ey, true);
		draw_eye_chevron(rx, ey, false);
		draw_blush_marks(dx, dy);
		draw_mouth_cat(cx, my);
		draw_sprite(12 + dx, 24 - float_up + dy, PROP_HEART_W,
			    PROP_HEART_H, PROP_HEART);
		draw_sprite(109 + dx, 22 - ((float_up + 2) % 4) + dy,
			    PROP_HEART_W, PROP_HEART_H, PROP_HEART);
		break;
	}

	case COMP_EXPR_COFFEE: {
		int steam_shift = (int)((now_ms / 240UL) % 2U);
		draw_eye_open(lx, ey, -2, 0, blink, true);
		draw_eye_open(rx, ey, -2, 0, blink, true);
		draw_mouth_smile(cx, my);
		draw_sprite(8 + dx, 36 + dy, PROP_COFFEE_MUG_W,
			    PROP_COFFEE_MUG_H, PROP_COFFEE_MUG);
		oled_draw_line_pts(11 + dx, 34 - steam_shift + dy, 12 + dx,
				   31 - steam_shift + dy, 1);
		oled_draw_line_pts(15 + dx, 33 + steam_shift + dy, 16 + dx,
				   30 + steam_shift + dy, 1);
		break;
	}

	case COMP_EXPR_IDLE_DROWSY:
		draw_eye_drowsy(lx, ey);
		draw_eye_drowsy(rx, ey);
		draw_mouth_yawn(cx, my - 1);
		break;

	case COMP_EXPR_SLEEP: {
		uint32_t step = (now_ms / 500UL) % 4U;
		draw_eye_closed_sleep(lx, ey + 2);
		draw_eye_closed_sleep(rx, ey + 2);
		draw_mouth_sleep_breath(cx, my, now_ms);
		if (step >= 1U) {
			draw_sprite(105 + dx, 33 + dy, PROP_SLEEP_Z_SMALL_W,
				    PROP_SLEEP_Z_SMALL_H, PROP_SLEEP_Z_SMALL);
		}
		if (step >= 2U) {
			draw_sprite(112 + dx, 24 + dy, PROP_SLEEP_Z_BIG_W,
				    PROP_SLEEP_Z_BIG_H, PROP_SLEEP_Z_BIG);
		}
		break;
	}

	case COMP_EXPR_NEUTRAL:
		draw_eye_open(lx, ey, 0, 0, blink, true);
		draw_eye_open(rx, ey, 0, 0, blink, true);
		draw_mouth_smile(cx, my);
		break;

	case COMP_EXPR_CODING_EMACS:
	case COMP_EXPR_CODING_TERM:
	case COMP_EXPR_GAMING:
	case COMP_EXPR_LATE_NIGHT:
		render_coding_face(expr, state, now_ms, dx, dy, blink);
		break;

	case COMP_EXPR_AGENT_THINKING:
	case COMP_EXPR_AGENT_DONE:
	case COMP_EXPR_AGENT_ERR:
	case COMP_EXPR_NEEDS_INPUT:
	case COMP_EXPR_GIT_COMMIT:
	case COMP_EXPR_PING:
		render_agent_git_ping(expr, now_ms, dx, dy, blink);
		break;

	case COMP_EXPR_VESKTOP_ON_AIR:
	case COMP_EXPR_VESKTOP_MUTED:
	case COMP_EXPR_VESKTOP_DEAF:
	case COMP_EXPR_VESKTOP_BRB:
	case COMP_EXPR_SPOTIFY:
	case COMP_EXPR_KINO:
	case COMP_EXPR_POMO_WORK:
	case COMP_EXPR_POMO_BREAK:
		render_media_voice_pomo(expr, state, now_ms, dx, dy, blink);
		break;

	default:
		draw_eye_open(lx, ey, 0, 0, blink, true);
		draw_eye_open(rx, ey, 0, 0, blink, true);
		draw_mouth_smile(cx, my);
		break;
	}
}

void companion_renderer_render(const companion_state_t *state, uint32_t now_ms)
{
	if (!state)
		return;

	companion_expr_t expr = companion_state_resolve_expr(state);
	if (expr == COMP_EXPR_OLED_OFF) {
		if (oled_is_active()) {
			oled_clear();
			oled_update();
			oled_set_power(false);
		}
		return;
	}

	if (!oled_is_active()) {
		oled_set_power(true);
		s_contrast_valid = false;
	}

	apply_contrast_if_changed(contrast_for_expr(expr));

	// Subtle pixel shift every 30s to prevent OLED burn-in on static desk scenes
	static const int8_t shift_x[4] = { 0, 1, 0, -1 };
	static const int8_t shift_y[4] = { 0, 0, 1, 0 };
	uint32_t shift_idx = (now_ms / 30000UL) % 4U;
	int dx = shift_x[shift_idx];
	int dy = shift_y[shift_idx];

	oled_clear();
	render_yellow_hud(state, expr, now_ms);
	render_base_face(expr, state, now_ms, dx, dy);
	oled_update();
}
