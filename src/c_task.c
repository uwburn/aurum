#include "aurum/c_task.h"

typedef enum {
  AU_TASK_UNUSED = 0,
  AU_TASK_STOPPED,
  AU_TASK_RUNNING
} au_task_state_t;

struct au_ctask {
  au_ctask_fn fn;
  void *context;
  au_task_state_t state;
  uint32_t delay;
  uint32_t period;
  uint32_t run_at;
};

static au_ctask_t tasks[AU_CTASK_CAPACITY];

au_ctask_t *au_ctask_add(au_ctask_fn fn, void *context, uint32_t delay, uint32_t period) {
  for (uint8_t i = 0; i < AU_CTASK_CAPACITY; i++) {
    au_ctask_t *task = &tasks[i];

    if (task->state != AU_TASK_UNUSED) {
        continue;
    }

    task->fn = fn;
    task->context = context;
    task->state = AU_TASK_STOPPED;
    task->delay = delay;
    task->period = period;
    task->run_at = 0;

    return task;
  }

  return NULL;
}

void au_ctask_remove(au_ctask_t *task) {
  if (task == NULL) {
    return;
  }

  task->fn = NULL;
  task->context = NULL;
  task->delay = 0;
  task->period = 0;
  task->run_at = 0;
  task->state = AU_TASK_UNUSED;
}

void au_ctask_start(au_ctask_t *task){
  if (task == NULL) {
    return;
  }

  if (task->state == AU_TASK_RUNNING) {
    return;
  }

  task->run_at = au_millis() + task->delay;
  task->state = AU_TASK_RUNNING;
}

void au_ctask_stop(au_ctask_t *task) {
  if (task == NULL) {
    return;
  }

  task->state = AU_TASK_STOPPED;
}

void au_ctask_run_at(au_ctask_t *task, uint32_t run_at) {
  if (task == NULL) {
    return;
  }

  task->run_at = run_at;
  task->state = AU_TASK_RUNNING;
}

void au_cscheduler_run() {
  uint32_t now = au_millis();

  for (uint8_t i = 0; i < AU_CTASK_CAPACITY; i++) {
    au_ctask_t *task = &tasks[i];

    if (task->state != AU_TASK_RUNNING) {
      continue;
    }

    if ((int32_t)(now - task->run_at) < 0) {
      continue;
    }

    uint32_t run_at = task->run_at;
    task->fn(task->context);

    if (task->state != AU_TASK_RUNNING) {
      continue;
    }

    if (task->run_at != run_at) {
      continue;
    }

    if (task->period == 0) {
      task->state = AU_TASK_STOPPED;
      continue;
    }

    do {
      task->run_at += task->period;
    } while ((int32_t)(now - task->run_at) >= 0);
  }
}

void au_cscheduler_loop() {
  for (;;) {
    au_cscheduler_run();
  }
}