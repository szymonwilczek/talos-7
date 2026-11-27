#include "bridge_monitors.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

static bool ci_contains(const char *haystack, const char *needle)
{
	if (!haystack || !needle || needle[0] == '\0')
		return false;
	size_t nlen = strlen(needle);
	for (const char *h = haystack; *h != '\0'; h++) {
		size_t i = 0;
		while (i < nlen && h[i] != '\0' &&
		       tolower((unsigned char)h[i]) ==
			       tolower((unsigned char)needle[i])) {
			i++;
		}
		if (i == nlen)
			return true;
	}
	return false;
}

static const char *skip_ws(const char *p)
{
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
		p++;
	return p;
}

static const char *parse_json_string(const char *p, char *dst, size_t dst_size)
{
	if (*p != '"')
		return NULL;
	p++;
	size_t out = 0;
	while (*p != '\0' && *p != '"') {
		char ch = *p++;
		if (ch == '\\' && *p != '\0') {
			char esc = *p++;
			switch (esc) {
			case '"':
			case '\\':
			case '/':
				ch = esc;
				break;
			case 'b':
				ch = '\b';
				break;
			case 'f':
				ch = '\f';
				break;
			case 'n':
				ch = '\n';
				break;
			case 'r':
				ch = '\r';
				break;
			case 't':
				ch = '\t';
				break;
			case 'u':
				for (int i = 0; i < 4 && *p != '\0'; i++)
					p++;
				ch = '?';
				break;
			default:
				ch = esc;
				break;
			}
		}
		if (dst && out + 1 < dst_size)
			dst[out++] = ch;
	}
	if (dst && dst_size > 0)
		dst[out] = '\0';
	return (*p == '"') ? (p + 1) : NULL;
}

static const char *skip_json_value(const char *p)
{
	p = skip_ws(p);
	if (*p == '"')
		return parse_json_string(p, NULL, 0);
	if (*p == '{' || *p == '[') {
		char open_ch = *p;
		char close_ch = (open_ch == '{') ? '}' : ']';
		int depth = 1;
		p++;
		while (*p != '\0' && depth > 0) {
			if (*p == '"') {
				p = parse_json_string(p, NULL, 0);
				if (!p)
					return NULL;
				continue;
			}
			if (*p == open_ch)
				depth++;
			else if (*p == close_ch)
				depth--;
			p++;
		}
		return (depth == 0) ? p : NULL;
	}
	while (*p != '\0' && *p != ',' && *p != '}' && *p != ']' &&
	       !isspace((unsigned char)*p)) {
		p++;
	}
	return p;
}

/* Look up top-level key directly inside JSON object pointed to by obj_start. */
static const char *json_obj_find_key(const char *obj_start, const char *key,
				     const char **out_val)
{
	const char *p = skip_ws(obj_start);
	if (*p != '{')
		return NULL;
	p = skip_ws(p + 1);
	while (*p != '\0' && *p != '}') {
		char cur_key[64];
		p = skip_ws(p);
		if (*p != '"')
			return NULL;
		p = parse_json_string(p, cur_key, sizeof(cur_key));
		if (!p)
			return NULL;
		p = skip_ws(p);
		if (*p != ':')
			return NULL;
		p = skip_ws(p + 1);
		if (strcmp(cur_key, key) == 0) {
			*out_val = p;
			return p;
		}
		p = skip_json_value(p);
		if (!p)
			return NULL;
		p = skip_ws(p);
		if (*p == ',')
			p++;
	}
	return NULL;
}

static bool extract_sway_node_props(const char *node_obj, char *out_app,
				    size_t app_size, char *out_title,
				    size_t title_size, bool *out_fullscreen,
				    bool *out_focused)
{
	const char *v = NULL;
	bool focused = false;
	if (json_obj_find_key(node_obj, "focused", &v)) {
		focused = (strncmp(v, "true", 4) == 0);
	}
	if (out_focused)
		*out_focused = focused;

	char app[128] = { 0 };
	if (json_obj_find_key(node_obj, "app_id", &v) && *v == '"') {
		parse_json_string(v, app, sizeof(app));
	}
	if (app[0] == '\0' &&
	    json_obj_find_key(node_obj, "window_properties", &v) && *v == '{') {
		const char *cls = NULL;
		if (json_obj_find_key(v, "class", &cls) && *cls == '"') {
			parse_json_string(cls, app, sizeof(app));
		}
	}

	char title[256] = { 0 };
	if (json_obj_find_key(node_obj, "name", &v) && *v == '"') {
		parse_json_string(v, title, sizeof(title));
	}

	bool fs = false;
	if (json_obj_find_key(node_obj, "fullscreen_mode", &v)) {
		fs = (strtol(v, NULL, 10) > 0);
	}

	snprintf(out_app, app_size, "%s", app);
	snprintf(out_title, title_size, "%s", title);
	*out_fullscreen = fs;
	return true;
}

