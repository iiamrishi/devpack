#ifndef STACK_LIST_H
#define STACK_LIST_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    const char *name;
    bool (*detect_fn)(char *details, size_t details_size);
} DevStack;

/* list all detected stacks, human-readable, return 0 on success */
int list_stacks(void);

/* list all detected stacks as JSON, return 0 on success */
int list_stacks_json(void);

bool detect_c_toolchain(char *details, size_t details_size);
bool detect_python(char *details, size_t details_size);
bool detect_git(char *details, size_t details_size);
bool detect_nodejs(char *details, size_t details_size);
bool detect_docker(char *details, size_t details_size);
bool detect_rust(char *details, size_t details_size);

#endif
