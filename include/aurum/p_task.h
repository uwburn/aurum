#ifndef AU_P_TASK_H
#define AU_P_TASK_H

#include <stdint.h>
#include <stddef.h>

#include "aurum/core.h"

#ifndef AU_PTASK_CAPACITY
#define AU_PTASK_CAPACITY 4
#endif

#ifndef AU_PTASK_STACK_SIZE
#define AU_PTASK_STACK_SIZE 192
#endif

#ifndef AU_PTASK_TIME_SLICE_MS
#define AU_PTASK_TIME_SLICE_MS 1
#endif

/*
* PREEMPTIVE TASK SCHEDULER
*
* Preemptive task scheduler. be sure to set appropriate values of
* AU_PTASK_CAPACITY and AU_PTASK_STACK_SIZE to balance memory usage.
*/

typedef void (*au_ptask_fn)(void *context);

typedef struct au_ptask au_ptask_t;

au_ptask_t *au_ptask_create(au_ptask_fn fn, void *context, uint8_t priority);
void au_ptask_remove(au_ptask_t *task);

au_ptask_t *au_ptask_current();
uint16_t au_ptask_stack_occupied();
uint16_t au_ptask_stack_available();

uint8_t au_ptask_get_priority(const au_ptask_t *task);
void au_ptask_set_priority(au_ptask_t *task, uint8_t priority);

void au_ptask_start(au_ptask_t *task);
void au_ptask_stop(au_ptask_t *task);

void au_pscheduler_run();

void au_ptask_yield();

void au_ptask_delay(uint32_t ms);
void au_ptask_delay_microseconds(uint32_t us);

#endif
