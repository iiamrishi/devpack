#include "stack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int install_stack(const Stack *stack, int dry_run) {
    if (!stack) {
        return -1;
    }

    if (stack->package_count <= 0 || !stack->packages) {
        fprintf(stderr, "Stack '%s' has no packages to install\n",
                stack->id ? stack->id : "(unknown)");
        return -1;
    }

    printf("Installing stack: %s\n", stack->name ? stack->name : "(unnamed)");
    int overall_rc = 0;

    for (int i = 0; i < stack->package_count; i++) {
        const Package *p = &stack->packages[i];

        const char *cmd = NULL;

#if defined(_WIN32)
        cmd = p->windows_cmd;
#else
        cmd = p->linux_cmd;
#endif

        if (!cmd || cmd[0] == '\0') {
            printf("  [skip] %s (no command for this platform)\n",
                   p->display_name ? p->display_name : "(unnamed package)");
            continue;
        }

        printf("  Package: %s\n", p->display_name ? p->display_name : "(unnamed package)");

        if (dry_run) {
            printf("    [dry-run] %s\n", cmd);
        } else {
            printf("    [run] %s\n", cmd);
            int rc = system(cmd);
            if (rc != 0) {
                fprintf(stderr, "    Command failed with code %d\n", rc);
                if (overall_rc == 0) {
                    overall_rc = rc;
                }
            } else if (p->verify_cmd && p->verify_cmd[0] != '\0') {
                printf("    [verify] %s\n", p->verify_cmd);
                int vrc = system(p->verify_cmd);
                if (vrc != 0 && overall_rc == 0) {
                    overall_rc = vrc;
                }
            }
        }
    }

    return overall_rc;
}

void free_stack(Stack *stack) {
    if (!stack) {
        return;
    }

    free(stack->id);
    free(stack->name);

    if (stack->packages) {
        for (int i = 0; i < stack->package_count; i++) {
            Package *p = &stack->packages[i];
            free(p->id);
            free(p->display_name);
            free(p->windows_cmd);
            free(p->linux_cmd);
            free(p->verify_cmd);
        }
        free(stack->packages);
    }

    memset(stack, 0, sizeof(*stack));
}


int verify_stack(const Stack *stack)
{
    if (!stack) {
        fprintf(stderr, "verify_stack: stack is NULL\n");
        return 1;
    }

    printf("Verifying stack: %s (%s)\n",
           stack->name ? stack->name : "(no-name)",
           stack->id   ? stack->id   : "(no-id)");
    printf("Packages: %d\n\n", stack->package_count);

    int failures = 0;

    for (int i = 0; i < stack->package_count; ++i) {
        const Package *p = &stack->packages[i];

        const char *id   = p->id           ? p->id           : "(no-id)";
        const char *name = p->display_name ? p->display_name : "(no-name)";

        printf("- [%s] %s\n", id, name);

        if (!p->verify_cmd || !*p->verify_cmd) {
            printf("    (no verify_cmd, skipping)\n\n");
            continue;
        }

        printf("    $ %s\n", p->verify_cmd);
        int status = system(p->verify_cmd);
        if (status == -1) {
            printf("    -> failed to start command\n\n");
            failures++;
        } else if (status != 0) {
            printf("    -> command exited with status %d (NOT OK)\n\n", status);
            failures++;
        } else {
            printf("    -> OK\n\n");
        }
    }

    if (failures > 0) {
        printf("Verification finished with %d failed check(s).\n", failures);
        return 1;
    }

    printf("All checks passed.\n");
    return 0;
}
