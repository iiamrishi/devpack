#include "stack.h"
#include "stack_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---------------------------------------------------------
 * Helpers
 * --------------------------------------------------------- */

static int run_install_command(const char *label,
                               const char *cmd,
                               int dry_run)
{
    if (!cmd || !*cmd) {
        printf("    " COLOR_YELLOW "(%s: no command for this platform, skipping)" COLOR_RESET "\n",
               label);
        return 0;
    }

    if (dry_run) {
        printf("    " COLOR_YELLOW "[DRY-RUN] %s: %s" COLOR_RESET "\n", label, cmd);
        return 0;
    }

    printf("    $ %s\n", cmd);
    int status = system(cmd);
    if (status == -1) {
        printf("    " COLOR_RED "-> failed to start command" COLOR_RESET "\n");
        return 1;
    }
    if (status != 0) {
        printf("    " COLOR_RED "-> command exited with status %d" COLOR_RESET "\n", status);
        return 1;
    }

    printf("    " COLOR_GREEN "-> OK" COLOR_RESET "\n");
    return 0;
}

/* -------- Package manager detection (Linux) -------- */

#if !defined(_WIN32)

static int command_exists(const char *name)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "command -v %s 2>/dev/null", name);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return 0;
    }

    char buf[64];
    int ok = (fgets(buf, sizeof(buf), fp) != NULL);
    pclose(fp);
    return ok;
}

static const char *detect_package_manager(void)
{
    static const char *pm = NULL;
    static int inited = 0;

    if (inited) return pm;
    inited = 1;

    if (command_exists("pacman")) pm = "pacman";
    else if (command_exists("apt")) pm = "apt";
    else if (command_exists("dnf")) pm = "dnf";
    else if (command_exists("yum")) pm = "yum";
    else if (command_exists("zypper")) pm = "zypper";
    else if (command_exists("brew")) pm = "brew";
    else pm = NULL;

    return pm;
}

/* linux_cmd format (optional advanced mode):
 *
 *   "pacman: sudo pacman -S foo | apt: sudo apt install foo | dnf: sudo dnf install foo"
 *
 * If no "pm:" prefixes are found, the string is used as-is.
 */
static const char *resolve_linux_cmd(const char *raw_cmd)
{
    static char buffer[1024];

    if (!raw_cmd || !*raw_cmd) {
        return raw_cmd;
    }

    const char *pm = detect_package_manager();
    if (!pm) {
        /* No known package manager detected, just use the raw string */
        return raw_cmd;
    }

    size_t pm_len = strlen(pm);
    const char *p = raw_cmd;

    while (*p) {
        /* skip leading separators and whitespace */
        while (*p == ' ' || *p == '\t' || *p == '|')
            p++;

        if (!*p) break;

        /* find prefix end: "tag: ..." */
        const char *colon = strchr(p, ':');
        if (!colon) {
            /* no "tag:" -> this is not a multi-variant string; fallback */
            return raw_cmd;
        }

        size_t tag_len = (size_t)(colon - p);

        /* compare tag with pm name */
        if (tag_len == pm_len && strncmp(p, pm, pm_len) == 0) {
            /* matched, extract command after "tag:" until '|' or end */
            const char *cmd_start = colon + 1;
            while (*cmd_start == ' ' || *cmd_start == '\t')
                cmd_start++;

            const char *cmd_end = strchr(cmd_start, '|');
            size_t copy_len = cmd_end ? (size_t)(cmd_end - cmd_start)
                                      : strlen(cmd_start);

            if (copy_len >= sizeof(buffer))
                copy_len = sizeof(buffer) - 1;

            memcpy(buffer, cmd_start, copy_len);
            buffer[copy_len] = '\0';
            return buffer;
        } else {
            /* skip to next '|' */
            const char *next_sep = strchr(colon + 1, '|');
            if (!next_sep)
                break;
            p = next_sep + 1;
        }
    }

    /* No matching tag found -> fallback to raw string */
    return raw_cmd;
}

#endif /* !defined(_WIN32) */

/* To avoid infinite recursion, put a simple depth limit. */
#define MAX_STACK_DEPTH 16