static bool walk_sway_node(const char *node_obj, char *out_app, size_t app_size,
			   char *out_title, size_t title_size,
			   bool *out_fullscreen)
{
	const char *v = NULL;
	if (json_obj_find_key(node_obj, "focused", &v) &&
	    strncmp(v, "true", 4) == 0) {
		/* Also check leaf/view identity; containers with focus can still carry name */
		return extract_sway_node_props(node_obj, out_app, app_size,
					       out_title, title_size,
					       out_fullscreen, NULL);
	}

	static const char *const CHILD_KEYS[] = { "nodes", "floating_nodes" };
	for (size_t k = 0; k < 2; k++) {
		const char *arr = NULL;
		if (!json_obj_find_key(node_obj, CHILD_KEYS[k], &arr) ||
		    *arr != '[')
			continue;
		const char *p = skip_ws(arr + 1);
		while (*p != '\0' && *p != ']') {
			p = skip_ws(p);
			if (*p == '{') {
				if (walk_sway_node(p, out_app, app_size,
						   out_title, title_size,
						   out_fullscreen)) {
					return true;
				}
			}
			p = skip_json_value(p);
			if (!p)
				break;
			p = skip_ws(p);
			if (*p == ',')
				p++;
		}
	}
	return false;
}

bool bridge_parse_sway_tree(const char *json, char *out_app, size_t app_size,
			    char *out_title, size_t title_size,
			    bool *out_fullscreen)
{
	if (!json || !out_app || !out_title || !out_fullscreen)
		return false;
	out_app[0] = '\0';
	out_title[0] = '\0';
	*out_fullscreen = false;
	return walk_sway_node(json, out_app, app_size, out_title, title_size,
			      out_fullscreen);
}

bool bridge_parse_sway_event(const char *json, char *out_app, size_t app_size,
			     char *out_title, size_t title_size,
			     bool *out_fullscreen)
{
	if (!json || !out_app || !out_title || !out_fullscreen)
		return false;

	const char *chg_v = NULL;
	char change[32] = { 0 };
	if (!json_obj_find_key(json, "change", &chg_v) || *chg_v != '"')
		return false;
	parse_json_string(chg_v, change, sizeof(change));

	if (strcmp(change, "focus") != 0 && strcmp(change, "title") != 0 &&
	    strcmp(change, "fullscreen_mode") != 0) {
		return false;
	}

	const char *cont = NULL;
	if (!json_obj_find_key(json, "container", &cont) || *cont != '{')
		return false;

	bool focused = false;
	char app[128] = { 0 };
	char title[256] = { 0 };
	bool fs = false;
	extract_sway_node_props(cont, app, sizeof(app), title, sizeof(title),
				&fs, &focused);
	if (!focused && strcmp(change, "focus") != 0)
		return false;

	snprintf(out_app, app_size, "%s", app);
	snprintf(out_title, title_size, "%s", title);
	*out_fullscreen = fs;
	return true;
}

static const char *read_gvariant_str(const char *p, char *dst, size_t dst_size)
{
	p = skip_ws(p);
	if (*p != '\'' && *p != '"')
		return NULL;
	char quote = *p++;
	size_t out = 0;
	while (*p != '\0' && *p != quote) {
		if (*p == '\\' && *(p + 1) != '\0')
			p++;
		if (dst && out + 1 < dst_size)
			dst[out++] = *p;
		p++;
	}
	if (dst && dst_size > 0)
		dst[out] = '\0';
	return (*p == quote) ? (p + 1) : NULL;
}

