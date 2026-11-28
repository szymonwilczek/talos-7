#include "bridge_daemon.h"
#include "bridge_monitors.h"
#include "bridge_socket.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

static void test_parse_sway_tree_and_events(void)
{
	const char *tree =
		"{\"nodes\":[{\"name\":\"ws1\",\"focused\":false,\"nodes\":["
		"{\"app_id\":\"emacs\",\"name\":\"init.el\",\"fullscreen_mode\":0,"
		"\"focused\":true}]}]}";

	char app[64];
	char title[128];
	bool fs = true;
	bool emacs_open = false;
	TEST_ASSERT_TRUE(bridge_parse_sway_tree_state(tree, app, sizeof(app),
						      title, sizeof(title), &fs,
						      &emacs_open));
	TEST_ASSERT_EQUAL_STRING("emacs", app);
	TEST_ASSERT_EQUAL_STRING("init.el", title);
	TEST_ASSERT_FALSE(fs);
	TEST_ASSERT_TRUE(emacs_open);

	const char *split_tree =
		"{\"nodes\":[{\"name\":\"ws1\",\"focused\":false,\"nodes\":["
		"{\"app_id\":\"emacs\",\"name\":\"MERGE_MSG\",\"fullscreen_mode\":0,\"focused\":false},"
		"{\"app_id\":\"kitty\",\"name\":\"agy -c\",\"fullscreen_mode\":0,\"focused\":true}]}]}";
	TEST_ASSERT_TRUE(bridge_parse_sway_tree_state(
		split_tree, app, sizeof(app), title, sizeof(title), &fs,
		&emacs_open));
	TEST_ASSERT_EQUAL_STRING("kitty", app);
	TEST_ASSERT_TRUE(emacs_open);

	/* Unfocused title change should be ignored */
	const char *bg_evt =
		"{\"change\":\"title\",\"container\":{\"focused\":false,"
		"\"app_id\":\"firefox\",\"name\":\"New Mail\",\"fullscreen_mode\":0}}";
	TEST_ASSERT_FALSE(bridge_parse_sway_event(bg_evt, app, sizeof(app),
						  title, sizeof(title), &fs));

	/* Focused XWayland window event extracts window_properties.class */
	const char *game_evt =
		"{\"change\":\"focus\",\"container\":{\"focused\":true,"
		"\"app_id\":null,\"window_properties\":{\"class\":\"steam_app_999\"},"
		"\"name\":\"DOOM\",\"fullscreen_mode\":1}}";
	TEST_ASSERT_TRUE(bridge_parse_sway_event(game_evt, app, sizeof(app),
						 title, sizeof(title), &fs));
	TEST_ASSERT_EQUAL_STRING("steam_app_999", app);
	TEST_ASSERT_EQUAL_STRING("DOOM", title);
	TEST_ASSERT_TRUE(fs);
}

static void test_sway_binary_frame_buffer(void)
{
	uint8_t buf[128];
	const char *payload = "{\"change\":\"focus\"}";
	uint32_t plen = (uint32_t)strlen(payload);
	uint32_t mtype = SWAY_IPC_EVENT_WINDOW;
	memcpy(buf, SWAY_IPC_MAGIC, 6);
	memcpy(buf + 6, &plen, 4);
	memcpy(buf + 10, &mtype, 4);
	memcpy(buf + 14, payload, plen);

	/* Partial frame wait */
	size_t partial_len = 14U + 4U;
	uint32_t out_type = 0;
	char out_payload[64];
	TEST_ASSERT_EQUAL(0, bridge_sway_consume_frame(buf, &partial_len,
						       &out_type, out_payload,
						       sizeof(out_payload)));

	size_t full_len = 14U + plen;
	TEST_ASSERT_EQUAL(1, bridge_sway_consume_frame(buf, &full_len,
						       &out_type, out_payload,
						       sizeof(out_payload)));
	TEST_ASSERT_EQUAL_UINT32(0x80000003U, SWAY_IPC_EVENT_WINDOW);
	TEST_ASSERT_EQUAL_UINT32(0x80000000U, SWAY_IPC_EVENT_WORKSPACE);
	TEST_ASSERT_EQUAL_UINT32(SWAY_IPC_EVENT_WINDOW, out_type);
	TEST_ASSERT_EQUAL_STRING("{\"change\":\"focus\"}", out_payload);
	TEST_ASSERT_EQUAL(0, full_len);
}

