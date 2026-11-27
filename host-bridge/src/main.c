#include "bridge_daemon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t monotonic_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000ULL +
	       (uint64_t)(ts.tv_nsec / 1000000L);
}

static void print_usage(const char *argv0)
{
	printf("Usage: %s [--device /dev/ttyACM0] [--idle-sec 1800]\n", argv0);
}

int main(int argc, char **argv)
{
	const char *device = NULL;
	uint32_t idle_sec = BRIDGE_IDLE_TIMEOUT_SEC;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0 ||
		    strcmp(argv[i], "-h") == 0) {
			print_usage(argv[0]);
			return 0;
		}
		if (strcmp(argv[i], "--device") == 0 && i + 1 < argc) {
			device = argv[++i];
			continue;
		}
		if (strcmp(argv[i], "--idle-sec") == 0 && i + 1 < argc) {
			long v = strtol(argv[++i], NULL, 10);
			if (v > 0)
				idle_sec = (uint32_t)v;
			continue;
		}
		fprintf(stderr, "Unknown argument: %s\n", argv[i]);
		print_usage(argv[0]);
		return 1;
	}

	bridge_daemon_t daemon;
	bridge_daemon_init(&daemon, device, idle_sec, NULL, monotonic_ms());
	return bridge_daemon_run(&daemon);
}
