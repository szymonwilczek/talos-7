#include "bridge_ci.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void test_ci_config_parse_save_and_mutations(void)
{
	const char *raw = "# Watch CI\n"
			  "poll_interval_sec = 45\n"
			  "require_checks = true\n"
			  "notify_on_startup = false\n"
			  "notify_mode = transition\n\n"
			  "[repo \"szymonwilczek/talos-7\"]\n"
			  "alias = talos\n"
			  "branches = main, release/*\n\n"
			  "[repo \"szymonwilczek/lota\"]\n"
			  "branches = dev\n";

	bridge_ci_config_t cfg;
	TEST_ASSERT_TRUE(bridge_ci_parse_config(raw, &cfg));
	TEST_ASSERT_EQUAL_UINT32(45U, cfg.poll_interval_sec);
	TEST_ASSERT_TRUE(cfg.require_checks);
	TEST_ASSERT_FALSE(cfg.notify_on_startup);
	TEST_ASSERT_EQUAL(2, cfg.repo_count);
	TEST_ASSERT_EQUAL_STRING("szymonwilczek/talos-7", cfg.repos[0].slug);
	TEST_ASSERT_EQUAL_STRING("talos", cfg.repos[0].alias);
	TEST_ASSERT_EQUAL(2, cfg.repos[0].branch_count);
	TEST_ASSERT_EQUAL_STRING("release/*", cfg.repos[0].branches[1]);

	TEST_ASSERT_TRUE(bridge_ci_config_add_repo(&cfg, "szymonwilczek/lota",
						   "main,feat/*", "lota"));
	TEST_ASSERT_EQUAL(3, cfg.repos[1].branch_count);
	TEST_ASSERT_EQUAL_STRING("lota", cfg.repos[1].alias);

	TEST_ASSERT_TRUE(bridge_ci_config_remove_repo(
		&cfg, "szymonwilczek/lota", "dev"));
	TEST_ASSERT_EQUAL(2, cfg.repos[1].branch_count);

	char conf_path[128];
	char state_path[128];
	snprintf(conf_path, sizeof(conf_path), "/tmp/talos_ci_conf_%d.conf",
		 (int)getpid());
	snprintf(state_path, sizeof(state_path), "/tmp/talos_ci_state_%d.state",
		 (int)getpid());
	unlink(conf_path);
	unlink(state_path);

	TEST_ASSERT_TRUE(bridge_ci_save_config(conf_path, &cfg));

	bridge_ci_t loaded;
	bridge_ci_init(&loaded, conf_path, state_path);
	TEST_ASSERT_EQUAL(2, loaded.cfg.repo_count);
	TEST_ASSERT_EQUAL_STRING("talos", loaded.cfg.repos[0].alias);
	unlink(conf_path);
	unlink(state_path);
}

static void test_ci_branch_glob_and_api_parsers(void)
{
	TEST_ASSERT_TRUE(bridge_ci_branch_matches("main", "main"));
	TEST_ASSERT_TRUE(bridge_ci_branch_matches("release/*", "release/v4.0"));
	TEST_ASSERT_FALSE(bridge_ci_branch_matches("release/*", "hotfix/1"));

	char sha[48];
	bridge_ci_status_t st = CI_STATUS_UNKNOWN;

	TEST_ASSERT_TRUE(bridge_ci_parse_rest_check_runs(
		"{\"total_count\":0,\"check_runs\":[]}", sha, sizeof(sha),
		&st));
	TEST_ASSERT_EQUAL(CI_STATUS_NONE, st);

	TEST_ASSERT_TRUE(bridge_ci_parse_rest_check_runs(
		"{\"check_runs\":[{\"head_sha\":\"abc123\",\"status\":\"completed\","
		"\"conclusion\":\"success\"},{\"head_sha\":\"abc123\","
		"\"status\":\"in_progress\",\"conclusion\":null}]}",
		sha, sizeof(sha), &st));
	TEST_ASSERT_EQUAL_STRING("abc123", sha);
	TEST_ASSERT_EQUAL(CI_STATUS_PENDING, st);

	TEST_ASSERT_TRUE(bridge_ci_parse_rest_check_runs(
		"{\"check_runs\":[{\"head_sha\":\"abc123\",\"status\":\"completed\","
		"\"conclusion\":\"success\"},{\"head_sha\":\"abc123\","
		"\"status\":\"completed\",\"conclusion\":\"failure\"}]}",
		sha, sizeof(sha), &st));
	TEST_ASSERT_EQUAL(CI_STATUS_FAIL, st);

	TEST_ASSERT_TRUE(bridge_ci_parse_rest_check_runs(
		"{\"check_runs\":[{\"head_sha\":\"def456\",\"status\":\"completed\","
		"\"conclusion\":\"success\"},{\"head_sha\":\"def456\","
		"\"status\":\"completed\",\"conclusion\":\"skipped\"}]}",
		sha, sizeof(sha), &st));
	TEST_ASSERT_EQUAL_STRING("def456", sha);
	TEST_ASSERT_EQUAL(CI_STATUS_GREEN, st);

	char branch[48];
	int count = 0;
	TEST_ASSERT_TRUE(bridge_ci_parse_graphql_ref(
		"{\"name\":\"main\",\"target\":{\"oid\":\"999aaa\","
		"\"statusCheckRollup\":{\"state\":\"SUCCESS\",\"contexts\":{\"totalCount\":3}}}}",
		branch, sizeof(branch), sha, sizeof(sha), &st, &count));
	TEST_ASSERT_EQUAL_STRING("main", branch);
	TEST_ASSERT_EQUAL_STRING("999aaa", sha);
	TEST_ASSERT_EQUAL_INT(3, count);
	TEST_ASSERT_EQUAL(CI_STATUS_GREEN, st);
}

