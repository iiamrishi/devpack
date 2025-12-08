#ifndef STACK_LOADER_H
#define STACK_LOADER_H

#include "stack.h"

/* Loads stacks/<stack_id>.json into out.
 * Returns 0 on success, non-zero on error.
 */
int load_stack_from_file(const char *stack_id, Stack *out);

/* List all stacks defined in the ./stacks directory.
 * Returns 0 on success, non-zero on error.
 */
int list_available_stacks(void);
/* List stacks as JSON. */
int list_available_stacks_json(void);
#endif /* STACK_LOADER_H */
