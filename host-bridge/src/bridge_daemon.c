#include "bridge_daemon.h"

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t g_stop_requested = 0;

static void on_stop_signal(int sig)
{
	(void)sig;
	g_stop_requested = 1;
}

static uint64_t monotonic_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000ULL +
	       (uint64_t)(ts.tv_nsec / 1000000L);
}

static void split_part(const char **cursor, char *dst, size_t dst_size)
{
	if (!cursor || !*cursor) {
		if (dst_size > 0)
			dst[0] = '\0';
		return;
	}
	const char *p = *cursor;
	const char *pipe = strchr(p, '|');
	size_t len = pipe ? (size_t)(pipe - p) : strlen(p);
	if (len >= dst_size)
		len = dst_size - 1;
	if (len > 0)
		memcpy(dst, p, len);
	dst[len] = '\0';
	*cursor = pipe ? (pipe + 1) : (p + strlen(p));
}

void bridge_daemon_init(bridge_daemon_t *daemon, const char *device_override,
			const char *socket_override, uint32_t idle_timeout_sec,
			const bridge_serial_ops_t *serial_ops, uint64_t now_ms)
{
	if (!daemon)
		return;
	memset(daemon, 0, sizeof(*daemon));
	daemon->hour_override = -1;
	daemon->sock.listen_fd = -1;
	if (socket_override && socket_override[0] != '\0') {
		snprintf(daemon->socket_override,
			 sizeof(daemon->socket_override), "%s",
			 socket_override);
	}
	bridge_serial_init(&daemon->serial, device_override, serial_ops);
	bridge_idle_init(&daemon->idle, idle_timeout_sec, now_ms);
}

static void notify_activity(bridge_daemon_t *daemon, uint64_t now_ms)
{
	if (bridge_idle_record_activity(&daemon->idle, now_ms) ==
	    BRIDGE_IDLE_WAKE) {
		daemon->snap.sleeping = false;
		bridge_serial_send_command(&daemon->serial, "COMP_WAKE");
	}
}

bool bridge_daemon_handle_line(bridge_daemon_t *daemon, const char *line,
			       uint64_t now_ms)
{
	if (!daemon || !line || line[0] == '\0')
		return false;

	const char *cur = line;
	char cmd[32] = { 0 };
	split_part(&cur, cmd, sizeof(cmd));

	if (strcmp(cmd, "emacs-buf") == 0) {
		split_part(&cur, daemon->snap.emacs_buffer,
			   sizeof(daemon->snap.emacs_buffer));
		split_part(&cur, daemon->snap.emacs_mode,
			   sizeof(daemon->snap.emacs_mode));
		notify_activity(daemon, now_ms);
		return true;
	}
	if (strcmp(cmd, "typing") == 0) {
		daemon->snap.typing_active = true;
		daemon->snap.typing_expires_ms = now_ms + 1500ULL;
		notify_activity(daemon, now_ms);
		return true;
	}
	if (strcmp(cmd, "agent-start") == 0) {
		daemon->snap.agent_thinking = true;
		split_part(&cur, daemon->snap.agent_task,
			   sizeof(daemon->snap.agent_task));
		notify_activity(daemon, now_ms);
		return true;
	}
	if (strcmp(cmd, "agent-done") == 0 || strcmp(cmd, "agent-err") == 0 ||
	    strcmp(cmd, "agent-input") == 0) {
		char label[64] = { 0 };
		split_part(&cur, label, sizeof(label));
		daemon->snap.agent_thinking = false;
		daemon->snap.agent_task[0] = '\0';
		notify_activity(daemon, now_ms);

		const char *evt = strcmp(cmd, "agent-done") == 0 ?
					  "AGENT_DONE" :
					  (strcmp(cmd, "agent-err") == 0 ?
						   "AGENT_ERR" :
						   "NEEDS_INPUT");
		uint32_t dur = strcmp(cmd, "agent-done") == 0 ? 5000U : 8000U;
		bridge_serial_send_event(&daemon->serial, evt, dur, label);
		return true;
	}
	if (strcmp(cmd, "git-commit") == 0) {
		char label[64] = { 0 };
		split_part(&cur, label, sizeof(label));
		notify_activity(daemon, now_ms);
		bridge_serial_send_event(&daemon->serial, "GIT_COMMIT", 4000U,
					 label);
		return true;
	}
	if (strcmp(cmd, "ping") == 0 || strcmp(cmd, "pet") == 0 ||
	    strcmp(cmd, "coffee") == 0) {
		char label[64] = { 0 };
		split_part(&cur, label, sizeof(label));
		notify_activity(daemon, now_ms);
		const char *evt =
			strcmp(cmd, "ping") == 0 ?
				"PING" :
				(strcmp(cmd, "pet") == 0 ? "PET" : "COFFEE");
		bridge_serial_send_event(&daemon->serial, evt, 4500U, label);
		return true;
	}
	if (strcmp(cmd, "pause") == 0) {
		bridge_serial_set_paused(&daemon->serial, true);
		return true;
	}
	if (strcmp(cmd, "resume") == 0) {
		bridge_serial_set_paused(&daemon->serial, false);
		daemon->last_time_sync_ms = 0;
		return true;
	}
	if (strcmp(cmd, "idle-30m") == 0) {
		if (!bridge_is_media_or_voice_busy(&daemon->snap) &&
		    bridge_idle_force_sleep(&daemon->idle) ==
			    BRIDGE_IDLE_SLEEP) {
			daemon->snap.sleeping = true;
			bridge_serial_send_command(&daemon->serial,
						   "COMP_SLEEP");
		}
		return true;
	}
	if (strcmp(cmd, "idle-resume") == 0) {
		notify_activity(daemon, now_ms);
		return true;
	}
	return false;
}

