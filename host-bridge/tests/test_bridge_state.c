#include "bridge_idle.h"
#include "bridge_state.h"
#include "unity.h"
#include <string.h>

static void test_priority_agent_overrides_all(void)
{
	host_snapshot_t snap;
	memset(&snap, 0, sizeof(snap));
	strcpy(snap.focused_app, "firefox");
	strcpy(snap.focused_title, "YouTube - Music");
	snap.firefox_mpris_playing = true;
	snap.spotify_playing = true;
	snap.vesktop_voice_active = true;
	snap.agent_thinking = true;
	strcpy(snap.agent_task, "Refactor Bridge");

	bridge_resolved_state_t out;
	bridge_resolve_state(&snap, 14, 1000ULL, &out);

	TEST_ASSERT_EQUAL(BRIDGE_CTX_AGENT_THINKING, out.ctx);
	TEST_ASSERT_EQUAL_STRING("Refactor Bridge", out.top_text);
	TEST_ASSERT_EQUAL_STRING("Thinking...", out.sub_text);
	TEST_ASSERT_TRUE((out.flags & BRIDGE_FLAG_MEDIA_ACTIVE) != 0);
	TEST_ASSERT_TRUE((out.flags & BRIDGE_FLAG_VOICE_ACTIVE) != 0);
}

static void test_firefox_docs_not_kino_unless_video_or_mpris(void)
{
	host_snapshot_t snap;
	memset(&snap, 0, sizeof(snap));
	strcpy(snap.focused_app, "firefox");
	strcpy(snap.focused_title, "ArchWiki Sway - Mozilla Firefox");
	snap.firefox_mpris_playing = false;

	bridge_resolved_state_t out;
	bridge_resolve_state(&snap, 15, 2000ULL, &out);

	TEST_ASSERT_EQUAL(BRIDGE_CTX_IDLE, out.ctx);
	TEST_ASSERT_FALSE(bridge_is_media_or_voice_busy(&snap));
}

static void test_firefox_youtube_playing_vs_paused_kino(void)
{
	host_snapshot_t snap;
	memset(&snap, 0, sizeof(snap));
	strcpy(snap.focused_app, "firefox");
	strcpy(snap.focused_title, "YouTube - C11 Deep Dive");
	snap.firefox_mpris_playing = false;

	/* Paused YouTube must NOT force Kino or hijack active coding/idle */
	bridge_resolved_state_t out;
	bridge_resolve_state(&snap, 20, 3000ULL, &out);
	TEST_ASSERT_EQUAL(BRIDGE_CTX_IDLE, out.ctx);
	TEST_ASSERT_TRUE(bridge_is_media_or_voice_busy(&snap));

	snap.firefox_mpris_playing = true;
	strcpy(snap.firefox_mpris_title, "C11 Deep Dive");
	bridge_resolve_state(&snap, 20, 4000ULL, &out);
	TEST_ASSERT_EQUAL(BRIDGE_CTX_KINO, out.ctx);
	TEST_ASSERT_EQUAL_STRING("C11 Deep Dive", out.top_text);
}

