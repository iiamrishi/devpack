#ifndef STACK_LOADER_H
#define STACK_LOADER_H

#include "stack.h"

/* Load stacks/<stack_id>.json into `out`.
 * Returns 0 on success, non-zero on error.
 */
int load_stack_from_file(const char *stack_id, Stack *out);

#endif
