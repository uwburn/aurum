#ifndef AU_TASK_CONTEXT_H
#define AU_TASK_CONTEXT_H

#include <stdint.h>
#include <stddef.h>

typedef uint16_t au_stack_pointer_t;

void au_context_init(
    au_stack_pointer_t *saved_sp,
    uint8_t *stack,
    size_t stack_size,
    void (*entry)(void *),
    void *context
);

void au_context_switch(au_stack_pointer_t *saved_sp, au_stack_pointer_t next_sp);

#endif