static int install_stack_internal(const Stack *stack, int dry_run, int depth);
static int verify_stack_internal(const Stack *stack, int depth);

/* ---------------------------------------------------------
 * Public API
 * --------------------------------------------------------- */

int install_stack(const Stack *stack, int dry_run)
{
    return install_stack_internal(stack, dry_run, 0);
}

int verify_stack(const Stack *stack)
{
    return verify_stack_internal(stack, 0);
}

/* ---------------------------------------------------------
 * Implementation: install with dependencies
 * --------------------------------------------------------- */

static int install_stack_internal(const Stack *stack, int dry_run, int depth)
{
    if (!stack) {
        fprintf(stderr, "install_stack: stack is NULL\n");
        return 1;
    }

    if (depth > MAX_STACK_DEPTH) {
        fprintf(stderr,
                COLOR_RED "install_stack: maximum dependency depth exceeded" COLOR_RESET "\n");
        return 1;
    }

    printf(COLOR_YELLOW "Installing stack: %s (%s)" COLOR_RESET "\n",
           stack->name ? stack->name : "(no-name)",
           stack->id   ? stack->id   : "(no-id)");
    printf("Packages: %d\n", stack->package_count);

    int failures = 0;

    /* ---- Dependencies first ---- */
    if (stack->depends_count > 0 && stack->depends_on) {
        printf(COLOR_YELLOW "Resolving dependencies (%d):" COLOR_RESET "\n",
               stack->depends_count);

        for (int i = 0; i < stack->depends_count; ++i) {
            const char *dep_id = stack->depends_on[i];
            if (!dep_id || !*dep_id) continue;

            /* Prevent trivial self-dependency loops */
            if (stack->id && strcmp(stack->id, dep_id) == 0) {
                printf("  " COLOR_RED "Skipping self-dependency '%s'" COLOR_RESET "\n", dep_id);
                failures++;
                continue;
            }

            printf("  -> %s\n", dep_id);

            Stack dep;
            if (load_stack_from_file(dep_id, &dep) != 0) {
                printf("    " COLOR_RED "Failed to load dependency '%s'" COLOR_RESET "\n", dep_id);
                failures++;
                continue;
            }

            int rc = install_stack_internal(&dep, dry_run, depth + 1);
            free_stack(&dep);

            if (rc != 0) {
                printf("    " COLOR_RED "Dependency '%s' not installed correctly" COLOR_RESET "\n",
                       dep_id);
                failures++;
            } else {
                printf("    " COLOR_GREEN "Dependency '%s' OK" COLOR_RESET "\n", dep_id);
            }
        }

        if (failures > 0) {
            printf(COLOR_RED "Aborting installation of '%s' due to dependency failures."
                   COLOR_RESET "\n",
                   stack->id ? stack->id : "(stack)");
            return 1;
        }

        printf("\n");
    }

    /* ---- Now install this stack's packages ---- */
    for (int i = 0; i < stack->package_count; ++i) {
        const Package *p = &stack->packages[i];

        const char *id   = p->id           ? p->id           : "(no-id)";
        const char *name = p->display_name ? p->display_name : "(no-name)";

        printf("- [%s] %s\n", id, name);

    #if defined(_WIN32)
        const char *install_cmd = p->windows_cmd;
    #else
        const char *install_cmd = resolve_linux_cmd(p->linux_cmd);
    #endif

        if (run_install_command("install", install_cmd, dry_run) != 0) {
            failures++;
        }

        if (p->verify_cmd && *p->verify_cmd) {
            if (run_install_command("verify", p->verify_cmd, dry_run) != 0) {
                failures++;
            }
        }

        printf("\n");
    }

    if (failures > 0) {
        printf(COLOR_RED "Finished with %d failed step(s)." COLOR_RESET "\n", failures);
        return 1;
    }

    printf(COLOR_GREEN "All steps completed successfully." COLOR_RESET "\n");
    return 0;
}

/* ---------------------------------------------------------
 * Implementation: verify with dependencies
 * --------------------------------------------------------- */

