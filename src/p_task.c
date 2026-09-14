#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "aurum/p_task.h"
#include "task_context.h"
#include "aurum_private.h"

#define AU_PTASK_TIMER_PRESCALER 64UL
#define AU_PTASK_TIMER_TOP \
  (((F_CPU / AU_PTASK_TIMER_PRESCALER / 1000UL) * AU_PTASK_TIME_SLICE_MS) - 1UL)

#if AU_PTASK_PREEMPTIVE && (AU_PTASK_TIMER_TOP > 65535UL)
#error "AU_PTASK_TIME_SLICE_MS troppo grande per Timer1 con prescaler 64"
#endif

typedef enum {
  AU_PTASK_UNUSED,
  AU_PTASK_READY,
  AU_PTASK_RUNNING,
  AU_PTASK_WAITING,
  AU_PTASK_STOPPED,
  AU_PTASK_TERMINATED
} au_ptask_state_t;

struct au_ptask {
  au_ptask_fn fn;
  void *context;

  uint8_t stack[AU_PTASK_STACK_SIZE];
  au_stack_pointer_t saved_sp;

  uint32_t wakeup_time;

  au_ptask_state_t state;

  uint8_t priority;
};

static au_ptask_t tasks[AU_PTASK_CAPACITY];
static au_ptask_t *current_task;
static au_stack_pointer_t idle_saved_sp;
static size_t last_task_index = 0;

static void au_ptask_timer_init() {
#if AU_PTASK_TIME_SLICE_MS > 0
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = (uint16_t)AU_PTASK_TIMER_TOP;
  // Discard pendings
  TIFR1 = (1 << OCF1A);

  // CTC, prescaler 64
  TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);
  TIMSK1 |= (1 << OCIE1A);
#endif
}

static void au_ptask_timer_stop() {
#if AU_PTASK_TIME_SLICE_MS > 0
  TIMSK1 &= ~(1 << OCIE1A);
  TCCR1B = 0;
#endif
}

static au_ptask_t *au_pscheduler_next_task() {
  au_ptask_t *best = NULL;
  uint8_t best_index = 0;
  uint32_t now = au_micros();

  for (uint8_t k = 1; k <= AU_PTASK_CAPACITY; k++) {
    uint8_t i = (uint8_t)((last_task_index + k) % AU_PTASK_CAPACITY);
    au_ptask_t *task = &tasks[i];

    switch (task->state) {
    case AU_PTASK_WAITING:
      if ((int32_t)(now - task->wakeup_time) < 0) {
        continue;
      }
      task->state = AU_PTASK_READY;
      break;

    case AU_PTASK_READY:
    case AU_PTASK_RUNNING:
      break;

    case AU_PTASK_UNUSED:
    case AU_PTASK_STOPPED:
    case AU_PTASK_TERMINATED:
      continue;
    }

    if (best == NULL || task->priority < best->priority) {
      best = task;
      best_index = i;
    }
  }

  if (best != NULL) {
    last_task_index = best_index;
  }

  return best;
}

static bool au_pscheduler_has_pending_task() {
  for (uint8_t i = 0; i < AU_PTASK_CAPACITY; i++) {
    switch (tasks[i].state) {
    case AU_PTASK_UNUSED:
    case AU_PTASK_TERMINATED:
      break;
    default:
      return true;
    }
  }

  return false;
}

 /* Should be called with I=0 (ISR or from critical section) */
static void au_pscheduler_switch() {
  // current_task == NULL means idle context
  au_ptask_t *prev = current_task;
  au_ptask_t *next = au_pscheduler_next_task();

  if (next == NULL) {
    if (prev == NULL) {
      return;
    }

    // Nothing to do, return to idle context
    current_task = NULL;
    au_context_switch(&prev->saved_sp, idle_saved_sp);
    return;
  }

  if (next == prev) {
    next->state = AU_PTASK_RUNNING;
    return;
  }

  if (prev != NULL && prev->state == AU_PTASK_RUNNING) {
    prev->state = AU_PTASK_READY;
  }

  next->state = AU_PTASK_RUNNING;
  current_task = next;

  au_context_switch(
    prev != NULL ? &prev->saved_sp : &idle_saved_sp,
    next->saved_sp
  );
}

/* Never returns, used by terminating tasks */
static void au_pscheduler_switch_away() {
  for (;;) {
    au_pscheduler_switch();
  }
}

/* Trampoline is part of the constructed stack frame of a task, it 
ensures a valid termination path in case a task function returns */
static void au_ptask_trampoline(void *context) {
  au_ptask_t *task = context;

  task->fn(task->context);

  cli();
  task->state = AU_PTASK_TERMINATED;
  au_pscheduler_switch_away();
}

