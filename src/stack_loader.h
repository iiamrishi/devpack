#ifndef STACK_LOADER_H
#define STACK_LOADER_H

#include "stack.h"

/**
 * Load a stack definition from stacks/<stack_id>.json
 * Returns 0 on success, non-zero on failure.
 */
int load_stack_from_file(const char *stack_id, Stack *out);

/**
 * Free all memory owned by a Stack loaded with load_stack_from_file.
 */
void free_stack(Stack *stack);

#endif
