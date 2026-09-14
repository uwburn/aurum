#include <stdbool.h>

#include "aurum/core.h"
#include "aurum/tone.h"
#include "aurum/p_task.h"

#define TRIMMER_PIN AU_A0
#define BUTTON_PIN 2
#define LED_PIN 8
#define TONE_PIN 12
#define TONE_FREQ 500

typedef struct blink_task_context {
  uint8_t blink;
} blink_task_context_t;

blink_task_context_t blink_task_context;

typedef struct counting_task_context {
  uint16_t n;
} counting_task_context_t;

counting_task_context_t counting_task_context;

void btn_task_fn(void *context);
void analog_task_fn(void *context);
void blink_task_fn(void *context);
void counting_task_fn(void *context);
void shouting_task_fn(void *context);

au_ptask_t *btn_task;
au_ptask_t *analog_task;
au_ptask_t *blink_task;
au_ptask_t *counting_task;
au_ptask_t *shouting_task;

au_printer_t serial_printer = au_serial_build_printer();

int main() {
  au_init();

  au_serial_begin(115200, AU_SERIAL_8N1);

  au_pin_mode(BUTTON_PIN, AU_INPUT_PULLUP);
  au_pin_mode(LED_PIN, AU_OUTPUT);
  au_pin_mode(TONE_PIN, AU_OUTPUT);

  btn_task = au_ptask_create(btn_task_fn, NULL, 0);
  au_ptask_start(btn_task);

  analog_task = au_ptask_create(analog_task_fn, NULL, 0);
  au_ptask_start(analog_task);

  blink_task = au_ptask_create(blink_task_fn, &blink_task_context, 1);
  au_ptask_start(blink_task);

  counting_task = au_ptask_create(counting_task_fn, &counting_task_context, 2);
  au_ptask_start(counting_task);

  shouting_task = au_ptask_create(shouting_task_fn, NULL, 2);
  au_ptask_start(shouting_task);

  au_pscheduler_run();

  for (;;);

  return 0;
}

void btn_task_fn(void *context) {
  (void)context;

  for (;;) {
    au_ptask_atomic_enter();
    uint8_t btn = au_digital_read(2);
    au_print_str(&serial_printer, "btn=");
    au_println_int(&serial_printer, btn);
    au_ptask_atomic_exit();

    au_ptask_delay(100);
  }
}

void analog_task_fn(void *context) {
  (void)context;

  for (;;) {
    au_ptask_atomic_enter();
    uint16_t analog = au_analog_read(TRIMMER_PIN);
    au_print_str(&serial_printer, "analog=");
    au_println_int(&serial_printer, analog);
    au_ptask_atomic_exit();

    au_ptask_delay(100);
  }
}

void blink_task_fn(void *context) {
  blink_task_context_t *blink_context = context;

  for (;;) {
    au_ptask_atomic_enter();
    au_digital_write(LED_PIN, blink_context->blink);
    au_tone(TONE_PIN, TONE_FREQ + blink_context->blink * TONE_FREQ, -1);
    blink_context->blink = !blink_context->blink;
    au_ptask_atomic_exit();

    au_ptask_delay(500);
  }
}

void counting_task_fn(void *context) {
  counting_task_context_t *counting_context = context;

  for (;;) {
    au_ptask_atomic_enter();
    au_print_str(&serial_printer, "n=");
    au_print_int(&serial_printer, counting_context->n++);
    au_print_str(&serial_printer, ", stack available=");
    au_println_int(&serial_printer, au_ptask_stack_available());
    au_ptask_atomic_exit();
  }
}

void shouting_task_fn(void *context) {
  (void)context;

  for (;;) {
    au_ptask_atomic_enter();
    au_println_str(&serial_printer, "SHOUT!!!");
    au_ptask_atomic_exit();
  }
}