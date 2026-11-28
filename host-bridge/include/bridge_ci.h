#ifndef BRIDGE_CI_H
#define BRIDGE_CI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#define BRIDGE_CI_MAX_REPOS 16U
#define BRIDGE_CI_MAX_BRANCHES 8U
#define BRIDGE_CI_MAX_TRACKED 64U

typedef enum {
	CI_STATUS_UNKNOWN = 0,
	CI_STATUS_NONE,
	CI_STATUS_PENDING,
	CI_STATUS_FAIL,
	CI_STATUS_GREEN
} bridge_ci_status_t;

typedef enum {
	CI_NOTIFY_TRANSITION = 0,
	CI_NOTIFY_NEW_SHA
} bridge_ci_notify_mode_t;

typedef struct {
	char slug[96]; /* owner/repo */
	char alias[48];
	char branches[BRIDGE_CI_MAX_BRANCHES][48];
	size_t branch_count;
} bridge_ci_repo_t;

typedef struct {
	uint32_t poll_interval_sec;
	bool require_checks;
	bool notify_on_startup;
	bridge_ci_notify_mode_t notify_mode;
	char token[128];
	bridge_ci_repo_t repos[BRIDGE_CI_MAX_REPOS];
	size_t repo_count;
} bridge_ci_config_t;

typedef struct {
	char key[144]; /* owner/repo#branch */
	char last_sha[48];
	char notified_sha[48];
	char etag[96];
	bridge_ci_status_t last_status;
	bool seen_once;
} bridge_ci_entry_t;

typedef struct {
	char config_path[256];
	char state_path[256];
	time_t config_mtime;
	bridge_ci_config_t cfg;
	bridge_ci_entry_t entries[BRIDGE_CI_MAX_TRACKED];
	size_t entry_count;

	int worker_fd;
	pid_t worker_pid;
	char worker_rx[16384];
	size_t worker_rx_len;
	uint64_t last_poll_ms;
	bool force_poll;
	bool startup_done;
} bridge_ci_t;

/**
 * Resolve default paths for `~/.config/talos-bridge/ci.conf` and
 * `~/.local/state/talos-bridge/ci-seen.state`.
 */
void bridge_ci_default_paths(char *conf_out, size_t conf_sz, char *state_out,
			     size_t state_sz);

/**
 * Initialize CI watcher state and load config + persisted seen commits.
 */
void bridge_ci_init(bridge_ci_t *ci, const char *conf_override,
		    const char *state_override);

/**
 * Stop any active async GitHub worker process cleanly.
 */
void bridge_ci_close(bridge_ci_t *ci);

/**
 * Parse INI-style `ci.conf` contents into a structured config.
 */
bool bridge_ci_parse_config(const char *text, bridge_ci_config_t *out_cfg);

/**
 * Write `cfg` cleanly to `path`, creating parent directories if needed.
 */
bool bridge_ci_save_config(const char *path, const bridge_ci_config_t *cfg);

/**
 * Add or update a repository + comma-separated branch list in `cfg`.
 */
bool bridge_ci_config_add_repo(bridge_ci_config_t *cfg, const char *slug,
			       const char *branches_csv, const char *alias);

/**
 * Remove a repository (or a single branch when `branch != NULL`) from `cfg`.
 */
bool bridge_ci_config_remove_repo(bridge_ci_config_t *cfg, const char *slug,
				  const char *branch);

/**
 * Glob matcher supporting `*` and `?` against a Git branch name.
 */
bool bridge_ci_branch_matches(const char *pattern, const char *branch);

/**
 * Parse GitHub REST `/commits/<ref>/check-runs` JSON into SHA + aggregate status.
 */
bool bridge_ci_parse_rest_check_runs(const char *json, char *out_sha,
				     size_t sha_sz,
				     bridge_ci_status_t *out_status);

/**
 * Split a `curl -i` HTTP response into HTTP status, optional ETag, and body.
 */
bool bridge_ci_split_http_response(const char *raw, int *out_status,
				   char *out_etag, size_t etag_sz,
				   const char **out_body);

/**
 * Parse GitHub GraphQL `ref { name target { ... on Commit { oid statusCheckRollup ... } } }`
 * fragment into branch name, SHA, and aggregate status.
 */
bool bridge_ci_parse_graphql_ref(const char *json, char *out_branch,
				 size_t branch_sz, char *out_sha, size_t sha_sz,
				 bridge_ci_status_t *out_status,
				 int *out_check_count);

/**
 * Evaluate one polled (repo, branch, sha, status) against history + config.
 * Returns true and formats `out_label` (e.g. "talos-7@main") if an alert fires.
 */
bool bridge_ci_eval_result(bridge_ci_t *ci, const char *slug, const char *alias,
			   const char *branch, const char *sha,
			   bridge_ci_status_t status, char *out_label,
			   size_t label_sz);

/**
 * Load persisted `ci-seen.state` and save updated entries.
 */
bool bridge_ci_load_seen_state(bridge_ci_t *ci);
bool bridge_ci_save_seen_state(const bridge_ci_t *ci);

/**
 * Reload `ci.conf` automatically when file mtime changes.
 */
void bridge_ci_reload_if_changed(bridge_ci_t *ci);

/**
 * Service periodic/forced GitHub checks and drain any finished async worker
 * lines, invoking `on_alert(label, ctx)` for every newly green commit.
 */
void bridge_ci_service(bridge_ci_t *ci, uint64_t now_ms, bool fd_readable,
		       void (*on_alert)(const char *label, void *ctx),
		       void *ctx);

/**
 * Format a compact status table line list from `ci.conf` + `ci-seen.state`.
 */
void bridge_ci_format_summary(const bridge_ci_t *ci, char *out,
			      size_t out_size);

#endif /* BRIDGE_CI_H */