static void test_mpris_pw_dump_and_pactl_parsers(void)
{
	const char *gdbus_out =
		"({'PlaybackStatus': <'Playing'>, 'Metadata': <{'xesam:artist': "
		"<['Daft Punk']>, 'xesam:title': <'Voyager'>}>},)";
	bool playing = false;
	char artist[64];
	char title[64];
	TEST_ASSERT_TRUE(bridge_parse_gdbus_mpris(gdbus_out, &playing, artist,
						  sizeof(artist), title,
						  sizeof(title)));
	TEST_ASSERT_TRUE(playing);
	TEST_ASSERT_EQUAL_STRING("Daft Punk", artist);
	TEST_ASSERT_EQUAL_STRING("Voyager", title);

	const char *pw_json =
		"[{\"id\":10,\"info\":{\"state\":\"idle\",\"props\":{\"media.class\":"
		"\"Stream/Input/Audio\",\"application.name\":\"firefox\"}}},"
		"{\"id\":42,\"info\":{\"state\":\"running\",\"props\":{\"media.class\":"
		"\"Stream/Input/Audio\",\"application.name\":\"WEBRTC VoiceEngine\","
		"\"application.process.binary\":\"vesktop\"}}}]";
	bool voice_active = false;
	TEST_ASSERT_TRUE(bridge_parse_pw_dump(pw_json, &voice_active));
	TEST_ASSERT_TRUE(voice_active);

	const char *pw_idle =
		"[{\"id\":42,\"info\":{\"state\":\"suspended\",\"props\":{\"media.class\":"
		"\"Stream/Input/Audio\",\"application.name\":\"vesktop\"}}}]";
	TEST_ASSERT_TRUE(bridge_parse_pw_dump(pw_idle, &voice_active));
	TEST_ASSERT_FALSE(voice_active);

	bool muted = false;
	TEST_ASSERT_TRUE(bridge_parse_pactl_mute("Mute: yes\n", &muted));
	TEST_ASSERT_TRUE(muted);
	TEST_ASSERT_TRUE(bridge_parse_pactl_mute("Mute: no\n", &muted));
	TEST_ASSERT_FALSE(muted);
	TEST_ASSERT_TRUE(bridge_parse_pactl_mute("Wyciszenie: tak\n", &muted));
	TEST_ASSERT_TRUE(muted);

	const char *pactl_so_json =
		"[{\"index\":30856,\"corked\":false,\"properties\":{"
		"\"pipewire.access.portal.app_id\":\"dev.vencord.Vesktop\","
		"\"application.name\":\"vesktop\",\"media.name\":\"RecordStream\"}}]";
	TEST_ASSERT_TRUE(bridge_parse_pactl_source_outputs(pactl_so_json,
							   &voice_active));
	TEST_ASSERT_TRUE(voice_active);

	const char *pactl_so_corked =
		"[{\"index\":30856,\"corked\":true,\"properties\":{"
		"\"application.name\":\"vesktop\"}}]";
	TEST_ASSERT_TRUE(bridge_parse_pactl_source_outputs(pactl_so_corked,
							   &voice_active));
	TEST_ASSERT_FALSE(voice_active);

	uint8_t vol = 0;
	TEST_ASSERT_TRUE(bridge_parse_pactl_volume(
		"Volume: front-left: 42598 /  65% / -11.20 dB, "
		"front-right: 42598 /  65% / -11.20 dB",
		&vol));
	TEST_ASSERT_EQUAL_UINT8(65, vol);
	TEST_ASSERT_TRUE(bridge_parse_pactl_volume(
		"Volume: mono: 98304 / 150% / 10.50 dB", &vol));
	TEST_ASSERT_EQUAL_UINT8(100, vol);
}

typedef struct {
	int count;
	char last_line[128];
} captured_lines_t;

static void capture_sock_line(const char *line, void *raw_ctx)
{
	captured_lines_t *cap = (captured_lines_t *)raw_ctx;
	cap->count++;
	snprintf(cap->last_line, sizeof(cap->last_line), "%s", line);
}

