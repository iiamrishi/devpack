#ifndef STACK_H
#define STACK_H

#include <stddef.h>

/* ANSI colors for pretty output */
#define COLOR_RESET  "\x1b[0m"
#define COLOR_RED    "\x1b[31m"
#define COLOR_GREEN  "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"

typedef struct {
    char *id;
    char *display_name;
    char *windows_cmd;
    char *linux_cmd;
    char *verify_cmd;
} Package;

typedef struct {
    char   *id;
    char   *name;

    Package *packages;
    int      package_count;

    /* Optional stack dependencies (by stack-id) */
    char  **depends_on;
    int      depends_count;
} Stack;

/* Install all packages in the stack (and dependencies).
 * dry_run != 0 → print commands but don't execute them.
 * Returns 0 on success, non-zero on any failure.
 */
int install_stack(const Stack *stack, int dry_run);

/* Verify stack (and dependencies) using verify_cmds.
 * Returns 0 on success, non-zero on any failure.
 */
int verify_stack(const Stack *stack);

/* Free all heap allocations inside the stack. */
void free_stack(Stack *stack);

#endif /* STACK_H */
