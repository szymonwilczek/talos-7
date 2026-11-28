#include "bridge_ci.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static void copy_str(char *dst, size_t dst_sz, const char *src)
{
	if (!dst || dst_sz == 0)
		return;
	if (!src) {
		dst[0] = '\0';
		return;
	}
	snprintf(dst, dst_sz, "%s", src);
}

static char *trim_inplace(char *s)
{
	while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
		s++;
	size_t len = strlen(s);
	while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' ||
			   s[len - 1] == '\r' || s[len - 1] == '\n')) {
		s[--len] = '\0';
	}
	return s;
}

static bool str_eq_ci(const char *a, const char *b)
{
	if (!a || !b)
		return false;
	while (*a && *b) {
		if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
			return false;
		a++;
		b++;
	}
	return *a == '\0' && *b == '\0';
}

static void ensure_parent_dir(const char *filepath)
{
	char tmp[256];
	copy_str(tmp, sizeof(tmp), filepath);
	for (char *p = tmp + 1; *p != '\0'; p++) {
		if (*p == '/') {
			*p = '\0';
			mkdir(tmp, 0755);
			*p = '/';
		}
	}
}

static void config_set_defaults(bridge_ci_config_t *cfg)
{
	memset(cfg, 0, sizeof(*cfg));
	cfg->poll_interval_sec = 30U;
	cfg->require_checks = true;
	cfg->notify_on_startup = false;
	cfg->notify_mode = CI_NOTIFY_TRANSITION;
}

void bridge_ci_default_paths(char *conf_out, size_t conf_sz, char *state_out,
			     size_t state_sz)
{
	const char *xdg_conf = getenv("XDG_CONFIG_HOME");
	const char *xdg_state = getenv("XDG_STATE_HOME");
	const char *home = getenv("HOME");
	if (!home || home[0] == '\0')
		home = "/tmp";

	if (conf_out && conf_sz > 0) {
		if (xdg_conf && xdg_conf[0] != '\0') {
			snprintf(conf_out, conf_sz, "%s/talos-bridge/ci.conf",
				 xdg_conf);
		} else {
			snprintf(conf_out, conf_sz,
				 "%s/.config/talos-bridge/ci.conf", home);
		}
	}
	if (state_out && state_sz > 0) {
		if (xdg_state && xdg_state[0] != '\0') {
			snprintf(state_out, state_sz,
				 "%s/talos-bridge/ci-seen.state", xdg_state);
		} else {
			snprintf(state_out, state_sz,
				 "%s/.local/state/talos-bridge/ci-seen.state",
				 home);
		}
	}
}

bool bridge_ci_branch_matches(const char *pattern, const char *branch)
{
	if (!pattern || !branch || pattern[0] == '\0' || branch[0] == '\0')
		return false;

	const char *p = pattern;
	const char *b = branch;
	const char *star = NULL;
	const char *b_back = NULL;

	while (*b != '\0') {
		if (*p == *b || *p == '?') {
			p++;
			b++;
		} else if (*p == '*') {
			star = p++;
			b_back = b;
		} else if (star != NULL) {
			p = star + 1;
			b = ++b_back;
		} else {
			return false;
		}
	}
	while (*p == '*')
		p++;
	return *p == '\0';
}

static void add_branches_csv(bridge_ci_repo_t *repo, const char *csv)
{
	if (!repo || !csv)
		return;
	char copy[384];
	copy_str(copy, sizeof(copy), csv);

	char *cur = copy;
	while (cur && *cur != '\0') {
		char *sep = strpbrk(cur, ", ");
		if (sep)
			*sep++ = '\0';
		char *tok = trim_inplace(cur);
		if (tok[0] != '\0' &&
		    repo->branch_count < BRIDGE_CI_MAX_BRANCHES) {
			bool dup = false;
			for (size_t i = 0; i < repo->branch_count; i++) {
				if (strcmp(repo->branches[i], tok) == 0) {
					dup = true;
					break;
				}
			}
			if (!dup) {
				copy_str(repo->branches[repo->branch_count++],
					 sizeof(repo->branches[0]), tok);
			}
		}
		cur = sep;
	}
}

