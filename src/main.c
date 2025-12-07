#include <stdio.h>
#include <string.h>
#include "stack.h"

/* Hardcoded stack for now – JSON comes next */
static Package web_basic_packages[] = {
    {
        .id = "node",
        .display_name = "Node.js LTS",
        .windows_cmd = "winget install --silent OpenJS.NodeJS.LTS",
        .linux_cmd   = "sudo dnf install -y nodejs npm",
        .verify_cmd  = "node -v"
    },
    {
        .id = "git",
        .display_name = "Git",
        .windows_cmd = "winget install --silent Git.Git",
        .linux_cmd   = "sudo dnf install -y git",
        .verify_cmd  = "git --version"
    }
};

static Stack web_basic_stack = {
    .id = "web-basic",
    .name = "Web Basic Stack",
    .package_count = 2,
    .packages = web_basic_packages
};

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("devpack – simple dev environment installer (prototype)\n\n");
        printf("Usage:\n");
        printf("  %s install <stack-id>\n", argv[0]);
        printf("\nExample:\n");
        printf("  %s install web-basic\n", argv[0]);
        return 1;
    }

    const char *cmd = argv[1];
    const char *stack_id = argv[2];

    if (strcmp(cmd, "install") == 0) {
        if (strcmp(stack_id, "web-basic") == 0) {
            return install_stack(&web_basic_stack);
        } else {
            fprintf(stderr, "Unknown stack id: %s\n", stack_id);
            return 1;
        }
    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        return 1;
    }
}
