#include <stdio.h>
#include "stack.h"
#include "platform.h"
#include "exec.h"
#include "log.h"

int install_stack(const Stack *stack, int dry_run) {
    if (!stack) return -1;

    OSType os = detect_os();
    if (os == OS_UNKNOWN) {
        log_error("Unsupported OS");
        return -1;
    }

    printf("Installing stack: %s (%s)\n", stack->name, stack->id);
    if (dry_run) {
        log_info("Dry-run mode: commands will NOT be executed.");
    } else {
        log_info("Starting stack install...");
    }

    for (int i = 0; i < stack->package_count; ++i) {
        Package *p = &stack->packages[i];

        printf("\n==> %s %s (%s)\n",
               dry_run ? "[DRY-RUN] Would install" : "Installing",
               p->display_name,
               p->id);

        const char *cmd = NULL;
        if (os == OS_WINDOWS) {
            cmd = p->windows_cmd;
        } else if (os == OS_LINUX) {
            cmd = p->linux_cmd;
        }

        if (!cmd) {
            log_error("No install command for this OS");
            continue;
        }

        printf("Install command: %s\n", cmd);

        if (!dry_run) {
            if (run_command(cmd) != 0) {
                log_error("Install failed");
                continue;
            }

            if (p->verify_cmd) {
                printf("Verifying %s...\n", p->id);
                if (run_command(p->verify_cmd) == 0) {
                    printf("✔ %s OK\n", p->id);
                } else {
                    printf("✖ %s verification FAILED\n", p->id);
                }
            }
        } else {
            if (p->verify_cmd) {
                printf("Verify command (would run): %s\n", p->verify_cmd);
            }
        }
    }

    if (!dry_run) {
        log_info("Stack install finished.");
    } else {
        log_info("Dry-run finished. No changes were made.");
    }

    return 0;
}