bool bridge_ci_parse_config(const char *text, bridge_ci_config_t *out_cfg)
{
	if (!text || !out_cfg)
		return false;
	config_set_defaults(out_cfg);

	bridge_ci_repo_t *cur_repo = NULL;
	const char *p = text;

	while (*p != '\0') {
		const char *nl = strchr(p, '\n');
		size_t len = nl ? (size_t)(nl - p) : strlen(p);
		char line[512];
		if (len >= sizeof(line))
			len = sizeof(line) - 1;
		memcpy(line, p, len);
		line[len] = '\0';
		p = nl ? (nl + 1) : (p + len);

		/* Strip inline comments outside quoted sections */
		char *hash = strchr(line, '#');
		if (hash)
			*hash = '\0';
		char *s = trim_inplace(line);
		if (s[0] == '\0')
			continue;

		if (s[0] == '[') {
			cur_repo = NULL;
			char *q1 = strchr(s, '"');
			char *q2 = q1 ? strchr(q1 + 1, '"') : NULL;
			if (q1 && q2 && q2 > q1 + 1 &&
			    out_cfg->repo_count < BRIDGE_CI_MAX_REPOS) {
				*q2 = '\0';
				cur_repo =
					&out_cfg->repos[out_cfg->repo_count++];
				memset(cur_repo, 0, sizeof(*cur_repo));
				copy_str(cur_repo->slug, sizeof(cur_repo->slug),
					 q1 + 1);
			}
			continue;
		}

		char *eq = strchr(s, '=');
		if (!eq)
			continue;
		*eq = '\0';
		char *key = trim_inplace(s);
		char *val = trim_inplace(eq + 1);

		if (cur_repo == NULL) {
			if (strcmp(key, "poll_interval_sec") == 0) {
				long sec = strtol(val, NULL, 10);
				if (sec < 10L)
					sec = 10L;
				if (sec > 3600L)
					sec = 3600L;
				out_cfg->poll_interval_sec = (uint32_t)sec;
			} else if (strcmp(key, "require_checks") == 0) {
				out_cfg->require_checks =
					(str_eq_ci(val, "true") ||
					 strcmp(val, "1") == 0);
			} else if (strcmp(key, "notify_on_startup") == 0) {
				out_cfg->notify_on_startup =
					(str_eq_ci(val, "true") ||
					 strcmp(val, "1") == 0);
			} else if (strcmp(key, "notify_mode") == 0) {
				out_cfg->notify_mode =
					str_eq_ci(val, "new_sha") ?
						CI_NOTIFY_NEW_SHA :
						CI_NOTIFY_TRANSITION;
			} else if (strcmp(key, "token") == 0) {
				copy_str(out_cfg->token, sizeof(out_cfg->token),
					 val);
			}
		} else {
			if (strcmp(key, "alias") == 0) {
				copy_str(cur_repo->alias,
					 sizeof(cur_repo->alias), val);
			} else if (strcmp(key, "branches") == 0 ||
				   strcmp(key, "branch") == 0) {
				add_branches_csv(cur_repo, val);
			}
		}
	}

	for (size_t i = 0; i < out_cfg->repo_count; i++) {
		if (out_cfg->repos[i].branch_count == 0) {
			copy_str(out_cfg->repos[i].branches[0],
				 sizeof(out_cfg->repos[i].branches[0]), "main");
			out_cfg->repos[i].branch_count = 1;
		}
	}
	return true;
}

bool bridge_ci_save_config(const char *path, const bridge_ci_config_t *cfg)
{
	if (!path || !cfg || path[0] == '\0')
		return false;
	ensure_parent_dir(path);

	FILE *f = fopen(path, "w");
	if (!f)
		return false;

	fprintf(f,
		"# Talos 7 GitHub CI Green watcher configuration\n"
		"poll_interval_sec = %u\n"
		"require_checks = %s\n"
		"notify_on_startup = %s\n"
		"notify_mode = %s\n",
		(unsigned)cfg->poll_interval_sec,
		cfg->require_checks ? "true" : "false",
		cfg->notify_on_startup ? "true" : "false",
		(cfg->notify_mode == CI_NOTIFY_NEW_SHA) ? "new_sha" :
							  "transition");
	if (cfg->token[0] != '\0') {
		fprintf(f, "token = %s\n", cfg->token);
	}

	for (size_t i = 0; i < cfg->repo_count; i++) {
		const bridge_ci_repo_t *r = &cfg->repos[i];
		fprintf(f, "\n[repo \"%s\"]\n", r->slug);
		if (r->alias[0] != '\0')
			fprintf(f, "alias = %s\n", r->alias);
		fprintf(f, "branches = ");
		for (size_t b = 0; b < r->branch_count; b++) {
			fprintf(f, "%s%s", (b > 0) ? ", " : "", r->branches[b]);
		}
		fputc('\n', f);
	}

	fclose(f);
	return true;
}