bool bridge_parse_gdbus_mpris(const char *text, bool *out_playing,
			      char *out_artist, size_t artist_size,
			      char *out_title, size_t title_size)
{
	if (!text || !out_playing || !out_artist || !out_title)
		return false;
	*out_playing = false;
	out_artist[0] = '\0';
	out_title[0] = '\0';

	const char *status_key = strstr(text, "PlaybackStatus");
	if (!status_key)
		return false;

	const char *lt = strchr(status_key, '<');
	if (lt) {
		char status[32] = { 0 };
		if (read_gvariant_str(lt + 1, status, sizeof(status))) {
			*out_playing = (strcmp(status, "Playing") == 0);
		}
	}

	const char *title_key = strstr(text, "xesam:title");
	if (title_key) {
		const char *t_lt = strchr(title_key, '<');
		if (t_lt)
			read_gvariant_str(t_lt + 1, out_title, title_size);
	}

	const char *artist_key = strstr(text, "xesam:artist");
	if (artist_key) {
		const char *br = strchr(artist_key, '[');
		if (br)
			read_gvariant_str(br + 1, out_artist, artist_size);
	}

	return true;
}

static bool pw_node_is_vesktop_voice(const char *node_obj)
{
	const char *info = NULL;
	if (!json_obj_find_key(node_obj, "info", &info) || *info != '{')
		return false;

	const char *state_v = NULL;
	char state[32] = { 0 };
	if (!json_obj_find_key(info, "state", &state_v) || *state_v != '"')
		return false;
	parse_json_string(state_v, state, sizeof(state));
	if (strcmp(state, "running") != 0)
		return false;

	const char *props = NULL;
	if (!json_obj_find_key(info, "props", &props) || *props != '{')
		return false;

	static const char *const PROP_KEYS[] = { "application.name",
						 "application.process.binary",
						 "node.name", "media.name" };
	bool is_voice_app = false;
	for (size_t i = 0; i < sizeof(PROP_KEYS) / sizeof(PROP_KEYS[0]); i++) {
		const char *pv = NULL;
		char val[128] = { 0 };
		if (json_obj_find_key(props, PROP_KEYS[i], &pv) && *pv == '"') {
			parse_json_string(pv, val, sizeof(val));
			if (ci_contains(val, "vesktop") ||
			    ci_contains(val, "discord") ||
			    ci_contains(val, "WEBRTC VoiceEngine")) {
				is_voice_app = true;
				break;
			}
		}
	}
	if (!is_voice_app)
		return false;

	const char *mclass = NULL;
	char mc[64] = { 0 };
	if (json_obj_find_key(props, "media.class", &mclass) &&
	    *mclass == '"') {
		parse_json_string(mclass, mc, sizeof(mc));
		return ci_contains(mc, "Stream/Input/Audio") ||
		       ci_contains(mc, "Stream/Output/Audio");
	}
	return false;
}

bool bridge_parse_pw_dump(const char *json, bool *out_voice_active)
{
	if (!json || !out_voice_active)
		return false;
	*out_voice_active = false;

	const char *p = skip_ws(json);
	if (*p != '[')
		return false;
	p = skip_ws(p + 1);
	while (*p != '\0' && *p != ']') {
		p = skip_ws(p);
		if (*p == '{') {
			if (pw_node_is_vesktop_voice(p)) {
				*out_voice_active = true;
				return true;
			}
		}
		p = skip_json_value(p);
		if (!p)
			break;
		p = skip_ws(p);
		if (*p == ',')
			p++;
	}
	return true;
}

bool bridge_parse_pactl_mute(const char *text, bool *out_muted)
{
	if (!text || !out_muted)
		return false;
	if (ci_contains(text, "yes")) {
		*out_muted = true;
		return true;
	}
	if (ci_contains(text, "no")) {
		*out_muted = false;
		return true;
	}
	return false;
}

static bool set_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
		return false;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

static int sway_ipc_connect(const char *sock_path)
{
	if (!sock_path || sock_path[0] == '\0')
		return -1;

	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	if (strlen(sock_path) >= sizeof(addr.sun_path)) {
		close(fd);
		return -1;
	}
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sock_path);

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		close(fd);
		return -1;
	}
	return fd;
}

static bool write_all(int fd, const uint8_t *buf, size_t len)
{
	size_t off = 0;
	while (off < len) {
		ssize_t n = write(fd, buf + off, len - off);
		if (n > 0) {
			off += (size_t)n;
			continue;
		}
		if (n < 0 && errno == EINTR)
			continue;
		return false;
	}
	return true;
}