static int verify_stack_internal(const Stack *stack, int depth)
{
    if (!stack) {
        fprintf(stderr, "verify_stack: stack is NULL\n");
        return 1;
    }

    if (depth > MAX_STACK_DEPTH) {
        fprintf(stderr,
                COLOR_RED "verify_stack: maximum dependency depth exceeded" COLOR_RESET "\n");
        return 1;
    }

    printf(COLOR_YELLOW "Verifying stack: %s (%s)" COLOR_RESET "\n",
           stack->name ? stack->name : "(no-name)",
           stack->id   ? stack->id   : "(no-id)");
    printf("Packages: %d\n\n", stack->package_count);

    int failures = 0;

    /* ---- Dependencies first ---- */
    if (stack->depends_count > 0 && stack->depends_on) {
        printf(COLOR_YELLOW "Verifying dependencies (%d):" COLOR_RESET "\n",
               stack->depends_count);

        for (int i = 0; i < stack->depends_count; ++i) {
            const char *dep_id = stack->depends_on[i];
            if (!dep_id || !*dep_id) continue;

            if (stack->id && strcmp(stack->id, dep_id) == 0) {
                printf("  " COLOR_RED "Skipping self-dependency '%s'" COLOR_RESET "\n", dep_id);
                failures++;
                continue;
            }

            printf("  -> %s\n", dep_id);

            Stack dep;
            if (load_stack_from_file(dep_id, &dep) != 0) {
                printf("    " COLOR_RED "Failed to load dependency '%s'" COLOR_RESET "\n", dep_id);
                failures++;
                continue;
            }

            int rc = verify_stack_internal(&dep, depth + 1);
            free_stack(&dep);

            if (rc != 0) {
                printf("    " COLOR_RED "Dependency '%s' NOT OK" COLOR_RESET "\n", dep_id);
                failures++;
            } else {
                printf("    " COLOR_GREEN "Dependency '%s' OK" COLOR_RESET "\n", dep_id);
            }
        }

        if (failures > 0) {
            printf(COLOR_RED
                   "Verification aborted: one or more dependencies are not satisfied."
                   COLOR_RESET "\n\n");
            return 1;
        }

        printf("\n");
    }

    /* ---- Now verify this stack's own packages ---- */
    for (int i = 0; i < stack->package_count; ++i) {
        const Package *p = &stack->packages[i];

        const char *id   = p->id           ? p->id           : "(no-id)";
        const char *name = p->display_name ? p->display_name : "(no-name)";

        printf("- [%s] %s\n", id, name);

        if (!p->verify_cmd || !*p->verify_cmd) {
            printf("    " COLOR_YELLOW "(no verify_cmd, skipping)" COLOR_RESET "\n\n");
            continue;
        }

        printf("    $ %s\n", p->verify_cmd);
        int status = system(p->verify_cmd);
        if (status == -1) {
            printf("    " COLOR_RED "-> failed to start command" COLOR_RESET "\n\n");
            failures++;
        } else if (status != 0) {
            printf("    " COLOR_RED "-> command exited with status %d (NOT OK)" COLOR_RESET "\n\n",
                   status);
            failures++;
        } else {
            printf("    " COLOR_GREEN "-> OK" COLOR_RESET "\n\n");
        }
    }

    if (failures > 0) {
        printf(COLOR_RED "Verification finished with %d failed check(s)." COLOR_RESET "\n",
               failures);
        return 1;
    }

    printf(COLOR_GREEN "All checks passed." COLOR_RESET "\n");
    return 0;
}

/* ---------------------------------------------------------
 * Free stack
 * --------------------------------------------------------- */

void free_stack(Stack *s)
{
    if (!s) return;

    free(s->id);
    free(s->name);

    if (s->packages) {
        for (int i = 0; i < s->package_count; ++i) {
            Package *p = &s->packages[i];
            free(p->id);
            free(p->display_name);
            free(p->windows_cmd);
            free(p->linux_cmd);
            free(p->verify_cmd);
        }
        free(s->packages);
    }

    if (s->depends_on) {
        for (int i = 0; i < s->depends_count; ++i) {
            free(s->depends_on[i]);
        }
        free(s->depends_on);
    }

    memset(s, 0, sizeof(*s));
}