bool bridge_ci_config_add_repo(bridge_ci_config_t *cfg, const char *slug,
			       const char *branches_csv, const char *alias)
{
	if (!cfg || !slug || slug[0] == '\0' || !strchr(slug, '/'))
		return false;

	bridge_ci_repo_t *target = NULL;
	for (size_t i = 0; i < cfg->repo_count; i++) {
		if (strcmp(cfg->repos[i].slug, slug) == 0) {
			target = &cfg->repos[i];
			break;
		}
	}
	if (!target) {
		if (cfg->repo_count >= BRIDGE_CI_MAX_REPOS)
			return false;
		target = &cfg->repos[cfg->repo_count++];
		memset(target, 0, sizeof(*target));
		copy_str(target->slug, sizeof(target->slug), slug);
	}
	if (alias && alias[0] != '\0') {
		copy_str(target->alias, sizeof(target->alias), alias);
	}
	if (branches_csv && branches_csv[0] != '\0') {
		add_branches_csv(target, branches_csv);
	}
	if (target->branch_count == 0) {
		copy_str(target->branches[0], sizeof(target->branches[0]),
			 "main");
		target->branch_count = 1;
	}
	return true;
}

bool bridge_ci_config_remove_repo(bridge_ci_config_t *cfg, const char *slug,
				  const char *branch)
{
	if (!cfg || !slug || slug[0] == '\0')
		return false;

	for (size_t i = 0; i < cfg->repo_count; i++) {
		if (strcmp(cfg->repos[i].slug, slug) != 0)
			continue;

		bridge_ci_repo_t *r = &cfg->repos[i];
		if (branch && branch[0] != '\0') {
			bool removed = false;
			for (size_t b = 0; b < r->branch_count; b++) {
				if (strcmp(r->branches[b], branch) == 0) {
					for (size_t k = b + 1;
					     k < r->branch_count; k++) {
						copy_str(r->branches[k - 1],
							 sizeof(r->branches[0]),
							 r->branches[k]);
					}
					r->branch_count--;
					removed = true;
					break;
				}
			}
			if (!removed)
				return false;
			if (r->branch_count > 0)
				return true;
		}

		for (size_t k = i + 1; k < cfg->repo_count; k++) {
			cfg->repos[k - 1] = cfg->repos[k];
		}
		cfg->repo_count--;
		return true;
	}
	return false;
}

static bool extract_json_str_after(const char *start, const char *key,
				   char *out, size_t out_sz)
{
	if (!start || !key || !out || out_sz == 0)
		return false;
	char pat[64];
	snprintf(pat, sizeof(pat), "\"%s\"", key);
	const char *hit = strstr(start, pat);
	if (!hit)
		return false;
	const char *p = hit + strlen(pat);
	while (*p == ' ' || *p == '\t' || *p == ':')
		p++;
	if (*p != '"')
		return false;
	p++;
	size_t n = 0;
	while (*p != '\0' && *p != '"' && n + 1 < out_sz) {
		if (*p == '\\' && *(p + 1) != '\0')
			p++;
		out[n++] = *p++;
	}
	out[n] = '\0';
	return true;
}

static bool conclusion_is_green(const char *conclusion)
{
	return strcmp(conclusion, "success") == 0 ||
	       strcmp(conclusion, "neutral") == 0 ||
	       strcmp(conclusion, "skipped") == 0;
}

