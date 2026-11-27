#include "bridge_serial.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
	bool present;
	bool fail_write;
	int open_count;
	int close_count;
	int write_count;
	char log[2048];
	size_t log_len;
} fake_port_t;

static int fake_find(const char *vid, const char *pid, char *out_path,
		     size_t out_size, void *raw_ctx)
{
	(void)vid;
	(void)pid;
	fake_port_t *ctx = (fake_port_t *)raw_ctx;
	if (!ctx->present)
		return -1;
	snprintf(out_path, out_size, "/dev/ttyACM99");
	return 0;
}

static int fake_open(const char *path, void *raw_ctx)
{
	(void)path;
	fake_port_t *ctx = (fake_port_t *)raw_ctx;
	ctx->open_count++;
	return 42;
}

static int fake_write(int fd, const char *data, size_t len, void *raw_ctx)
{
	(void)fd;
	fake_port_t *ctx = (fake_port_t *)raw_ctx;
	if (ctx->fail_write)
		return -1;
	ctx->write_count++;
	size_t space = sizeof(ctx->log) - 1 - ctx->log_len;
	size_t cp = len < space ? len : space;
	if (cp > 0) {
		memcpy(ctx->log + ctx->log_len, data, cp);
		ctx->log_len += cp;
		ctx->log[ctx->log_len] = '\0';
	}
	return 0;
}

static void fake_close(int fd, void *raw_ctx)
{
	(void)fd;
	fake_port_t *ctx = (fake_port_t *)raw_ctx;
	ctx->close_count++;
}

static bridge_serial_ops_t make_ops(fake_port_t *ctx)
{
	bridge_serial_ops_t ops = { .find_device = fake_find,
				    .open_port = fake_open,
				    .write_port = fake_write,
				    .close_port = fake_close,
				    .ctx = ctx };
	return ops;
}

static void test_sanitization_and_truncation(void)
{
	char out[64];
	bridge_sanitize_field("Hello|World\nŻółć\tTest", out, sizeof(out), 47);
	TEST_ASSERT_EQUAL_STRING("Hello/World ???? Test", out);

	/* Multi-byte UTF-8 collapses each codepoint to single '?' */
	bridge_sanitize_field("Łódź|Test", out, sizeof(out), 47);
	TEST_ASSERT_EQUAL_STRING("??d?/Test", out);

	bridge_sanitize_field("abcdefghijklmnopqrstuvwxyz0123456789", out,
			      sizeof(out), 10);
	TEST_ASSERT_EQUAL_STRING("abcdefghij", out);
}

static void test_diff_only_and_typing_cooldown(void)
{
	fake_port_t port;
	memset(&port, 0, sizeof(port));
	port.present = true;
	bridge_serial_ops_t ops = make_ops(&port);

	bridge_serial_t ser;
	bridge_serial_init(&ser, NULL, &ops);

	TEST_ASSERT_TRUE(bridge_serial_send_state(&ser, BRIDGE_CTX_EMACS, 0U,
						  "main.c", "c", 100));
	/* Exact repeat -> 0 extra writes */
	TEST_ASSERT_TRUE(bridge_serial_send_state(&ser, BRIDGE_CTX_EMACS, 0U,
						  "main.c", "c", 200));
	TEST_ASSERT_EQUAL(1, port.write_count);

	/* First typing pulse goes through immediately */
	TEST_ASSERT_TRUE(bridge_serial_send_state(&ser, BRIDGE_CTX_EMACS,
						  BRIDGE_FLAG_TYPING_PULSE,
						  "main.c", "c", 1000));
	TEST_ASSERT_EQUAL(2, port.write_count);

	/* Inside 2500ms cooldown -> suppressed without touching serial */
	TEST_ASSERT_TRUE(bridge_serial_send_state(&ser, BRIDGE_CTX_EMACS,
						  BRIDGE_FLAG_TYPING_PULSE,
						  "main.c", "c", 2500));
	TEST_ASSERT_EQUAL(2, port.write_count);

	/* Cooldown expired (>= 2500ms) -> emits pulse again */
	TEST_ASSERT_TRUE(bridge_serial_send_state(&ser, BRIDGE_CTX_EMACS,
						  BRIDGE_FLAG_TYPING_PULSE,
						  "main.c", "c", 3500));
	TEST_ASSERT_EQUAL(3, port.write_count);

	/* Time sync dedups identical hour/minute */
	TEST_ASSERT_TRUE(bridge_serial_send_time(&ser, 21, 5));
	TEST_ASSERT_TRUE(bridge_serial_send_time(&ser, 21, 5));
	TEST_ASSERT_TRUE(bridge_serial_send_time(&ser, 21, 6));
	TEST_ASSERT_EQUAL(5, port.write_count);
}

