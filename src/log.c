#include <stdio.h>
#include "log.h"

void log_info(const char *msg) {
    printf("[info] %s\n", msg);
}

void log_error(const char *msg) {
    fprintf(stderr, "[error] %s\n", msg);
}