bool bridge_ci_parse_rest_check_runs(const char *json, char *out_sha,
				     size_t sha_sz,
				     bridge_ci_status_t *out_status)
{
	if (!json || !out_sha || sha_sz == 0 || !out_status)
		return false;
	out_sha[0] = '\0';
	*out_status = CI_STATUS_NONE;

	const char *arr = strstr(json, "\"check_runs\"");
	if (!arr)
		return false;
	arr = strchr(arr, '[');
	if (!arr)
		return false;

	size_t count = 0;
	bool has_pending = false;
	bool has_fail = false;

	const char *p = arr + 1;
	while (*p != '\0' && *p != ']') {
		const char *obj = strchr(p, '{');
		const char *end_arr = strchr(p, ']');
		if (!obj || (end_arr && obj > end_arr))
			break;

		/* Find matching closing brace of check_run object */
		int depth = 0;
		const char *q = obj;
		bool in_str = false;
		while (*q != '\0') {
			if (*q == '"' && (q == obj || *(q - 1) != '\\')) {
				in_str = !in_str;
			} else if (!in_str) {
				if (*q == '{')
					depth++;
				else if (*q == '}' && --depth == 0) {
					q++;
					break;
				}
			}
			q++;
		}
		if (depth != 0)
			break;

		size_t obj_len = (size_t)(q - obj);
		char *slice = malloc(obj_len + 1);
		if (!slice)
			break;
		memcpy(slice, obj, obj_len);
		slice[obj_len] = '\0';

		count++;
		if (out_sha[0] == '\0') {
			extract_json_str_after(slice, "head_sha", out_sha,
					       sha_sz);
		}
		char status[32] = { 0 };
		char conclusion[32] = { 0 };
		extract_json_str_after(slice, "status", status, sizeof(status));
		extract_json_str_after(slice, "conclusion", conclusion,
				       sizeof(conclusion));
		free(slice);

		if (strcmp(status, "completed") != 0) {
			has_pending = true;
		} else if (!conclusion_is_green(conclusion)) {
			has_fail = true;
		}
		p = q;
	}

	if (count == 0U) {
		*out_status = CI_STATUS_NONE;
	} else if (has_fail) {
		*out_status = CI_STATUS_FAIL;
	} else if (has_pending) {
		*out_status = CI_STATUS_PENDING;
	} else {
		*out_status = CI_STATUS_GREEN;
	}
	return true;
}

bool bridge_ci_parse_graphql_ref(const char *json, char *out_branch,
				 size_t branch_sz, char *out_sha, size_t sha_sz,
				 bridge_ci_status_t *out_status,
				 int *out_check_count)
{
	if (!json || !out_branch || !out_sha || !out_status)
		return false;
	out_branch[0] = '\0';
	out_sha[0] = '\0';
	*out_status = CI_STATUS_NONE;
	if (out_check_count)
		*out_check_count = 0;

	if (!extract_json_str_after(json, "name", out_branch, branch_sz))
		return false;
	if (!extract_json_str_after(json, "oid", out_sha, sha_sz))
		return false;

	int total_count = 0;
	const char *tc = strstr(json, "\"totalCount\"");
	if (tc) {
		const char *colon = strchr(tc, ':');
		if (colon)
			total_count = (int)strtol(colon + 1, NULL, 10);
	}
	if (out_check_count)
		*out_check_count = total_count;

	char state[32] = { 0 };
	if (!extract_json_str_after(json, "state", state, sizeof(state)) ||
	    total_count <= 0) {
		*out_status = CI_STATUS_NONE;
		return true;
	}

	if (strcmp(state, "SUCCESS") == 0) {
		*out_status = CI_STATUS_GREEN;
	} else if (strcmp(state, "PENDING") == 0 ||
		   strcmp(state, "EXPECTED") == 0) {
		*out_status = CI_STATUS_PENDING;
	} else {
		*out_status = CI_STATUS_FAIL;
	}
	return true;
}

static bridge_ci_entry_t *find_or_create_entry(bridge_ci_t *ci, const char *key)
{
	for (size_t i = 0; i < ci->entry_count; i++) {
		if (strcmp(ci->entries[i].key, key) == 0)
			return &ci->entries[i];
	}
	if (ci->entry_count >= BRIDGE_CI_MAX_TRACKED)
		return NULL;
	bridge_ci_entry_t *e = &ci->entries[ci->entry_count++];
	memset(e, 0, sizeof(*e));
	copy_str(e->key, sizeof(e->key), key);
	return e;
}

static const char *repo_display_name(const char *slug, const char *alias)
{
	if (alias && alias[0] != '\0')
		return alias;
	const char *slash = slug ? strrchr(slug, '/') : NULL;
	return (slash && *(slash + 1) != '\0') ? (slash + 1) :
						 (slug ? slug : "repo");
}