static bool read_all(int fd, uint8_t *buf, size_t len)
{
	size_t off = 0;
	while (off < len) {
		ssize_t n = read(fd, buf + off, len - off);
		if (n > 0) {
			off += (size_t)n;
			continue;
		}
		if (n < 0 && errno == EINTR)
			continue;
		return false;
	}
	return true;
}

static bool sway_send_msg(int fd, uint32_t msg_type, const char *payload)
{
	size_t plen = payload ? strlen(payload) : 0U;
	uint8_t hdr[14];
	memcpy(hdr, SWAY_IPC_MAGIC, SWAY_IPC_MAGIC_LEN);
	uint32_t len32 = (uint32_t)plen;
	memcpy(hdr + 6, &len32, 4);
	memcpy(hdr + 10, &msg_type, 4);

	if (!write_all(fd, hdr, sizeof(hdr)))
		return false;
	if (plen > 0 && !write_all(fd, (const uint8_t *)payload, plen))
		return false;
	return true;
}

static void refresh_sway_tree_initial(host_snapshot_t *snap)
{
	const char *swaysock = getenv("SWAYSOCK");
	int fd = sway_ipc_connect(swaysock);
	if (fd < 0)
		return;

	if (sway_send_msg(fd, SWAY_IPC_GET_TREE, NULL)) {
		uint8_t hdr[14];
		if (read_all(fd, hdr, sizeof(hdr)) &&
		    memcmp(hdr, SWAY_IPC_MAGIC, SWAY_IPC_MAGIC_LEN) == 0) {
			uint32_t plen = 0;
			memcpy(&plen, hdr + 6, 4);
			if (plen > 0 && plen < 1048576U) {
				char *buf = malloc(plen + 1);
				if (buf) {
					if (read_all(fd, (uint8_t *)buf,
						     plen)) {
						buf[plen] = '\0';
						char app[64];
						char title[128];
						bool fs = false;
						if (bridge_parse_sway_tree(
							    buf, app,
							    sizeof(app), title,
							    sizeof(title),
							    &fs)) {
							snprintf(
								snap->focused_app,
								sizeof(snap->focused_app),
								"%s", app);
							snprintf(
								snap->focused_title,
								sizeof(snap->focused_title),
								"%s", title);
							snap->focused_fullscreen =
								fs;
						}
					}
					free(buf);
				}
			}
		}
	}
	close(fd);
}

static int connect_sway_subscription(void)
{
	const char *swaysock = getenv("SWAYSOCK");
	int fd = sway_ipc_connect(swaysock);
	if (fd < 0)
		return -1;

	if (!sway_send_msg(fd, SWAY_IPC_SUBSCRIBE, "[\"window\"]")) {
		close(fd);
		return -1;
	}
	set_nonblocking(fd);
	return fd;
}

static int spawn_line_watcher(const char *const argv[], pid_t *out_pid)
{
	int pipefd[2];
	if (pipe(pipefd) != 0)
		return -1;

	pid_t pid = fork();
	if (pid < 0) {
		close(pipefd[0]);
		close(pipefd[1]);
		return -1;
	}
	if (pid == 0) {
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		int devnull = open("/dev/null", O_RDWR);
		if (devnull >= 0) {
			dup2(devnull, STDIN_FILENO);
			dup2(devnull, STDERR_FILENO);
			close(devnull);
		}
		execvp(argv[0], (char *const *)argv);
		_exit(127);
	}

	close(pipefd[1]);
	set_nonblocking(pipefd[0]);
	*out_pid = pid;
	return pipefd[0];
}

