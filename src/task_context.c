#include <avr/io.h>

#include "task_context.h"

void au_context_init(
    au_stack_pointer_t *saved_sp,
    uint8_t *stack,
    size_t stack_size,
    void (*entry)(void *),
    void *context
) {
  uint8_t *sp = stack + stack_size - 1;      /* ultimo byte utilizzabile */
  uint16_t pc  = (uint16_t)(uintptr_t)entry;
  uint16_t ctx = (uint16_t)(uintptr_t)context;

  *sp-- = (uint8_t)(pc & 0xFF);              /* return address LOW  */
  *sp-- = (uint8_t)(pc >> 8);                /* return address HIGH */

  for (uint8_t i = 0; i < 16; i++) {         /* r2 ... r17 */
    *sp-- = 0;
  }

  *sp-- = (uint8_t)(ctx & 0xFF);             /* r24 = context LOW  */
  *sp-- = (uint8_t)(ctx >> 8);               /* r25 = context HIGH */
  *sp-- = 0;                                 /* r28 */
  *sp-- = 0;                                 /* r29 */

  *saved_sp = (au_stack_pointer_t)(uintptr_t)sp;
}