bool bridge_ci_eval_result(bridge_ci_t *ci, const char *slug, const char *alias,
			   const char *branch, const char *sha,
			   bridge_ci_status_t status, char *out_label,
			   size_t label_sz)
{
	if (!ci || !slug || !branch || !sha)
		return false;

	char key[144];
	snprintf(key, sizeof(key), "%.90s#%.46s", slug, branch);
	bridge_ci_entry_t *ent = find_or_create_entry(ci, key);
	if (!ent)
		return false;

	bridge_ci_status_t effective = status;
	if (status == CI_STATUS_NONE && !ci->cfg.require_checks &&
	    sha[0] != '\0') {
		effective = CI_STATUS_GREEN;
	}

	bool fire = false;
	if (effective == CI_STATUS_GREEN && sha[0] != '\0' &&
	    strcmp(ent->notified_sha, sha) != 0) {
		if (!ent->seen_once && !ci->cfg.notify_on_startup) {
			/* Baseline existing green commit silently at startup */
			copy_str(ent->notified_sha, sizeof(ent->notified_sha),
				 sha);
		} else {
			fire = true;
			copy_str(ent->notified_sha, sizeof(ent->notified_sha),
				 sha);
		}
	}

	copy_str(ent->last_sha, sizeof(ent->last_sha), sha);
	ent->last_status = effective;
	ent->seen_once = true;

	if (fire && out_label && label_sz > 0) {
		const char *name = repo_display_name(slug, alias);
		snprintf(out_label, label_sz, "%.40s@%.20s", name, branch);
	}
	return fire;
}

bool bridge_ci_load_seen_state(bridge_ci_t *ci)
{
	if (!ci || ci->state_path[0] == '\0')
		return false;
	FILE *f = fopen(ci->state_path, "r");
	if (!f)
		return false;

	char line[256];
	while (fgets(line, sizeof(line), f)) {
		char *s = trim_inplace(line);
		if (s[0] == '\0' || s[0] == '#')
			continue;
		char *p1 = strchr(s, '|');
		if (!p1)
			continue;
		*p1++ = '\0';
		char *p2 = strchr(p1, '|');
		if (p2)
			*p2++ = '\0';

		bridge_ci_entry_t *ent = find_or_create_entry(ci, s);
		if (!ent)
			continue;
		copy_str(ent->notified_sha, sizeof(ent->notified_sha), p1);
		copy_str(ent->last_sha, sizeof(ent->last_sha), p1);
		if (p2) {
			ent->last_status =
				(bridge_ci_status_t)strtol(p2, NULL, 10);
		}
		ent->seen_once = true;
	}
	fclose(f);
	return true;
}

bool bridge_ci_save_seen_state(const bridge_ci_t *ci)
{
	if (!ci || ci->state_path[0] == '\0')
		return false;
	ensure_parent_dir(ci->state_path);

	FILE *f = fopen(ci->state_path, "w");
	if (!f)
		return false;
	for (size_t i = 0; i < ci->entry_count; i++) {
		const bridge_ci_entry_t *e = &ci->entries[i];
		const char *sha = e->notified_sha[0] ? e->notified_sha :
						       e->last_sha;
		fprintf(f, "%s|%s|%d\n", e->key, sha, (int)e->last_status);
	}
	fclose(f);
	return true;
}

static bool load_config_file(bridge_ci_t *ci)
{
	struct stat st;
	if (stat(ci->config_path, &st) != 0)
		return false;

	FILE *f = fopen(ci->config_path, "r");
	if (!f)
		return false;

	char buf[16384];
	size_t n = fread(buf, 1, sizeof(buf) - 1, f);
	fclose(f);
	buf[n] = '\0';

	if (!bridge_ci_parse_config(buf, &ci->cfg))
		return false;
	ci->config_mtime = st.st_mtime;
	return true;
}

void bridge_ci_reload_if_changed(bridge_ci_t *ci)
{
	if (!ci || ci->config_path[0] == '\0')
		return;
	struct stat st;
	if (stat(ci->config_path, &st) != 0) {
		config_set_defaults(&ci->cfg);
		ci->config_mtime = 0;
		return;
	}
	if (st.st_mtime != ci->config_mtime) {
		load_config_file(ci);
	}
}

void bridge_ci_init(bridge_ci_t *ci, const char *conf_override,
		    const char *state_override)
{
	if (!ci)
		return;
	memset(ci, 0, sizeof(*ci));
	config_set_defaults(&ci->cfg);
	ci->worker_fd = -1;
	ci->worker_pid = -1;

	bridge_ci_default_paths(ci->config_path, sizeof(ci->config_path),
				ci->state_path, sizeof(ci->state_path));
	if (conf_override && conf_override[0] != '\0')
		copy_str(ci->config_path, sizeof(ci->config_path),
			 conf_override);
	if (state_override && state_override[0] != '\0')
		copy_str(ci->state_path, sizeof(ci->state_path),
			 state_override);

	load_config_file(ci);
	bridge_ci_load_seen_state(ci);
}

