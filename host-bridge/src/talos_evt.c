#include "bridge_socket.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static void print_usage(const char *argv0)
{
	printf("Usage: %s [--socket PATH] [--quiet] <event> [arg...]\n"
	       "Events: agent-start, agent-done, agent-err, agent-input,\n"
	       "        git-commit, emacs-buf, typing, ping, pet, coffee,\n"
	       "        pause, resume, idle-30m, idle-resume\n",
	       argv0);
}

static bool is_valid_event(const char *evt)
{
	static const char *const EVENTS[] = {
		"agent-start", "agent-done", "agent-err", "agent-input",
		"git-commit",  "emacs-buf",  "typing",	  "ping",
		"pet",	       "coffee",     "pause",	  "resume",
		"idle-30m",    "idle-resume"
	};
	for (size_t i = 0; i < sizeof(EVENTS) / sizeof(EVENTS[0]); i++) {
		if (strcmp(evt, EVENTS[i]) == 0)
			return true;
	}
	return false;
}

static int build_payload(int arg_idx, int argc, char **argv, char *out,
			 size_t out_size)
{
	const char *evt = argv[arg_idx];
	if (!is_valid_event(evt))
		return -1;

	if (arg_idx + 1 >= argc) {
		snprintf(out, out_size, "%s\n", evt);
		return 0;
	}

	if (strcmp(evt, "emacs-buf") == 0) {
		const char *buf = argv[arg_idx + 1];
		const char *mode = (arg_idx + 2 < argc) ? argv[arg_idx + 2] :
							  "";
		snprintf(out, out_size, "%s|%s|%s\n", evt, buf, mode);
		return 0;
	}

	char joined[192] = { 0 };
	size_t off = 0;
	for (int i = arg_idx + 1; i < argc && off + 1 < sizeof(joined); i++) {
		if (i > arg_idx + 1 && off + 1 < sizeof(joined))
			joined[off++] = ' ';
		size_t alen = strlen(argv[i]);
		if (alen > sizeof(joined) - 1 - off)
			alen = sizeof(joined) - 1 - off;
		memcpy(joined + off, argv[i], alen);
		off += alen;
	}
	joined[off] = '\0';
	snprintf(out, out_size, "%s|%s\n", evt, joined);
	return 0;
}

int main(int argc, char **argv)
{
	const char *sock_override = NULL;
	bool quiet = false;
	int i = 1;

	while (i < argc) {
		if (strcmp(argv[i], "--help") == 0 ||
		    strcmp(argv[i], "-h") == 0) {
			print_usage(argv[0]);
			return 0;
		}
		if (strcmp(argv[i], "--quiet") == 0 ||
		    strcmp(argv[i], "-q") == 0) {
			quiet = true;
			i++;
			continue;
		}
		if (strcmp(argv[i], "--socket") == 0 && i + 1 < argc) {
			sock_override = argv[i + 1];
			i += 2;
			continue;
		}
		break;
	}

	if (i >= argc) {
		print_usage(argv[0]);
		return 1;
	}

	char line[256];
	if (build_payload(i, argc, argv, line, sizeof(line)) != 0) {
		if (!quiet)
			fprintf(stderr, "Unknown event: %s\n", argv[i]);
		return 1;
	}

	char sock_path[108];
	if (sock_override && sock_override[0] != '\0') {
		if ((size_t)snprintf(sock_path, sizeof(sock_path), "%s",
				     sock_override) >= sizeof(sock_path)) {
			return 1;
		}
	} else if (!bridge_socket_default_path(sock_path, sizeof(sock_path))) {
		return 1;
	}

	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
		return quiet ? 0 : 1;

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sock_path);

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		if (!quiet) {
			fprintf(stderr,
				"talos-evt: cannot connect to %s (%s)\n",
				sock_path, strerror(errno));
		}
		close(fd);
		return quiet ? 0 : 1;
	}

	size_t len = strlen(line);
	size_t off = 0;
	while (off < len) {
		ssize_t n = write(fd, line + off, len - off);
		if (n > 0) {
			off += (size_t)n;
			continue;
		}
		if (n < 0 && errno == EINTR)
			continue;
		close(fd);
		return quiet ? 0 : 1;
	}

	close(fd);
	return 0;
}
