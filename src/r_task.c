#include <stdbool.h>

#include "aurum/r_task.h"
#include "task_context.h"

typedef enum {
  AU_RTASK_UNUSED,
  AU_RTASK_READY,
  AU_RTASK_RUNNING,
  AU_RTASK_WAITING,
  AU_RTASK_TERMINATED
} au_rtask_state_t;

struct au_rtask {
  au_rtask_fn fn;
  void *context;

  uint8_t stack[AU_RTASK_STACK_SIZE];
  au_stack_pointer_t saved_sp;

  uint32_t wakeup_time;

  au_rtask_state_t state;
};

static au_rtask_t tasks[AU_RTASK_CAPACITY];
static au_rtask_t *current_task;
static size_t last_task_index = 0;
static au_stack_pointer_t scheduler_saved_sp;

static void au_rtask_trampoline(void *context) {
  au_rtask_t *task = context;

  task->state = AU_RTASK_RUNNING;

  task->fn(task->context);

  task->state = AU_RTASK_TERMINATED;

  au_context_switch(&task->saved_sp, scheduler_saved_sp);
}

au_rtask_t *au_rtask_create(au_rtask_fn fn, void *context) {
  if (fn == NULL) {
    return NULL;
  }

  for (size_t i = 0; i < AU_RTASK_CAPACITY; i++) {
    au_rtask_t *task = &tasks[i];

    if (task->state != AU_RTASK_UNUSED) {
      continue;
    }

    task->fn = fn;
    task->context = context;

    au_context_init(
      &task->saved_sp,
      task->stack,
      AU_RTASK_STACK_SIZE,
      au_rtask_trampoline,
      task
    );

    task->wakeup_time = 0;
    task->state = AU_RTASK_READY;

    return task;
  }

  return NULL;
}

void au_rscheduler_run() {
  for (;;) {
    au_rtask_t *next = NULL;
    bool alive = false;

    for (size_t k = 1; k <= AU_RTASK_CAPACITY; k++) {
      size_t i = (last_task_index + k) % AU_RTASK_CAPACITY;
      au_rtask_t *task = &tasks[i];

      if (task->state == AU_RTASK_UNUSED || task->state == AU_RTASK_TERMINATED) {
        continue;
      }
      alive = true;

      if (task->state == AU_RTASK_WAITING &&
          (int32_t)(au_micros() - task->wakeup_time) >= 0) {
        task->state = AU_RTASK_READY;
      }
      if (task->state == AU_RTASK_READY) {
        next = task;
        last_task_index = i;
        break;
      }
    }

    if (!alive) return;
    if (next == NULL) continue;      /* tutti in attesa: idle */

    current_task = next;
    next->state = AU_RTASK_RUNNING;
    au_context_switch(&scheduler_saved_sp, next->saved_sp);
    current_task = NULL;
  }
}

void au_rtask_yield(void) {
  if (current_task == NULL) {
    return;
  }

  current_task->state = AU_RTASK_READY;

  au_context_switch(&current_task->saved_sp, scheduler_saved_sp);
}

void au_rtask_delay(uint32_t ms) {
  au_rtask_delay_microseconds(ms * 1000UL);
}

void au_rtask_delay_microseconds(uint32_t us) {
  if (current_task == NULL) {
    return;
  }

  if (us == 0) {
    au_rtask_yield();
    return;
  }

  current_task->wakeup_time = au_micros() + us;
  current_task->state = AU_RTASK_WAITING;

  au_context_switch(&current_task->saved_sp, scheduler_saved_sp);
}