void bridge_ci_close(bridge_ci_t *ci)
{
	if (!ci)
		return;
	if (ci->worker_fd >= 0) {
		close(ci->worker_fd);
		ci->worker_fd = -1;
	}
	if (ci->worker_pid > 0) {
		kill(ci->worker_pid, SIGTERM);
		waitpid(ci->worker_pid, NULL, 0);
		ci->worker_pid = -1;
	}
}

static bool capture_cmd(const char *const argv[], char *out, size_t out_sz)
{
	out[0] = '\0';
	int pfd[2];
	if (pipe(pfd) != 0)
		return false;
	pid_t pid = fork();
	if (pid < 0) {
		close(pfd[0]);
		close(pfd[1]);
		return false;
	}
	if (pid == 0) {
		close(pfd[0]);
		dup2(pfd[1], STDOUT_FILENO);
		close(pfd[1]);
		int dn = open("/dev/null", O_RDWR);
		if (dn >= 0) {
			dup2(dn, STDERR_FILENO);
			close(dn);
		}
		execvp(argv[0], (char *const *)argv);
		_exit(127);
	}
	close(pfd[1]);
	size_t tot = 0;
	while (tot + 1 < out_sz) {
		ssize_t n = read(pfd[0], out + tot, out_sz - 1 - tot);
		if (n > 0) {
			tot += (size_t)n;
			continue;
		}
		if (n < 0 && errno == EINTR)
			continue;
		break;
	}
	close(pfd[0]);
	out[tot] = '\0';
	int status = 0;
	while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
	}
	return WIFEXITED(status) && WEXITSTATUS(status) == 0 && tot > 0;
}

static const char *status_to_tok(bridge_ci_status_t st)
{
	switch (st) {
	case CI_STATUS_GREEN:
		return "GREEN";
	case CI_STATUS_PENDING:
		return "PENDING";
	case CI_STATUS_FAIL:
		return "FAIL";
	case CI_STATUS_NONE:
	default:
		return "NONE";
	}
}

static bridge_ci_status_t tok_to_status(const char *tok)
{
	if (strcmp(tok, "GREEN") == 0)
		return CI_STATUS_GREEN;
	if (strcmp(tok, "PENDING") == 0)
		return CI_STATUS_PENDING;
	if (strcmp(tok, "FAIL") == 0)
		return CI_STATUS_FAIL;
	return CI_STATUS_NONE;
}

static void worker_poll_repo_branch_rest(size_t repo_idx,
					 const bridge_ci_repo_t *repo,
					 const char *branch, const char *token,
					 int out_fd)
{
	char url[256];
	snprintf(url, sizeof(url),
		 "https://api.github.com/repos/%s/commits/%s/check-runs",
		 repo->slug, branch);

	char auth_hdr[192];
	const char *argv[12];
	int ac = 0;
	argv[ac++] = "curl";
	argv[ac++] = "-sfL";
	argv[ac++] = "--max-time";
	argv[ac++] = "10";
	argv[ac++] = "-H";
	argv[ac++] = "Accept: application/vnd.github+json";
	if (token && token[0] != '\0') {
		snprintf(auth_hdr, sizeof(auth_hdr), "Authorization: Bearer %s",
			 token);
		argv[ac++] = "-H";
		argv[ac++] = auth_hdr;
	}
	argv[ac++] = url;
	argv[ac++] = NULL;

	char *json = malloc(65536);
	if (!json)
		return;
	if (capture_cmd(argv, json, 65536)) {
		char sha[48] = { 0 };
		bridge_ci_status_t st = CI_STATUS_NONE;
		if (bridge_ci_parse_rest_check_runs(json, sha, sizeof(sha),
						    &st)) {
			if (sha[0] == '\0') {
				/* Fetch commit SHA directly when 0 check_runs yet */
				char sha_url[256];
				snprintf(
					sha_url, sizeof(sha_url),
					"https://api.github.com/repos/%s/commits/%s",
					repo->slug, branch);
				argv[ac - 2] = sha_url;
				char cjson[4096];
				if (capture_cmd(argv, cjson, sizeof(cjson))) {
					extract_json_str_after(
						cjson, "sha", sha, sizeof(sha));
				}
			}
			dprintf(out_fd, "RES|%zu|%s|%s|%s\n", repo_idx, branch,
				sha, status_to_tok(st));
		}
	}
	free(json);
}