static void test_unix_socket_roundtrip_and_daemon_commands(void)
{
	char sock_path[108];
	snprintf(sock_path, sizeof(sock_path), "/tmp/talos_bridge_test_%d.sock",
		 (int)getpid());
	unlink(sock_path);

	bridge_socket_server_t srv;
	TEST_ASSERT_TRUE(bridge_socket_open(&srv, sock_path));

	int cli = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(cli >= 0);
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sock_path);
	TEST_ASSERT_EQUAL(0,
			  connect(cli, (struct sockaddr *)&addr, sizeof(addr)));

	const char *msg = "agent-done|Build Green\n";
	TEST_ASSERT_TRUE(write(cli, msg, strlen(msg)) == (ssize_t)strlen(msg));
	close(cli);

	captured_lines_t cap;
	memset(&cap, 0, sizeof(cap));
	bridge_socket_service(&srv, capture_sock_line, &cap);
	TEST_ASSERT_EQUAL(1, cap.count);
	TEST_ASSERT_EQUAL_STRING("agent-done|Build Green", cap.last_line);
	bridge_socket_close(&srv);

	/* Daemon line dispatch + guarded idle */
	bridge_daemon_t daemon;
	bridge_daemon_init(&daemon, NULL, NULL, 1800U, NULL, 1000ULL);
	TEST_ASSERT_TRUE(bridge_daemon_handle_line(
		&daemon, "emacs-buf|plan.org|org", 1100ULL));
	TEST_ASSERT_EQUAL_STRING("plan.org", daemon.snap.emacs_buffer);
	TEST_ASSERT_EQUAL_STRING("org", daemon.snap.emacs_mode);

	daemon.snap.spotify_playing = true;
	TEST_ASSERT_TRUE(
		bridge_daemon_handle_line(&daemon, "idle-30m", 1200ULL));
	TEST_ASSERT_FALSE(daemon.snap.sleeping);

	daemon.snap.spotify_playing = false;
	TEST_ASSERT_TRUE(
		bridge_daemon_handle_line(&daemon, "idle-30m", 1300ULL));
	TEST_ASSERT_TRUE(daemon.snap.sleeping);

	TEST_ASSERT_TRUE(
		bridge_daemon_handle_line(&daemon, "load|87|45", 1400ULL));
	TEST_ASSERT_TRUE(daemon.snap.load_override);
	TEST_ASSERT_EQUAL_UINT8(2, daemon.snap.load_tier);
	TEST_ASSERT_FALSE(daemon.snap.sleeping);

	TEST_ASSERT_TRUE(
		bridge_daemon_handle_line(&daemon, "load|clear", 1500ULL));
	TEST_ASSERT_FALSE(daemon.snap.load_override);
	TEST_ASSERT_EQUAL_UINT8(0, daemon.snap.load_tier);
}

static void test_meminfo_and_hwmon_temp_parsers(void)
{
	uint8_t ram_pct = 0;
	const char *mem = "MemTotal:       10000000 kB\n"
			  "MemFree:         1000000 kB\n"
			  "MemAvailable:    1500000 kB\n";
	TEST_ASSERT_TRUE(bridge_parse_meminfo(mem, &ram_pct));
	TEST_ASSERT_EQUAL_UINT8(85, ram_pct);

	char tmpl[] = "/tmp/talos_hwmon_XXXXXX";
	char *root = mkdtemp(tmpl);
	TEST_ASSERT_NOT_NULL(root);

	char hw0[256], hw1[256], fpath[300];
	snprintf(hw0, sizeof(hw0), "%s/hwmon0", root);
	snprintf(hw1, sizeof(hw1), "%s/hwmon1", root);
	TEST_ASSERT_EQUAL(0, mkdir(hw0, 0755));
	TEST_ASSERT_EQUAL(0, mkdir(hw1, 0755));

	snprintf(fpath, sizeof(fpath), "%s/name", hw0);
	FILE *f = fopen(fpath, "w");
	TEST_ASSERT_NOT_NULL(f);
	fputs("nvme\n", f);
	fclose(f);
	snprintf(fpath, sizeof(fpath), "%s/temp1_input", hw0);
	f = fopen(fpath, "w");
	fputs("95000\n", f);
	fclose(f);

	snprintf(fpath, sizeof(fpath), "%s/name", hw1);
	f = fopen(fpath, "w");
	fputs("k10temp\n", f);
	fclose(f);
	snprintf(fpath, sizeof(fpath), "%s/temp1_input", hw1);
	f = fopen(fpath, "w");
	fputs("84500\n", f);
	fclose(f);

	uint8_t temp_c = 0;
	TEST_ASSERT_TRUE(bridge_read_cpu_temp_in_sysfs(root, &temp_c));
	TEST_ASSERT_EQUAL_UINT8(84, temp_c);
}

void run_bridge_monitors_tests(void)
{
	RUN_TEST(test_parse_sway_tree_and_events);
	RUN_TEST(test_sway_binary_frame_buffer);
	RUN_TEST(test_mpris_pw_dump_and_pactl_parsers);
	RUN_TEST(test_unix_socket_roundtrip_and_daemon_commands);
	RUN_TEST(test_meminfo_and_hwmon_temp_parsers);
}
