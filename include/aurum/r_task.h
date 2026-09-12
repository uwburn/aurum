#ifndef AU_R_TASK_H
#define AU_R_TASK_H

#include <stdint.h>
#include <stddef.h>

#include "aurum/core.h"

#ifndef AU_RTASK_CAPACITY
#define AU_RTASK_CAPACITY 4
#endif

#ifndef AU_RTASK_STACK_SIZE
#define AU_RTASK_STACK_SIZE 128
#endif

/*
* COROUTINE TASK SCHEDULER
*
* Coroutine task scheduler. be sure to set appropriate values of
* AU_RTASK_CAPACITY and AU_RTASK_STACK_SIZE to balance memory usage.
*/

typedef void (*au_rtask_fn)(void *context);

typedef struct au_rtask au_rtask_t;

au_rtask_t *au_rtask_create(au_rtask_fn fn, void *context);

void au_rscheduler_run(void);

void au_rtask_yield(void);

void au_rtask_delay(uint32_t ms);
void au_rtask_delay_microseconds(uint32_t us);

#endif
