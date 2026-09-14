#ifndef AU_TASK_CONTEXT_H
#define AU_TASK_CONTEXT_H

#include <stdint.h>
#include <stddef.h>

typedef uint16_t au_stack_pointer_t;

/* Constructs a valid stack frame for a task */
void au_context_init(
    au_stack_pointer_t *saved_sp,
    uint8_t *stack,
    size_t stack_size,
    void (*entry)(void *),
    void *context
);

/* This function must be called with interrupts disabled.
* 
* When calling context_switch, the following happens:
* - Upon CALL, the caller pushes the return address onto its own stack
*   (the currently active stack, about to be suspended)
* - General-purpose registers and SREG are pushed on top of it
* - The current stack pointer is saved in *saved_sp
* - The stack pointer is updated with next_sp, actually moving to the target
*   stack frame
* - Registers of the target stack frame are popped from the stack
* - The return address is now the next instruction where execution was
*   suspended
*/
void au_context_switch(au_stack_pointer_t *saved_sp, au_stack_pointer_t next_sp);

#endif