static bool repo_has_wildcard(const bridge_ci_repo_t *r)
{
	for (size_t b = 0; b < r->branch_count; b++) {
		if (strpbrk(r->branches[b], "*?") != NULL)
			return true;
	}
	return false;
}

static void worker_run(const bridge_ci_config_t *cfg, int out_fd)
{
	char gh_tok[128] = { 0 };
	const char *tok = cfg->token;
	if (!tok || tok[0] == '\0') {
		const char *env_tok = getenv("GH_TOKEN");
		if (!env_tok || env_tok[0] == '\0')
			env_tok = getenv("GITHUB_TOKEN");
		if (env_tok && env_tok[0] != '\0') {
			tok = env_tok;
		} else {
			const char *const gh_argv[] = { "gh", "auth", "token",
							NULL };
			if (capture_cmd(gh_argv, gh_tok, sizeof(gh_tok))) {
				tok = trim_inplace(gh_tok);
			} else {
				tok = "";
			}
		}
	}

	/* List branch names matching wildcards via REST branches endpoint, then query check-runs */
	for (size_t r = 0; r < cfg->repo_count; r++) {
		const bridge_ci_repo_t *repo = &cfg->repos[r];
		if (!repo_has_wildcard(repo)) {
			for (size_t b = 0; b < repo->branch_count; b++) {
				worker_poll_repo_branch_rest(r, repo,
							     repo->branches[b],
							     tok, out_fd);
			}
			continue;
		}

		char burl[256];
		snprintf(burl, sizeof(burl),
			 "https://api.github.com/repos/%s/branches?per_page=30",
			 repo->slug);
		char auth_hdr[192];
		const char *bargv[12];
		int ac = 0;
		bargv[ac++] = "curl";
		bargv[ac++] = "-sfL";
		bargv[ac++] = "--max-time";
		bargv[ac++] = "10";
		bargv[ac++] = "-H";
		bargv[ac++] = "Accept: application/vnd.github+json";
		if (tok[0] != '\0') {
			snprintf(auth_hdr, sizeof(auth_hdr),
				 "Authorization: Bearer %s", tok);
			bargv[ac++] = "-H";
			bargv[ac++] = auth_hdr;
		}
		bargv[ac++] = burl;
		bargv[ac++] = NULL;

		char bjson[32768];
		if (!capture_cmd(bargv, bjson, sizeof(bjson)))
			continue;

		const char *cur = bjson;
		char matched[BRIDGE_CI_MAX_BRANCHES][48];
		size_t mcount = 0;
		while ((cur = strstr(cur, "\"name\"")) != NULL &&
		       mcount < BRIDGE_CI_MAX_BRANCHES) {
			char bname[48] = { 0 };
			if (extract_json_str_after(cur, "name", bname,
						   sizeof(bname))) {
				for (size_t p = 0; p < repo->branch_count;
				     p++) {
					if (bridge_ci_branch_matches(
						    repo->branches[p], bname)) {
						copy_str(matched[mcount++],
							 sizeof(matched[0]),
							 bname);
						break;
					}
				}
			}
			cur += 6;
		}
		for (size_t m = 0; m < mcount; m++) {
			worker_poll_repo_branch_rest(r, repo, matched[m], tok,
						     out_fd);
		}
	}
}

static void spawn_worker(bridge_ci_t *ci)
{
	if (ci->cfg.repo_count == 0 || ci->worker_pid > 0)
		return;

	int pfd[2];
	if (pipe(pfd) != 0)
		return;

	pid_t pid = fork();
	if (pid < 0) {
		close(pfd[0]);
		close(pfd[1]);
		return;
	}
	if (pid == 0) {
		close(pfd[0]);
		worker_run(&ci->cfg, pfd[1]);
		close(pfd[1]);
		_exit(0);
	}

	close(pfd[1]);
	int flags = fcntl(pfd[0], F_GETFL, 0);
	if (flags >= 0)
		fcntl(pfd[0], F_SETFL, flags | O_NONBLOCK);
	ci->worker_fd = pfd[0];
	ci->worker_pid = pid;
	ci->worker_rx_len = 0;
}

