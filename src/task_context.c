#include <avr/io.h>

#include "task_context.h"

void au_context_init(
    au_stack_pointer_t *saved_sp,
    uint8_t *stack,
    size_t stack_size,
    void (*entry)(void *),
    void *context
) {
  uint8_t *sp = stack + stack_size - 1;
  uint16_t pc  = (uint16_t)(uintptr_t)entry;
  uint16_t ctx = (uint16_t)(uintptr_t)context;

  *sp-- = (uint8_t)(pc & 0xFF);   /* return address LOW  */
  *sp-- = (uint8_t)(pc >> 8);     /* return address HIGH */
  *sp-- = 0;                      /* r0   */
  *sp-- = 0x80;                   /* SREG */
  *sp-- = 0;                      /* r1 deve valere 0 per avr-gcc */

  for (uint8_t r = 2; r <= 31; r++) {
    uint8_t v = 0;
    if (r == 24) v = (uint8_t)(ctx & 0xFF);
    if (r == 25) v = (uint8_t)(ctx >> 8);
    *sp-- = v;
  }

  *saved_sp = (au_stack_pointer_t)(uintptr_t)sp;
}