au_ptask_t *au_ptask_create(au_ptask_fn fn, void *context, uint8_t priority) {
  if (fn == NULL) {
    return NULL;
  }

  uint8_t sreg = SREG;
  cli();

  for (uint8_t i = 0; i < AU_PTASK_CAPACITY; i++) {
    au_ptask_t *task = &tasks[i];

    switch (task->state) {
    case AU_PTASK_UNUSED:
    case AU_PTASK_TERMINATED:
      break;
    default:
      continue;
    }

    task->fn = fn;
    task->context = context;
    task->wakeup_time = 0;
    task->priority = priority;

    au_context_init(
      &task->saved_sp,
      task->stack,
      AU_PTASK_STACK_SIZE,
      au_ptask_trampoline,
      task
    );

    task->state = AU_PTASK_READY;

    SREG = sreg;
    return task;
  }

  SREG = sreg;
  return NULL;
}

void au_ptask_remove(au_ptask_t *task) {
  if (task == NULL) {
    return;
  }

  uint8_t sreg = SREG;
  cli();

  if (task == current_task) {
    // Slot remains terminated untile the stack is exited
    task->state = AU_PTASK_TERMINATED;
    au_pscheduler_switch_away();
  }

  task->state = AU_PTASK_UNUSED;
  SREG = sreg;
}

au_ptask_t *au_ptask_current() {
  return current_task;
}

uint16_t au_ptask_stack_occupied() {
  if (current_task == NULL) {
    return 0;
  }

  uint16_t sp = SP;

  return (uint16_t)((uintptr_t)(current_task->stack + AU_PTASK_STACK_SIZE - 1) - sp);
}

uint16_t au_ptask_stack_available() {
  if (current_task == NULL) {
    return 0;
  }

  return (uint16_t)AU_PTASK_STACK_SIZE - au_ptask_stack_occupied();
}

uint8_t au_ptask_get_priority(const au_ptask_t *task) {
  return task != NULL ? task->priority : 0;
}

void au_ptask_set_priority(au_ptask_t *task, uint8_t priority) {
  if (task == NULL) {
    return;
  }

  uint8_t sreg = SREG;
  cli();
  task->priority = priority;
  SREG = sreg;
}

void au_ptask_start(au_ptask_t *task) {
  if (task == NULL) {
    return;
  }

  uint8_t sreg = SREG;
  cli();

  switch (task->state) {
  case AU_PTASK_STOPPED:
    task->state = AU_PTASK_READY;
    break;
  default:
    break;
  }

  SREG = sreg;
}

void au_ptask_stop(au_ptask_t *task) {
  if (task == NULL) {
    return;
  }

  uint8_t sreg = SREG;
  cli();

  switch (task->state) {
  case AU_PTASK_READY:
  case AU_PTASK_RUNNING:
  case AU_PTASK_WAITING:
    task->state = AU_PTASK_STOPPED;
    if (task == current_task) {
      // Resume from au_ptask_start
      au_pscheduler_switch();
    }
    break;
  default:
    break;
  }

  SREG = sreg;
}

void au_ptask_yield() {
  uint8_t sreg = SREG;
  cli();

  au_ptask_t *task = current_task;

  if (task != NULL) {
    if (task->state == AU_PTASK_RUNNING) {
      task->state = AU_PTASK_READY;
    }
    au_pscheduler_switch();
  }

  SREG = sreg;
}

void au_ptask_delay_microseconds(uint32_t us) {
  if (us == 0) {
    au_ptask_yield();
    return;
  }

  uint8_t sreg = SREG;
  cli();

  au_ptask_t *task = current_task;

  if (task != NULL) {
    task->wakeup_time = au_micros() + us;
    task->state = AU_PTASK_WAITING;
    au_pscheduler_switch();
  }

  SREG = sreg;
}

void au_ptask_delay(uint32_t ms) {
  au_ptask_delay_microseconds(ms * 1000UL);
}

void au_pscheduler_run() {
  au_ptask_timer_init();

  for (;;) {
    cli();

    if (!au_pscheduler_has_pending_task()) {
      sei();
      break;
    }

    au_ptask_t *next = au_pscheduler_next_task();

    if (next == NULL) {
      // Idle, no task available, wait next interrupt
      set_sleep_mode(SLEEP_MODE_IDLE);
      sleep_enable();
      sei();
      sleep_cpu();
      sleep_disable();
      continue;
    }

    current_task = next;
    next->state = AU_PTASK_RUNNING;

    // When returning current_task is NULL and I = 0
    au_context_switch(&idle_saved_sp, next->saved_sp);
  }

  au_ptask_timer_stop();
}

#if AU_PTASK_TIME_SLICE_MS > 0
ISR(TIMER1_COMPA_vect) {
  au_pscheduler_switch();
}
#endif