static bool run_argv_capture(const char *const argv[], char *out,
			     size_t out_size)
{
	if (!argv || !argv[0] || !out || out_size == 0)
		return false;
	out[0] = '\0';

	int pipefd[2];
	if (pipe(pipefd) != 0)
		return false;

	pid_t pid = fork();
	if (pid < 0) {
		close(pipefd[0]);
		close(pipefd[1]);
		return false;
	}
	if (pid == 0) {
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		int devnull = open("/dev/null", O_RDWR);
		if (devnull >= 0) {
			dup2(devnull, STDIN_FILENO);
			dup2(devnull, STDERR_FILENO);
			close(devnull);
		}
		execvp(argv[0], (char *const *)argv);
		_exit(127);
	}

	close(pipefd[1]);
	size_t total = 0;
	while (total + 1 < out_size) {
		ssize_t n = read(pipefd[0], out + total, out_size - 1 - total);
		if (n > 0) {
			total += (size_t)n;
			continue;
		}
		if (n < 0 && errno == EINTR)
			continue;
		break;
	}
	close(pipefd[0]);
	out[total] = '\0';

	int status = 0;
	while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
	}
	return WIFEXITED(status) && WEXITSTATUS(status) == 0 && total > 0;
}

static void query_mpris_player(const char *dest, bool *playing, char *artist,
			       size_t artist_size, char *title,
			       size_t title_size)
{
	const char *const argv[] = { "gdbus",
				     "call",
				     "--session",
				     "--dest",
				     dest,
				     "--object-path",
				     "/org/mpris/MediaPlayer2",
				     "--method",
				     "org.freedesktop.DBus.Properties.GetAll",
				     "org.mpris.MediaPlayer2.Player",
				     NULL };
	char reply[4096];
	if (run_argv_capture(argv, reply, sizeof(reply))) {
		bridge_parse_gdbus_mpris(reply, playing, artist, artist_size,
					 title, title_size);
	}
}

static void refresh_mpris(host_snapshot_t *snap)
{
	const char *const list_argv[] = { "gdbus",
					  "call",
					  "--session",
					  "--dest",
					  "org.freedesktop.DBus",
					  "--object-path",
					  "/org/freedesktop/DBus",
					  "--method",
					  "org.freedesktop.DBus.ListNames",
					  NULL };
	char names_buf[4096];
	if (!run_argv_capture(list_argv, names_buf, sizeof(names_buf)))
		return;

	char spotify_dest[128] = { 0 };
	char firefox_dest[128] = { 0 };

	const char *cur = names_buf;
	while ((cur = strstr(cur, "org.mpris.MediaPlayer2.")) != NULL) {
		char dest[128] = { 0 };
		size_t i = 0;
		while (cur[i] != '\0' && cur[i] != '\'' && cur[i] != '"' &&
		       i + 1 < sizeof(dest)) {
			dest[i] = cur[i];
			i++;
		}
		dest[i] = '\0';
		if (ci_contains(dest, "spotify") && spotify_dest[0] == '\0') {
			snprintf(spotify_dest, sizeof(spotify_dest), "%s",
				 dest);
		} else if (ci_contains(dest, "firefox") &&
			   firefox_dest[0] == '\0') {
			snprintf(firefox_dest, sizeof(firefox_dest), "%s",
				 dest);
		}
		cur += i;
	}

	snap->spotify_playing = false;
	snap->spotify_artist[0] = '\0';
	snap->spotify_title[0] = '\0';
	if (spotify_dest[0] != '\0') {
		query_mpris_player(spotify_dest, &snap->spotify_playing,
				   snap->spotify_artist,
				   sizeof(snap->spotify_artist),
				   snap->spotify_title,
				   sizeof(snap->spotify_title));
	}

	snap->firefox_mpris_playing = false;
	snap->firefox_mpris_title[0] = '\0';
	if (firefox_dest[0] != '\0') {
		char unused_artist[64];
		query_mpris_player(firefox_dest, &snap->firefox_mpris_playing,
				   unused_artist, sizeof(unused_artist),
				   snap->firefox_mpris_title,
				   sizeof(snap->firefox_mpris_title));
	}
}

static void refresh_audio(host_snapshot_t *snap)
{
	static char pw_buf[131072];
	const char *const pw_argv[] = { "pw-dump", NULL };
	if (run_argv_capture(pw_argv, pw_buf, sizeof(pw_buf))) {
		bridge_parse_pw_dump(pw_buf, &snap->vesktop_voice_active);
	} else {
		snap->vesktop_voice_active = false;
	}

	char mute_buf[128];
	const char *const src_mute_argv[] = { "pactl", "get-source-mute",
					      "@DEFAULT_SOURCE@", NULL };
	if (run_argv_capture(src_mute_argv, mute_buf, sizeof(mute_buf))) {
		bridge_parse_pactl_mute(mute_buf, &snap->mic_muted);
	}

	bool sink_muted = false;
	const char *const sink_mute_argv[] = { "pactl", "get-sink-mute",
					       "@DEFAULT_SINK@", NULL };
	if (snap->vesktop_voice_active &&
	    run_argv_capture(sink_mute_argv, mute_buf, sizeof(mute_buf))) {
		bridge_parse_pactl_mute(mute_buf, &sink_muted);
	}
	snap->deafened = snap->vesktop_voice_active && sink_muted;
}

