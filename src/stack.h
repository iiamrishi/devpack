#ifndef STACK_H
#define STACK_H
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

/* Free all memory owned by a Stack (packages, strings, etc.). */
void free_stack(Stack *stack);


int verify_stack(const Stack *stack);

#endif
