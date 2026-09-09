#ifndef AU_COOP_TASK_H
#define AU_COOP_TASK_H

#ifndef AU_TASK_CAPACITY
#define AU_TASK_CAPACITY 8
#endif

/*
 * COOPERATIVE TASK SCHEDULER
 *
 * Cooperative callback scheduler. All times are expressed in milliseconds.
 */

#include <stdint.h>
#include <stddef.h>

#include "aurum/core.h"

typedef void (*au_task_fn)(void *context);

typedef struct au_task au_task_t;

au_task_t *au_task_add(au_task_fn fn, void *context, uint32_t delay, uint32_t period);
void au_task_remove(au_task_t *task);

void au_task_start(au_task_t *task);
void au_task_stop(au_task_t *task);
void au_task_run_at(au_task_t *task, uint32_t run_at);

void au_scheduler_run();
void au_scheduler_loop();

#endif