int bridge_sway_consume_frame(uint8_t *buf, size_t *len, uint32_t *out_type,
			      char *out_payload, size_t payload_cap)
{
	if (!buf || !len || !out_type || !out_payload || payload_cap == 0)
		return -1;
	if (*len < 14U)
		return 0;

	if (memcmp(buf, SWAY_IPC_MAGIC, SWAY_IPC_MAGIC_LEN) != 0)
		return -1;

	uint32_t plen = 0;
	uint32_t mtype = 0;
	memcpy(&plen, buf + 6, 4);
	memcpy(&mtype, buf + 10, 4);
	if (plen > 65536U)
		return -1;
	if (*len < 14U + plen)
		return 0;

	size_t copy_len = plen < (payload_cap - 1) ? plen : (payload_cap - 1);
	if (copy_len > 0)
		memcpy(out_payload, buf + 14U, copy_len);
	out_payload[copy_len] = '\0';
	*out_type = mtype;

	size_t remain = *len - (14U + plen);
	if (remain > 0)
		memmove(buf, buf + 14U + plen, remain);
	*len = remain;
	return 1;
}

void bridge_monitors_init(bridge_monitors_t *m, host_snapshot_t *snap)
{
	if (!m)
		return;
	memset(m, 0, sizeof(*m));
	m->sway_fd = -1;
	m->dbus_fd = -1;
	m->dbus_pid = -1;
	m->pactl_fd = -1;
	m->pactl_pid = -1;

	if (snap) {
		refresh_sway_tree_initial(snap);
		refresh_mpris(snap);
		refresh_audio(snap);
	}

	m->sway_fd = connect_sway_subscription();
	m->need_sway_reconnect = (m->sway_fd < 0);

	const char *const dbus_argv[] = {
		"dbus-monitor", "--session",
		"type='signal',interface='org.freedesktop.DBus.Properties',"
		"member='PropertiesChanged',path='/org/mpris/MediaPlayer2'",
		"type='signal',interface='org.freedesktop.DBus',member='NameOwnerChanged'",
		NULL
	};
	m->dbus_fd = spawn_line_watcher(dbus_argv, &m->dbus_pid);

	const char *const pactl_argv[] = { "pactl", "subscribe", NULL };
	m->pactl_fd = spawn_line_watcher(pactl_argv, &m->pactl_pid);
}

static void kill_child_watcher(int *fd, pid_t *pid)
{
	if (*fd >= 0) {
		close(*fd);
		*fd = -1;
	}
	if (*pid > 0) {
		kill(*pid, SIGTERM);
		waitpid(*pid, NULL, 0);
		*pid = -1;
	}
}

void bridge_monitors_close(bridge_monitors_t *m)
{
	if (!m)
		return;
	if (m->sway_fd >= 0) {
		close(m->sway_fd);
		m->sway_fd = -1;
	}
	kill_child_watcher(&m->dbus_fd, &m->dbus_pid);
	kill_child_watcher(&m->pactl_fd, &m->pactl_pid);
}