static void test_strict_priority_vesktop_gaming_kino_spotify_coding(void)
{
	host_snapshot_t snap;
	memset(&snap, 0, sizeof(snap));

	/* 1. Active Vesktop call overrides Gaming, Kino, Spotify, Emacs */
	snap.vesktop_voice_active = true;
	snap.mic_muted = true;
	snap.focused_fullscreen = true;
	strcpy(snap.focused_app, "steam_app_1234");
	strcpy(snap.focused_title, "Hades II");
	snap.firefox_mpris_playing = true;
	snap.spotify_playing = true;
	strcpy(snap.spotify_artist, "Perturbator");
	strcpy(snap.spotify_title, "Future Club");

	bridge_resolved_state_t out;
	bridge_resolve_state(&snap, 14, 4500ULL, &out);
	TEST_ASSERT_EQUAL(BRIDGE_CTX_VESKTOP, out.ctx);
	TEST_ASSERT_TRUE((out.flags & BRIDGE_FLAG_VOICE_ACTIVE) != 0);
	TEST_ASSERT_TRUE((out.flags & BRIDGE_FLAG_MIC_MUTED) != 0);
	TEST_ASSERT_TRUE((out.flags & BRIDGE_FLAG_MEDIA_ACTIVE) != 0);

	/* 2. Drop Vesktop -> Gaming beats Kino & Spotify */
	snap.vesktop_voice_active = false;
	bridge_resolve_state(&snap, 14, 5000ULL, &out);
	TEST_ASSERT_EQUAL(BRIDGE_CTX_GAMING, out.ctx);

	/* 3. Drop Gaming -> Kino (playing Firefox) beats Spotify & Emacs */
	snap.focused_fullscreen = false;
	strcpy(snap.focused_app, "emacs");
	strcpy(snap.emacs_buffer, "bridge_state.c");
	strcpy(snap.emacs_mode, "c");
	bridge_resolve_state(&snap, 14, 5500ULL, &out);
	TEST_ASSERT_EQUAL(BRIDGE_CTX_KINO, out.ctx);

	/* 4. Drop Kino -> actively playing Spotify beats focused Emacs/Terminal */
	snap.firefox_mpris_playing = false;
	bridge_resolve_state(&snap, 14, 6000ULL, &out);
	TEST_ASSERT_EQUAL(BRIDGE_CTX_SPOTIFY, out.ctx);
	TEST_ASSERT_EQUAL_STRING("Perturbator - Future Club", out.top_text);

	/* 5. Pause Spotify -> drops cleanly to focused Emacs / Terminal */
	snap.spotify_playing = false;
	snap.typing_active = true;
	snap.typing_expires_ms = 7000ULL;
	bridge_resolve_state(&snap, 2, 6500ULL, &out);
	TEST_ASSERT_EQUAL(BRIDGE_CTX_EMACS, out.ctx);
	TEST_ASSERT_EQUAL_STRING("bridge_state.c", out.top_text);
	TEST_ASSERT_TRUE((out.flags & BRIDGE_FLAG_LATE_NIGHT) != 0);
	TEST_ASSERT_TRUE((out.flags & BRIDGE_FLAG_TYPING_PULSE) != 0);

	strcpy(snap.focused_app, "foot");
	strcpy(snap.focused_title, "nvim");
	bridge_resolve_state(&snap, 12, 7500ULL, &out);
	TEST_ASSERT_EQUAL(BRIDGE_CTX_TERM, out.ctx);
	TEST_ASSERT_TRUE((out.flags & BRIDGE_FLAG_LATE_NIGHT) == 0);
	TEST_ASSERT_TRUE((out.flags & BRIDGE_FLAG_TYPING_PULSE) == 0);
}

static void test_late_night_boundaries(void)
{
	TEST_ASSERT_FALSE(bridge_is_late_night(0));
	TEST_ASSERT_TRUE(bridge_is_late_night(1));
	TEST_ASSERT_TRUE(bridge_is_late_night(2));
	TEST_ASSERT_TRUE(bridge_is_late_night(3));
	TEST_ASSERT_FALSE(bridge_is_late_night(4));
	TEST_ASSERT_FALSE(bridge_is_late_night(23));
}

static void test_idle_tracker_media_guard_and_wake(void)
{
	bridge_idle_t idle;
	bridge_idle_init(&idle, 1800U, 0ULL);

	/* Busy at 29m59s resets timer so subsequent 29m still doesn't sleep */
	TEST_ASSERT_EQUAL(BRIDGE_IDLE_NONE,
			  bridge_idle_step(&idle, false, 1799000ULL));
	TEST_ASSERT_EQUAL(BRIDGE_IDLE_NONE,
			  bridge_idle_step(&idle, true, 1800000ULL));
	TEST_ASSERT_EQUAL(BRIDGE_IDLE_NONE,
			  bridge_idle_step(&idle, false, 3599000ULL));

	/* Hits 1800s without media after 1800000ms -> 3600000ms */
	TEST_ASSERT_EQUAL(BRIDGE_IDLE_SLEEP,
			  bridge_idle_step(&idle, false, 3600000ULL));
	TEST_ASSERT_TRUE(idle.sleeping);

	/* Repeated step stays sleeping */
	TEST_ASSERT_EQUAL(BRIDGE_IDLE_NONE,
			  bridge_idle_step(&idle, false, 3650000ULL));

	/* Activity wakes cleanly */
	TEST_ASSERT_EQUAL(BRIDGE_IDLE_WAKE,
			  bridge_idle_record_activity(&idle, 3700000ULL));
	TEST_ASSERT_FALSE(idle.sleeping);
}

void run_bridge_state_tests(void)
{
	RUN_TEST(test_priority_agent_overrides_all);
	RUN_TEST(test_firefox_docs_not_kino_unless_video_or_mpris);
	RUN_TEST(test_firefox_youtube_playing_vs_paused_kino);
	RUN_TEST(test_strict_priority_vesktop_gaming_kino_spotify_coding);
	RUN_TEST(test_late_night_boundaries);
	RUN_TEST(test_idle_tracker_media_guard_and_wake);
}