void bridge_daemon_tick(bridge_daemon_t *daemon, uint64_t now_ms,
			time_t wall_time, bool activity_occurred)
{
	if (!daemon)
		return;

	if (daemon->snap.typing_active &&
	    now_ms > daemon->snap.typing_expires_ms) {
		daemon->snap.typing_active = false;
	}

	if (activity_occurred)
		notify_activity(daemon, now_ms);

	bool busy = bridge_is_media_or_voice_busy(&daemon->snap);
	bridge_idle_transition_t tr =
		bridge_idle_step(&daemon->idle, busy, now_ms);
	if (tr == BRIDGE_IDLE_SLEEP) {
		daemon->snap.sleeping = true;
		bridge_serial_send_command(&daemon->serial, "COMP_SLEEP");
	} else if (tr == BRIDGE_IDLE_WAKE) {
		daemon->snap.sleeping = false;
		bridge_serial_send_command(&daemon->serial, "COMP_WAKE");
	}

	int hour = 12;
	int minute = 0;
	if (daemon->hour_override >= 0) {
		hour = daemon->hour_override;
		minute = daemon->minute_override;
	} else {
		struct tm local_tm;
		if (localtime_r(&wall_time, &local_tm)) {
			hour = local_tm.tm_hour;
			minute = local_tm.tm_min;
		}
	}

	if (daemon->last_time_sync_ms == 0 ||
	    (now_ms >= daemon->last_time_sync_ms &&
	     (now_ms - daemon->last_time_sync_ms) >=
		     BRIDGE_TIME_SYNC_INTERVAL_MS)) {
		bridge_serial_send_time(&daemon->serial, (uint8_t)hour,
					(uint8_t)minute);
		daemon->last_time_sync_ms = now_ms;
	}

	bridge_resolved_state_t resolved;
	bridge_resolve_state(&daemon->snap, hour, now_ms, &resolved);
	bridge_serial_send_state(&daemon->serial, resolved.ctx, resolved.flags,
				 resolved.top_text, resolved.sub_text, now_ms);
	if (daemon->snap.sink_vol_valid) {
		bridge_serial_send_volume(&daemon->serial,
					  daemon->snap.sink_vol_pct,
					  daemon->snap.sink_muted);
	}
}

typedef struct {
	bridge_daemon_t *daemon;
	uint64_t now_ms;
} sock_cb_ctx_t;

static void on_socket_line(const char *line, void *raw_ctx)
{
	sock_cb_ctx_t *ctx = (sock_cb_ctx_t *)raw_ctx;
	bridge_daemon_handle_line(ctx->daemon, line, ctx->now_ms);
}

int bridge_daemon_run(bridge_daemon_t *daemon)
{
	if (!daemon)
		return 1;

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = on_stop_signal;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	signal(SIGPIPE, SIG_IGN);

	bridge_monitors_init(&daemon->monitors, &daemon->snap);
	bridge_socket_open(&daemon->sock, daemon->socket_override);
	bridge_daemon_tick(daemon, monotonic_ms(), time(NULL), false);

	while (!g_stop_requested) {
		struct pollfd pfds[4 + BRIDGE_SOCK_MAX_CLIENTS];
		nfds_t nfds = 0;
		int sway_idx = -1;
		int dbus_idx = -1;
		int pactl_idx = -1;

		if (daemon->monitors.sway_fd >= 0) {
			sway_idx = (int)nfds;
			pfds[nfds++] =
				(struct pollfd){ .fd = daemon->monitors.sway_fd,
						 .events = POLLIN };
		}
		if (daemon->monitors.dbus_fd >= 0) {
			dbus_idx = (int)nfds;
			pfds[nfds++] =
				(struct pollfd){ .fd = daemon->monitors.dbus_fd,
						 .events = POLLIN };
		}
		if (daemon->monitors.pactl_fd >= 0) {
			pactl_idx = (int)nfds;
			pfds[nfds++] = (struct pollfd){
				.fd = daemon->monitors.pactl_fd,
				.events = POLLIN
			};
		}

		nfds_t sock_cap =
			(nfds_t)(sizeof(pfds) / sizeof(pfds[0])) - nfds;
		nfds += bridge_socket_fill_pollfds(&daemon->sock, pfds + nfds,
						   sock_cap);

		int rc = poll(pfds, nfds, 500);
		if (rc < 0 && errno == EINTR)
			continue;

		bool sway_r = (sway_idx >= 0 && (pfds[sway_idx].revents &
						 (POLLIN | POLLHUP | POLLERR)));
		bool dbus_r = (dbus_idx >= 0 && (pfds[dbus_idx].revents &
						 (POLLIN | POLLHUP | POLLERR)));
		bool pactl_r =
			(pactl_idx >= 0 && (pfds[pactl_idx].revents &
					    (POLLIN | POLLHUP | POLLERR)));

		uint64_t now_ms = monotonic_ms();
		bool activity = false;
		bridge_monitors_process_events(&daemon->monitors, &daemon->snap,
					       now_ms, sway_r, dbus_r, pactl_r,
					       &activity);
		sock_cb_ctx_t cb_ctx = { .daemon = daemon, .now_ms = now_ms };
		bridge_socket_service(&daemon->sock, on_socket_line, &cb_ctx);
		bridge_daemon_tick(daemon, now_ms, time(NULL), activity);
	}

	bridge_socket_close(&daemon->sock);
	bridge_monitors_close(&daemon->monitors);
	bridge_serial_close(&daemon->serial);
	return 0;
}
