#ifndef BRIDGE_SOCKET_H
#define BRIDGE_SOCKET_H

#include <poll.h>
#include <stdbool.h>
#include <stddef.h>

#define BRIDGE_SOCK_MAX_CLIENTS 16
#define BRIDGE_SOCK_LINE_MAX 256

typedef void (*bridge_socket_line_fn)(const char *line, void *ctx);

typedef struct {
	int fd;
	char rx[BRIDGE_SOCK_LINE_MAX];
	size_t rx_len;
} bridge_socket_client_t;

typedef struct {
	int listen_fd;
	char path[108];
	bridge_socket_client_t clients[BRIDGE_SOCK_MAX_CLIENTS];
} bridge_socket_server_t;

/**
 * Compute default runtime socket path ($TALOS_BRIDGE_SOCK, $XDG_RUNTIME_DIR, or
 * /tmp fallback).
 */
bool bridge_socket_default_path(char *out_path, size_t out_size);

/**
 * Bind and listen on a non-blocking Unix domain stream socket.
 */
bool bridge_socket_open(bridge_socket_server_t *srv, const char *path_override);

/**
 * Append active server + client pollfds into pfds[cap]. Returns count added.
 */
nfds_t bridge_socket_fill_pollfds(const bridge_socket_server_t *srv,
				  struct pollfd *pfds, nfds_t cap);

/**
 * Accept new connections and drain complete lines into on_line callback.
 */
void bridge_socket_service(bridge_socket_server_t *srv,
			   bridge_socket_line_fn on_line, void *ctx);

/**
 * Close listener/clients cleanly and unlink the Unix socket file.
 */
void bridge_socket_close(bridge_socket_server_t *srv);

#endif /* BRIDGE_SOCKET_H */
