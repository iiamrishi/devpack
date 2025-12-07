#ifndef STACK_H
#define STACK_H

typedef struct {
    const char *id;
    const char *display_name;
    const char *windows_cmd;
    const char *linux_cmd;
    const char *verify_cmd;
} Package;

typedef struct {
    const char *id;
    const char *name;
    int package_count;
    Package *packages;
} Stack;

/**
 * Install all packages in a stack.
 * If dry_run != 0, commands are only printed, not executed.
 */
int install_stack(const Stack *stack, int dry_run);

#endif
