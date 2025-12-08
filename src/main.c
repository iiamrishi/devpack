#include <stdio.h>
#include <string.h>

#include "stack.h"
#include "stack_loader.h"
#include "stack_list.h"

static void print_usage(const char *prog) {
    printf("devpack – simple dev environment installer\n\n");
    printf("Usage:\n");
    printf("  %s list\n", prog);
    printf("  %s install <stack-id> [--dry-run]\n", prog);
    printf("  %s verify <stack-id>\n", prog);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *cmd = argv[1];

    /* -------- list -------- */
    if (strcmp(cmd, "list") == 0) {
        return list_stacks();
    }

    /* -------- install -------- */
    if (strcmp(cmd, "install") == 0) {
        if (argc < 3) {
            print_usage(argv[0]);
            return 1;
        }

        const char *stack_id = argv[2];
        int dry_run = (argc >= 4 && strcmp(argv[3], "--dry-run") == 0);

        Stack stack;
        if (load_stack_from_file(stack_id, &stack) != 0) {
            fprintf(stderr, "Failed to load stack '%s'\n", stack_id);
            return 1;
        }

        int rc = install_stack(&stack, dry_run);
        free_stack(&stack);
        return rc;
    }

    /* -------- verify -------- */
    if (strcmp(cmd, "verify") == 0) {
        if (argc < 3) {
            print_usage(argv[0]);
            return 1;
        }

        const char *stack_id = argv[2];

        Stack stack;
        if (load_stack_from_file(stack_id, &stack) != 0) {
            fprintf(stderr, "Failed to load stack '%s'\n", stack_id);
            return 1;
        }

        int rc = verify_stack(&stack);
        free_stack(&stack);
        return rc;
    }

    /* -------- unknown -------- */
    fprintf(stderr, "Unknown command: %s\n\n", cmd);
    print_usage(argv[0]);
    return 1;
}
