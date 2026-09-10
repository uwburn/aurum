#ifndef AU_C_TASK_H
#define AU_C_TASK_H

#ifndef AU_CTASK_CAPACITY
#define AU_CTASK_CAPACITY 8
#endif

/*
* COOPERATIVE TASK SCHEDULER
*
* Cooperative callback scheduler. All times are expressed in milliseconds.
* Tasks are added in stopped state, they must be started with au_ctask_start.
* Scheduling must be triggered by repeatdly calling au_cscheduler_run or by
* calling once au_cscheduler_loop, which never returns.
*/

#include <stdint.h>
#include <stddef.h>

#include "aurum/core.h"

typedef void (*au_ctask_fn)(void *context);

typedef struct au_ctask au_ctask_t;

au_ctask_t *au_ctask_add(au_ctask_fn fn, void *context, uint32_t delay, uint32_t period);
void au_ctask_remove(au_ctask_t *task);

void au_ctask_start(au_ctask_t *task);
void au_ctask_stop(au_ctask_t *task);
void au_ctask_run_at(au_ctask_t *task, uint32_t run_at);

void au_cscheduler_run();
void au_cscheduler_loop();

#endif