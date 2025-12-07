#include <stdio.h>
#include <string.h>
#include "stack.h"
#include "stack_loader.h"

static void print_usage(const char *prog) {
    printf("devpack – simple dev environment installer (prototype)\n\n");
    printf("Usage:\n");
    printf("  %s install <stack-id> [--dry-run]\n", prog);
    printf("\nExamples:\n");
    printf("  %s install web-basic\n", prog);
    printf("  %s install web-basic --dry-run\n", prog);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char *cmd = argv[1];
    const char *stack_id = argv[2];
    int dry_run = 0;

    if (argc >= 4 && strcmp(argv[3], "--dry-run") == 0) {
        dry_run = 1;
    }

    if (strcmp(cmd, "install") != 0) {
        fprintf(stderr, "Unknown command: %s\n\n", cmd);
        print_usage(argv[0]);
        return 1;
    }

    Stack stack;
    int rc = load_stack_from_file(stack_id, &stack);
    if (rc != 0) {
        fprintf(stderr, "Failed to load stack '%s'\n", stack_id);
        return 1;
    }

    rc = install_stack(&stack, dry_run);
    free_stack(&stack);

    return rc;
}
