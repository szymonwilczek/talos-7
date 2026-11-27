#include "bridge_socket.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

static bool set_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
		return false;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

bool bridge_socket_default_path(char *out_path, size_t out_size)
{
	if (!out_path || out_size == 0)
		return false;

	const char *env_sock = getenv("TALOS_BRIDGE_SOCK");
	if (env_sock && env_sock[0] != '\0') {
		return (size_t)snprintf(out_path, out_size, "%s", env_sock) <
		       out_size;
	}

	const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
	if (runtime_dir && runtime_dir[0] != '\0') {
		return (size_t)snprintf(out_path, out_size,
					"%s/talos-bridge.sock",
					runtime_dir) < out_size;
	}

	return (size_t)snprintf(out_path, out_size, "/tmp/talos-bridge-%u.sock",
				(unsigned)getuid()) < out_size;
}

bool bridge_socket_open(bridge_socket_server_t *srv, const char *path_override)
{
	if (!srv)
		return false;
	memset(srv, 0, sizeof(*srv));
	srv->listen_fd = -1;
	for (size_t i = 0; i < BRIDGE_SOCK_MAX_CLIENTS; i++)
		srv->clients[i].fd = -1;

	if (path_override && path_override[0] != '\0') {
		if ((size_t)snprintf(srv->path, sizeof(srv->path), "%s",
				     path_override) >= sizeof(srv->path)) {
			return false;
		}
	} else if (!bridge_socket_default_path(srv->path, sizeof(srv->path))) {
		return false;
	}

	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
		return false;

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", srv->path);

	unlink(srv->path);
	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0 ||
	    listen(fd, 8) != 0) {
		close(fd);
		srv->path[0] = '\0';
		return false;
	}

	chmod(srv->path, 0600);
	set_nonblocking(fd);
	srv->listen_fd = fd;
	return true;
}

nfds_t bridge_socket_fill_pollfds(const bridge_socket_server_t *srv,
				  struct pollfd *pfds, nfds_t cap)
{
	if (!srv || !pfds || cap == 0 || srv->listen_fd < 0)
		return 0;

	nfds_t count = 0;
	pfds[count++] =
		(struct pollfd){ .fd = srv->listen_fd, .events = POLLIN };
	for (size_t i = 0; i < BRIDGE_SOCK_MAX_CLIENTS && count < cap; i++) {
		if (srv->clients[i].fd >= 0) {
			pfds[count++] =
				(struct pollfd){ .fd = srv->clients[i].fd,
						 .events = POLLIN };
		}
	}
	return count;
}

static void dispatch_line(char *line, size_t len, bridge_socket_line_fn on_line,
			  void *ctx)
{
	while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n'))
		line[--len] = '\0';
	if (len > 0 && on_line)
		on_line(line, ctx);
}

static void drop_client(bridge_socket_client_t *c,
			bridge_socket_line_fn on_line, void *ctx)
{
	if (c->rx_len > 0) {
		c->rx[c->rx_len] = '\0';
		dispatch_line(c->rx, c->rx_len, on_line, ctx);
		c->rx_len = 0;
	}
	close(c->fd);
	c->fd = -1;
}

static void service_client(bridge_socket_client_t *c,
			   bridge_socket_line_fn on_line, void *ctx)
{
	for (;;) {
		if (c->rx_len >= sizeof(c->rx) - 1) {
			c->rx[c->rx_len] = '\0';
			dispatch_line(c->rx, c->rx_len, on_line, ctx);
			c->rx_len = 0;
		}

		ssize_t n = read(c->fd, c->rx + c->rx_len,
				 sizeof(c->rx) - 1 - c->rx_len);
		if (n > 0) {
			c->rx_len += (size_t)n;
			c->rx[c->rx_len] = '\0';

			char *start = c->rx;
			char *nl;
			while ((nl = strchr(start, '\n')) != NULL) {
				*nl = '\0';
				dispatch_line(start, (size_t)(nl - start),
					      on_line, ctx);
				start = nl + 1;
			}
			size_t remain = strlen(start);
			if (start != c->rx && remain > 0)
				memmove(c->rx, start, remain);
			c->rx_len = remain;
			continue;
		}

		if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK &&
			       errno != EINTR)) {
			drop_client(c, on_line, ctx);
		}
		break;
	}
}

void bridge_socket_service(bridge_socket_server_t *srv,
			   bridge_socket_line_fn on_line, void *ctx)
{
	if (!srv || srv->listen_fd < 0)
		return;

	for (;;) {
		int cfd = accept(srv->listen_fd, NULL, NULL);
		if (cfd < 0)
			break;

		bool placed = false;
		for (size_t i = 0; i < BRIDGE_SOCK_MAX_CLIENTS; i++) {
			if (srv->clients[i].fd < 0) {
				set_nonblocking(cfd);
				srv->clients[i].fd = cfd;
				srv->clients[i].rx_len = 0;
				placed = true;
				break;
			}
		}
		if (!placed)
			close(cfd);
	}

	for (size_t i = 0; i < BRIDGE_SOCK_MAX_CLIENTS; i++) {
		if (srv->clients[i].fd >= 0) {
			service_client(&srv->clients[i], on_line, ctx);
		}
	}
}

void bridge_socket_close(bridge_socket_server_t *srv)
{
	if (!srv)
		return;
	for (size_t i = 0; i < BRIDGE_SOCK_MAX_CLIENTS; i++) {
		if (srv->clients[i].fd >= 0) {
			close(srv->clients[i].fd);
			srv->clients[i].fd = -1;
			srv->clients[i].rx_len = 0;
		}
	}
	if (srv->listen_fd >= 0) {
		close(srv->listen_fd);
		srv->listen_fd = -1;
	}
	if (srv->path[0] != '\0') {
		unlink(srv->path);
		srv->path[0] = '\0';
	}
}