static void test_pause_resume_and_disconnect_recovery(void)
{
	fake_port_t port;
	memset(&port, 0, sizeof(port));
	port.present = true;
	bridge_serial_ops_t ops = make_ops(&port);

	bridge_serial_t ser;
	bridge_serial_init(&ser, NULL, &ops);

	TEST_ASSERT_TRUE(bridge_serial_send_state(&ser, BRIDGE_CTX_IDLE, 0U, "",
						  "", 100));
	TEST_ASSERT_EQUAL(1, port.open_count);

	/* Pausing closes FD immediately and blocks sends */
	bridge_serial_set_paused(&ser, true);
	TEST_ASSERT_EQUAL(1, port.close_count);
	TEST_ASSERT_FALSE(bridge_serial_send_state(&ser, BRIDGE_CTX_IDLE, 0U,
						   "", "", 200));

	/* Resume clears cache and reopens + resends identical state */
	bridge_serial_set_paused(&ser, false);
	TEST_ASSERT_TRUE(bridge_serial_send_state(&ser, BRIDGE_CTX_IDLE, 0U, "",
						  "", 300));
	TEST_ASSERT_EQUAL(2, port.open_count);
	TEST_ASSERT_EQUAL(2, port.write_count);

	/* Write error closes port so next write reconnects cleanly */
	port.fail_write = true;
	TEST_ASSERT_FALSE(bridge_serial_send_command(&ser, "COMP_WAKE"));
	TEST_ASSERT_EQUAL(2, port.close_count);

	port.fail_write = false;
	TEST_ASSERT_TRUE(bridge_serial_send_command(&ser, "COMP_WAKE"));
	TEST_ASSERT_EQUAL(3, port.open_count);
}

static void test_sysfs_discovery_matches_talos_vid_pid(void)
{
	char tmpl[] = "/tmp/talos_sysfs_XXXXXX";
	char *root = mkdtemp(tmpl);
	TEST_ASSERT_TRUE(root != NULL);

	char usb_wrong[128];
	char usb_talos[128];
	char tty_root[128];
	char acm0_dir[256];
	char acm1_dir[256];
	snprintf(usb_wrong, sizeof(usb_wrong), "%s/usb0", root);
	snprintf(usb_talos, sizeof(usb_talos), "%s/usb1", root);
	snprintf(tty_root, sizeof(tty_root), "%s/tty", root);
	snprintf(acm0_dir, sizeof(acm0_dir), "%s/ttyACM0", tty_root);
	snprintf(acm1_dir, sizeof(acm1_dir), "%s/ttyACM1", tty_root);

	mkdir(usb_wrong, 0755);
	mkdir(usb_talos, 0755);
	mkdir(tty_root, 0755);
	mkdir(acm0_dir, 0755);
	mkdir(acm1_dir, 0755);

	char path[320];
	FILE *f;
	snprintf(path, sizeof(path), "%s/idVendor", usb_wrong);
	f = fopen(path, "w");
	fprintf(f, "1234\n");
	fclose(f);
	snprintf(path, sizeof(path), "%s/idProduct", usb_wrong);
	f = fopen(path, "w");
	fprintf(f, "9999\n");
	fclose(f);

	snprintf(path, sizeof(path), "%s/idVendor", usb_talos);
	f = fopen(path, "w");
	fprintf(f, "2e8a\n");
	fclose(f);
	snprintf(path, sizeof(path), "%s/idProduct", usb_talos);
	f = fopen(path, "w");
	fprintf(f, "0032\n");
	fclose(f);

	char link0[320];
	char link1[320];
	snprintf(link0, sizeof(link0), "%s/device", acm0_dir);
	snprintf(link1, sizeof(link1), "%s/device", acm1_dir);
	symlink(usb_wrong, link0);
	symlink(usb_talos, link1);

	char dev_path[128] = { 0 };
	int rc = bridge_find_tty_in_sysfs(tty_root, "/dev", "2e8a", "0032",
					  dev_path, sizeof(dev_path));
	TEST_ASSERT_EQUAL(0, rc);
	TEST_ASSERT_EQUAL_STRING("/dev/ttyACM1", dev_path);

	unlink(link0);
	unlink(link1);
	rmdir(acm0_dir);
	rmdir(acm1_dir);
	rmdir(tty_root);
	snprintf(path, sizeof(path), "%s/idVendor", usb_wrong);
	unlink(path);
	snprintf(path, sizeof(path), "%s/idProduct", usb_wrong);
	unlink(path);
	snprintf(path, sizeof(path), "%s/idVendor", usb_talos);
	unlink(path);
	snprintf(path, sizeof(path), "%s/idProduct", usb_talos);
	unlink(path);
	rmdir(usb_wrong);
	rmdir(usb_talos);
	rmdir(root);
}

void run_bridge_serial_tests(void)
{
	RUN_TEST(test_sanitization_and_truncation);
	RUN_TEST(test_diff_only_and_typing_cooldown);
	RUN_TEST(test_pause_resume_and_disconnect_recovery);
	RUN_TEST(test_sysfs_discovery_matches_talos_vid_pid);
}