static void drain_sway_fd(bridge_monitors_t *m, host_snapshot_t *snap,
			  bool *out_activity)
{
	while (m->sway_rx_len < sizeof(m->sway_rx)) {
		ssize_t n = read(m->sway_fd, m->sway_rx + m->sway_rx_len,
				 sizeof(m->sway_rx) - m->sway_rx_len);
		if (n > 0) {
			m->sway_rx_len += (size_t)n;
			continue;
		}
		if (n == 0 || (n < 0 && errno != EAGAIN &&
			       errno != EWOULDBLOCK && errno != EINTR)) {
			close(m->sway_fd);
			m->sway_fd = -1;
			m->sway_rx_len = 0;
			m->need_sway_reconnect = true;
			break;
		}
		break;
	}

	char payload[16384];
	uint32_t mtype = 0;
	int rc;
	while ((rc = bridge_sway_consume_frame(m->sway_rx, &m->sway_rx_len,
					       &mtype, payload,
					       sizeof(payload))) == 1) {
		if (mtype == SWAY_IPC_EVENT_WINDOW) {
			char app[64];
			char title[128];
			bool fs = false;
			if (bridge_parse_sway_event(payload, app, sizeof(app),
						    title, sizeof(title),
						    &fs)) {
				snprintf(snap->focused_app,
					 sizeof(snap->focused_app), "%s", app);
				snprintf(snap->focused_title,
					 sizeof(snap->focused_title), "%s",
					 title);
				snap->focused_fullscreen = fs;
				if (out_activity)
					*out_activity = true;
			}
		}
	}
	if (rc < 0) {
		close(m->sway_fd);
		m->sway_fd = -1;
		m->sway_rx_len = 0;
		m->need_sway_reconnect = true;
	}
}

static void drain_line_pipe(int *fd, pid_t *pid, char *buf, size_t cap,
			    size_t *len, const char *keyword, bool *flag)
{
	for (;;) {
		ssize_t n = read(*fd, buf + *len, cap - 1 - *len);
		if (n > 0) {
			*len += (size_t)n;
			buf[*len] = '\0';
			if (!keyword || ci_contains(buf, keyword))
				*flag = true;
			char *nl = strrchr(buf, '\n');
			if (nl) {
				size_t tail = strlen(nl + 1);
				memmove(buf, nl + 1, tail + 1);
				*len = tail;
			} else if (*len >= cap - 2) {
				*len = 0;
				buf[0] = '\0';
			}
			continue;
		}
		if (n == 0 || (n < 0 && errno != EAGAIN &&
			       errno != EWOULDBLOCK && errno != EINTR)) {
			kill_child_watcher(fd, pid);
		}
		break;
	}
}

void bridge_monitors_process_events(bridge_monitors_t *m, host_snapshot_t *snap,
				    uint64_t now_ms, bool sway_readable,
				    bool dbus_readable, bool pactl_readable,
				    bool *out_activity)
{
	if (!m || !snap)
		return;

	if (m->need_sway_reconnect && now_ms >= m->next_reconnect_ms) {
		m->sway_fd = connect_sway_subscription();
		m->need_sway_reconnect = (m->sway_fd < 0);
		m->next_reconnect_ms = now_ms + 3000ULL;
		if (m->sway_fd >= 0)
			refresh_sway_tree_initial(snap);
	}

	if (sway_readable && m->sway_fd >= 0)
		drain_sway_fd(m, snap, out_activity);

	if (dbus_readable && m->dbus_fd >= 0) {
		drain_line_pipe(&m->dbus_fd, &m->dbus_pid, m->dbus_rx,
				sizeof(m->dbus_rx), &m->dbus_rx_len, "mpris",
				&m->need_mpris_refresh);
	}

	if (pactl_readable && m->pactl_fd >= 0) {
		drain_line_pipe(&m->pactl_fd, &m->pactl_pid, m->pactl_rx,
				sizeof(m->pactl_rx), &m->pactl_rx_len, NULL,
				&m->need_audio_refresh);
	}

	if (m->need_mpris_refresh ||
	    (now_ms >= m->last_mpris_poll_ms &&
	     (now_ms - m->last_mpris_poll_ms) >= 5000ULL)) {
		bool prev_ff = snap->firefox_mpris_playing;
		bool prev_sp = snap->spotify_playing;
		refresh_mpris(snap);
		m->need_mpris_refresh = false;
		m->last_mpris_poll_ms = now_ms;
		if ((!prev_ff && snap->firefox_mpris_playing) ||
		    (!prev_sp && snap->spotify_playing)) {
			if (out_activity)
				*out_activity = true;
		}
	}

	if (m->need_audio_refresh ||
	    (now_ms >= m->last_audio_poll_ms &&
	     (now_ms - m->last_audio_poll_ms) >= 5000ULL)) {
		bool prev_voice = snap->vesktop_voice_active;
		refresh_audio(snap);
		m->need_audio_refresh = false;
		m->last_audio_poll_ms = now_ms;
		if (!prev_voice && snap->vesktop_voice_active && out_activity)
			*out_activity = true;
	}
}