static void test_ci_eval_transitions_and_seen_persistence(void)
{
	char conf_path[128];
	char state_path[128];
	snprintf(conf_path, sizeof(conf_path), "/tmp/talos_ci_ev_%d.conf",
		 (int)getpid());
	snprintf(state_path, sizeof(state_path), "/tmp/talos_ci_ev_%d.state",
		 (int)getpid());
	unlink(conf_path);
	unlink(state_path);

	bridge_ci_t ci;
	bridge_ci_init(&ci, conf_path, state_path);
	ci.cfg.notify_on_startup = false;

	char label[64];
	/* Startup green commit is baselined silently when notify_on_startup=false */
	TEST_ASSERT_FALSE(bridge_ci_eval_result(
		&ci, "szymonwilczek/talos-7", "talos-7", "main", "sha1",
		CI_STATUS_GREEN, label, sizeof(label)));

	/* Pending commit -> transition to green fires once */
	TEST_ASSERT_FALSE(bridge_ci_eval_result(
		&ci, "szymonwilczek/talos-7", "talos-7", "main", "sha2",
		CI_STATUS_PENDING, label, sizeof(label)));
	TEST_ASSERT_TRUE(bridge_ci_eval_result(
		&ci, "szymonwilczek/talos-7", "talos-7", "main", "sha2",
		CI_STATUS_GREEN, label, sizeof(label)));
	TEST_ASSERT_EQUAL_STRING("talos-7@main", label);

	/* Repeated poll on same SHA does not fire again */
	TEST_ASSERT_FALSE(bridge_ci_eval_result(
		&ci, "szymonwilczek/talos-7", "talos-7", "main", "sha2",
		CI_STATUS_GREEN, label, sizeof(label)));

	/* Fast CI that finishes between polls on a brand-new SHA also fires */
	TEST_ASSERT_TRUE(bridge_ci_eval_result(
		&ci, "szymonwilczek/talos-7", "talos-7", "main", "sha3",
		CI_STATUS_GREEN, label, sizeof(label)));

	TEST_ASSERT_TRUE(bridge_ci_save_seen_state(&ci));

	/* Reloaded instance remembers sha3 was already notified */
	bridge_ci_t reloaded;
	bridge_ci_init(&reloaded, conf_path, state_path);
	TEST_ASSERT_FALSE(bridge_ci_eval_result(
		&reloaded, "szymonwilczek/talos-7", "talos-7", "main", "sha3",
		CI_STATUS_GREEN, label, sizeof(label)));
	unlink(conf_path);
	unlink(state_path);
}

void run_bridge_ci_tests(void)
{
	RUN_TEST(test_ci_config_parse_save_and_mutations);
	RUN_TEST(test_ci_branch_glob_and_api_parsers);
	RUN_TEST(test_ci_eval_transitions_and_seen_persistence);
}