static void process_worker_line(bridge_ci_t *ci, const char *line,
				void (*on_alert)(const char *label, void *ctx),
				void *ctx, bool *changed_state)
{
	if (strncmp(line, "RES|", 4) != 0)
		return;
	char copy[256];
	copy_str(copy, sizeof(copy), line + 4);

	char *p1 = strchr(copy, '|');
	if (!p1)
		return;
	*p1++ = '\0';
	char *p2 = strchr(p1, '|');
	if (!p2)
		return;
	*p2++ = '\0';
	char *p3 = strchr(p2, '|');
	if (!p3)
		return;
	*p3++ = '\0';

	size_t idx = (size_t)strtoul(copy, NULL, 10);
	if (idx >= ci->cfg.repo_count)
		return;
	const bridge_ci_repo_t *repo = &ci->cfg.repos[idx];

	char label[64];
	if (bridge_ci_eval_result(ci, repo->slug, repo->alias, p1, p2,
				  tok_to_status(p3), label, sizeof(label))) {
		if (on_alert)
			on_alert(label, ctx);
	}
	*changed_state = true;
}

void bridge_ci_service(bridge_ci_t *ci, uint64_t now_ms, bool fd_readable,
		       void (*on_alert)(const char *label, void *ctx),
		       void *ctx)
{
	if (!ci)
		return;
	bridge_ci_reload_if_changed(ci);

	if (ci->worker_fd >= 0 && fd_readable) {
		bool eof = false;
		while (ci->worker_rx_len + 1 < sizeof(ci->worker_rx)) {
			ssize_t n = read(ci->worker_fd,
					 ci->worker_rx + ci->worker_rx_len,
					 sizeof(ci->worker_rx) - 1 -
						 ci->worker_rx_len);
			if (n > 0) {
				ci->worker_rx_len += (size_t)n;
				continue;
			}
			if (n == 0 ||
			    (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK &&
			     errno != EINTR)) {
				eof = true;
			}
			break;
		}
		ci->worker_rx[ci->worker_rx_len] = '\0';

		bool changed_state = false;
		char *nl;
		while ((nl = strchr(ci->worker_rx, '\n')) != NULL) {
			*nl = '\0';
			process_worker_line(ci, ci->worker_rx, on_alert, ctx,
					    &changed_state);
			size_t rest = strlen(nl + 1);
			memmove(ci->worker_rx, nl + 1, rest + 1);
			ci->worker_rx_len = rest;
		}
		if (eof) {
			if (ci->worker_rx_len > 0) {
				process_worker_line(ci, ci->worker_rx, on_alert,
						    ctx, &changed_state);
				ci->worker_rx_len = 0;
			}
			close(ci->worker_fd);
			ci->worker_fd = -1;
			if (ci->worker_pid > 0) {
				waitpid(ci->worker_pid, NULL, 0);
				ci->worker_pid = -1;
			}
		}
		if (changed_state)
			bridge_ci_save_seen_state(ci);
	}

	uint64_t interval_ms = (uint64_t)ci->cfg.poll_interval_sec * 1000ULL;
	bool due = ci->force_poll || ci->last_poll_ms == 0 ||
		   (now_ms >= ci->last_poll_ms &&
		    (now_ms - ci->last_poll_ms) >= interval_ms);
	if (due && ci->cfg.repo_count > 0 && ci->worker_pid <= 0) {
		ci->force_poll = false;
		ci->last_poll_ms = now_ms;
		spawn_worker(ci);
	}
}

void bridge_ci_format_summary(const bridge_ci_t *ci, char *out, size_t out_size)
{
	if (!ci || !out || out_size == 0)
		return;
	out[0] = '\0';
	if (ci->cfg.repo_count == 0) {
		snprintf(out, out_size, "No GitHub repos tracked. Config: %s\n",
			 ci->config_path);
		return;
	}

	size_t off = 0;
	off += (size_t)snprintf(out + off, out_size - off,
				"GitHub CI Watcher (%s, poll=%us, repos=%zu)\n",
				ci->config_path,
				(unsigned)ci->cfg.poll_interval_sec,
				ci->cfg.repo_count);
	for (size_t i = 0; i < ci->cfg.repo_count && off + 1 < out_size; i++) {
		const bridge_ci_repo_t *r = &ci->cfg.repos[i];
		off += (size_t)snprintf(out + off, out_size - off,
					"  - %s%s%s: ", r->slug,
					r->alias[0] ? " as " : "",
					r->alias[0] ? r->alias : "");
		for (size_t b = 0; b < r->branch_count && off + 1 < out_size;
		     b++) {
			off += (size_t)snprintf(out + off, out_size - off,
						"%s%s", (b > 0) ? ", " : "",
						r->branches[b]);
		}
		if (off + 1 < out_size)
			out[off++] = '\n';
	}
	out[off] = '\0';
}
