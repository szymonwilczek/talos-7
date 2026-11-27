#include "bridge_serial.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

static bool read_trimmed_line(const char *path, char *buf, size_t buf_size)
{
	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return false;

	ssize_t n = read(fd, buf, buf_size - 1);
	close(fd);
	if (n <= 0)
		return false;

	buf[n] = '\0';
	while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r' ||
			 buf[n - 1] == ' ' || buf[n - 1] == '\t')) {
		buf[--n] = '\0';
	}
	return n > 0;
}

static bool match_usb_ids(const char *dev_dir, const char *vid, const char *pid)
{
	char resolved[PATH_MAX];
	if (!realpath(dev_dir, resolved))
		return false;

	char cur[PATH_MAX];
	snprintf(cur, sizeof(cur), "%s", resolved);

	for (int depth = 0; depth < 6; depth++) {
		char id_path[PATH_MAX];
		char id_val[32];

		if ((size_t)snprintf(id_path, sizeof(id_path), "%s/idVendor",
				     cur) < sizeof(id_path) &&
		    read_trimmed_line(id_path, id_val, sizeof(id_val)) &&
		    strcmp(id_val, vid) == 0) {
			if ((size_t)snprintf(id_path, sizeof(id_path),
					     "%s/idProduct",
					     cur) < sizeof(id_path) &&
			    read_trimmed_line(id_path, id_val,
					      sizeof(id_val)) &&
			    strcmp(id_val, pid) == 0) {
				return true;
			}
		}

		char *slash = strrchr(cur, '/');
		if (!slash || slash == cur)
			break;
		*slash = '\0';
	}
	return false;
}

int bridge_find_tty_in_sysfs(const char *sysfs_tty_root, const char *dev_prefix,
			     const char *vid, const char *pid, char *out_path,
			     size_t out_size)
{
	if (!sysfs_tty_root || !dev_prefix || !vid || !pid || !out_path ||
	    out_size == 0) {
		return -1;
	}

	DIR *dir = opendir(sysfs_tty_root);
	if (!dir)
		return -1;

	char best_name[256] = { 0 };
	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL) {
		if (strncmp(ent->d_name, "ttyACM", 6) != 0)
			continue;

		char dev_link[PATH_MAX];
		int n = snprintf(dev_link, sizeof(dev_link), "%s/%s/device",
				 sysfs_tty_root, ent->d_name);
		if (n < 0 || (size_t)n >= sizeof(dev_link))
			continue;

		if (!match_usb_ids(dev_link, vid, pid))
			continue;

		if (best_name[0] == '\0' ||
		    strcmp(ent->d_name, best_name) < 0) {
			snprintf(best_name, sizeof(best_name), "%s",
				 ent->d_name);
		}
	}
	closedir(dir);

	if (best_name[0] == '\0')
		return -1;

	int written =
		snprintf(out_path, out_size, "%s/%s", dev_prefix, best_name);
	if (written < 0 || (size_t)written >= out_size)
		return -1;
	return 0;
}

static int posix_find_device(const char *vid, const char *pid, char *out_path,
			     size_t out_size, void *ctx)
{
	(void)ctx;
	return bridge_find_tty_in_sysfs("/sys/class/tty", "/dev", vid, pid,
					out_path, out_size);
}

