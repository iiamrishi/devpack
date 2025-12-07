#include <stdio.h>
#include <stdlib.h>
#include "exec.h"

int run_command(const char *cmd) {
    if (!cmd) return -1;

    printf("[cmd] %s\n", cmd);
    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "Command failed with code %d\n", rc);
    }
    return rc;
}
