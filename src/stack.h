#ifndef STACK_H
#define STACK_H

typedef struct {
    char *id;
    char *display_name;
    char *windows_cmd;
    char *linux_cmd;
    char *verify_cmd;
} Package;

typedef struct {
    char *id;
    char *name;
    int package_count;
    Package *packages;
} Stack;

/**
 * Install all packages in a stack.
 * If dry_run != 0, commands are only printed, not executed.
 */
int install_stack(const Stack *stack, int dry_run);

#endif