static int posix_open_port(const char *path, void *ctx)
{
	(void)ctx;
	/* Strictly one-way: write-only so bridge never consumes or monitors USB RX */
	int fd = open(path, O_WRONLY | O_NOCTTY | O_NONBLOCK);
	if (fd < 0)
		return -1;

	struct termios tio;
	if (tcgetattr(fd, &tio) == 0) {
		tio.c_iflag &= ~(unsigned)(IGNBRK | BRKINT | PARMRK | ISTRIP |
					   INLCR | IGNCR | ICRNL | IXON);
		tio.c_oflag &= ~(unsigned)OPOST;
		tio.c_lflag &=
			~(unsigned)(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
		tio.c_cflag &= ~(unsigned)(CSIZE | PARENB);
		tio.c_cflag |= (unsigned)(CS8 | CLOCAL);
		cfsetospeed(&tio, B115200);
		tcsetattr(fd, TCSANOW, &tio);
	}
	return fd;
}

static int posix_write_port(int fd, const char *data, size_t len, void *ctx)
{
	(void)ctx;
	size_t off = 0;
	while (off < len) {
		ssize_t n = write(fd, data + off, len - off);
		if (n > 0) {
			off += (size_t)n;
			continue;
		}
		if (n < 0 && errno == EINTR)
			continue;
		if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
			break;
		return -1;
	}
	return (off == len) ? 0 : -1;
}

static void posix_close_port(int fd, void *ctx)
{
	(void)ctx;
	if (fd >= 0)
		close(fd);
}

static const char *ctx_to_token(bridge_ctx_t ctx)
{
	switch (ctx) {
	case BRIDGE_CTX_EMACS:
		return "EMACS";
	case BRIDGE_CTX_TERM:
		return "TERM";
	case BRIDGE_CTX_AGENT_THINKING:
		return "AGENT_THINKING";
	case BRIDGE_CTX_VESKTOP:
		return "VESKTOP";
	case BRIDGE_CTX_SPOTIFY:
		return "SPOTIFY";
	case BRIDGE_CTX_KINO:
		return "KINO";
	case BRIDGE_CTX_GAMING:
		return "GAMING";
	case BRIDGE_CTX_IDLE:
	default:
		return "IDLE";
	}
}

void bridge_sanitize_field(const char *src, char *dst, size_t dst_size,
			   size_t max_chars)
{
	if (!dst || dst_size == 0)
		return;
	if (!src) {
		dst[0] = '\0';
		return;
	}

	size_t cap = dst_size - 1;
	if (max_chars < cap)
		cap = max_chars;

	size_t out = 0;
	const unsigned char *p = (const unsigned char *)src;
	while (*p != '\0' && out < cap) {
		unsigned char ch = *p;
		if ((ch & 0x80U) != 0) {
			/* Collapse a multi-byte UTF-8 codepoint into one '?' */
			dst[out++] = '?';
			p++;
			while ((*p & 0xC0U) == 0x80U)
				p++;
			continue;
		}

		if (ch == '|') {
			dst[out++] = '/';
		} else if (ch == '\r' || ch == '\n' || ch == '\t') {
			if (out > 0 && dst[out - 1] != ' ')
				dst[out++] = ' ';
		} else if (ch >= 0x20U && ch <= 0x7EU) {
			dst[out++] = (char)ch;
		}
		p++;
	}

	while (out > 0 && dst[out - 1] == ' ')
		out--;
	dst[out] = '\0';
}

void bridge_serial_init(bridge_serial_t *serial, const char *device_override,
			const bridge_serial_ops_t *ops)
{
	if (!serial)
		return;
	memset(serial, 0, sizeof(*serial));
	serial->fd = -1;

	if (device_override && device_override[0] != '\0') {
		snprintf(serial->device_override,
			 sizeof(serial->device_override), "%s",
			 device_override);
	}

	if (ops) {
		serial->ops = *ops;
	} else {
		serial->ops.find_device = posix_find_device;
		serial->ops.open_port = posix_open_port;
		serial->ops.write_port = posix_write_port;
		serial->ops.close_port = posix_close_port;
		serial->ops.ctx = NULL;
	}
}

void bridge_serial_reset_cache(bridge_serial_t *serial)
{
	if (!serial)
		return;
	serial->has_last_state = false;
	serial->has_last_time = false;
	serial->has_typing_sent = false;
}

void bridge_serial_close(bridge_serial_t *serial)
{
	if (!serial)
		return;
	if (serial->fd >= 0 && serial->ops.close_port) {
		serial->ops.close_port(serial->fd, serial->ops.ctx);
	}
	serial->fd = -1;
	serial->active_path[0] = '\0';
	bridge_serial_reset_cache(serial);
}

void bridge_serial_set_paused(bridge_serial_t *serial, bool paused)
{
	if (!serial || serial->paused == paused)
		return;
	serial->paused = paused;
	if (paused) {
		bridge_serial_close(serial);
	} else {
		bridge_serial_reset_cache(serial);
	}
}

bool bridge_serial_ensure_open(bridge_serial_t *serial)
{
	if (!serial || serial->paused)
		return false;
	if (serial->fd >= 0)
		return true;

	char path[128] = { 0 };
	if (serial->device_override[0] != '\0') {
		snprintf(path, sizeof(path), "%s", serial->device_override);
	} else if (!serial->ops.find_device ||
		   serial->ops.find_device(TALOS_USB_VID, TALOS_USB_PID, path,
					   sizeof(path),
					   serial->ops.ctx) != 0) {
		return false;
	}

	if (!serial->ops.open_port)
		return false;
	int fd = serial->ops.open_port(path, serial->ops.ctx);
	if (fd < 0)
		return false;

	serial->fd = fd;
	snprintf(serial->active_path, sizeof(serial->active_path), "%s", path);
	bridge_serial_reset_cache(serial);
	return true;
}

static bool write_line(bridge_serial_t *serial, const char *line)
{
	if (!bridge_serial_ensure_open(serial) || !serial->ops.write_port)
		return false;

	size_t len = strlen(line);
	if (serial->ops.write_port(serial->fd, line, len, serial->ops.ctx) !=
	    0) {
		bridge_serial_close(serial);
		return false;
	}
	return true;
}

bool bridge_serial_send_state(bridge_serial_t *serial, bridge_ctx_t ctx,
			      uint32_t flags, const char *top_text,
			      const char *sub_text, uint64_t now_ms)
{
	if (!serial || serial->paused)
		return false;

	char clean_top[BRIDGE_TEXT_MAX + 1];
	char clean_sub[BRIDGE_SUB_MAX + 1];
	bridge_sanitize_field(top_text, clean_top, sizeof(clean_top),
			      BRIDGE_TEXT_MAX);
	bridge_sanitize_field(sub_text, clean_sub, sizeof(clean_sub),
			      BRIDGE_SUB_MAX);

	uint32_t base_flags = flags & ~(uint32_t)BRIDGE_FLAG_TYPING_PULSE;
	bool wants_typing = (flags & BRIDGE_FLAG_TYPING_PULSE) != 0;

	bool base_changed = !serial->has_last_state ||
			    serial->last_ctx != ctx ||
			    serial->last_base_flags != base_flags ||
			    strcmp(serial->last_top, clean_top) != 0 ||
			    strcmp(serial->last_sub, clean_sub) != 0;

	bool emit_typing = false;
	if (wants_typing) {
		if (!serial->has_typing_sent ||
		    (now_ms >= serial->last_typing_ms &&
		     (now_ms - serial->last_typing_ms) >=
			     BRIDGE_TYPING_COOLDOWN_MS)) {
			emit_typing = true;
		}
	}

	if (!base_changed && !emit_typing)
		return true;

	uint32_t tx_flags = base_flags |
			    (emit_typing ? BRIDGE_FLAG_TYPING_PULSE : 0U);
	char frame[192];
	snprintf(frame, sizeof(frame), "COMP_STATE|%s|%u|%s|%s\n",
		 ctx_to_token(ctx), (unsigned)tx_flags, clean_top, clean_sub);

	if (!write_line(serial, frame))
		return false;

	serial->has_last_state = true;
	serial->last_ctx = ctx;
	serial->last_base_flags = base_flags;
	snprintf(serial->last_top, sizeof(serial->last_top), "%s", clean_top);
	snprintf(serial->last_sub, sizeof(serial->last_sub), "%s", clean_sub);
	if (emit_typing) {
		serial->has_typing_sent = true;
		serial->last_typing_ms = now_ms;
	}
	return true;
}

bool bridge_serial_send_event(bridge_serial_t *serial, const char *evt_name,
			      uint32_t duration_ms, const char *label)
{
	if (!serial || serial->paused || !evt_name || evt_name[0] == '\0')
		return false;

	char clean_evt[24];
	char clean_label[BRIDGE_SUB_MAX + 1];
	bridge_sanitize_field(evt_name, clean_evt, sizeof(clean_evt), 23);
	bridge_sanitize_field(label, clean_label, sizeof(clean_label),
			      BRIDGE_SUB_MAX);

	char frame[128];
	snprintf(frame, sizeof(frame), "COMP_EVT|%s|%u|%s\n", clean_evt,
		 (unsigned)duration_ms, clean_label);
	return write_line(serial, frame);
}

bool bridge_serial_send_time(bridge_serial_t *serial, uint8_t hour,
			     uint8_t minute)
{
	if (!serial || serial->paused)
		return false;
	if (hour > 23 || minute > 59)
		return false;

	if (serial->has_last_time && serial->last_hour == hour &&
	    serial->last_minute == minute) {
		return true;
	}

	char frame[48];
	snprintf(frame, sizeof(frame), "COMP_TIME|%u|%u\n", (unsigned)hour,
		 (unsigned)minute);
	if (!write_line(serial, frame))
		return false;

	serial->has_last_time = true;
	serial->last_hour = hour;
	serial->last_minute = minute;
	return true;
}

bool bridge_serial_send_command(bridge_serial_t *serial, const char *cmd)
{
	if (!serial || serial->paused || !cmd || cmd[0] == '\0')
		return false;

	char frame[96];
	char clean_cmd[80];
	bridge_sanitize_field(cmd, clean_cmd, sizeof(clean_cmd), 79);
	snprintf(frame, sizeof(frame), "%s\n", clean_cmd);
	return write_line(serial, frame);
}
