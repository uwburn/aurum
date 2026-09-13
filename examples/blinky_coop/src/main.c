#include <stdbool.h>

#include "aurum/core.h"
#include "aurum/tone.h"
#include "aurum/c_task.h"

#define TRIMMER_PIN AU_A0
#define BUTTON_PIN 2
#define LED_PIN 8
#define TONE_PIN 12
#define TONE_FREQ 500

typedef struct blink_task_context {
  uint8_t blink;
} blink_task_context_t;

blink_task_context_t blink_task_context;

void btn_task_fn(void *context);
void analog_task_fn(void *context);
void blink_task_fn(void *context);

au_ctask_t *btn_task;
au_ctask_t *analog_task;
au_ctask_t *blink_task;

au_printer_t serial_printer = au_serial_build_printer();

int main(void) {
  au_init();

  au_serial_begin(115200, AU_SERIAL_8N1);

  au_pin_mode(BUTTON_PIN, AU_INPUT_PULLUP);
  au_pin_mode(LED_PIN, AU_OUTPUT);
  au_pin_mode(TONE_PIN, AU_OUTPUT);

  btn_task = au_ctask_add(btn_task_fn, NULL, 0, 100);
  au_ctask_start(btn_task);

  analog_task = au_ctask_add(analog_task_fn, NULL, 0, 100);
  au_ctask_start(analog_task);
  
  blink_task = au_ctask_add(blink_task_fn, &blink_task_context, 0, 500);
  au_ctask_start(blink_task);

  au_cscheduler_loop();

  return 0;
}

void btn_task_fn(void *context) {
  (void)context;

  uint8_t btn = au_digital_read(2);
  au_print_str(&serial_printer, "btn=");
  au_println_int(&serial_printer, btn);
}

void analog_task_fn(void *context) {
  (void)context;

  uint16_t analog = au_analog_read(TRIMMER_PIN);
  au_print_str(&serial_printer, "analog=");
  au_println_int(&serial_printer, analog);
}

void blink_task_fn(void *context) {
  blink_task_context_t *blink_context = (blink_task_context_t*)context;

  au_digital_write(LED_PIN, blink_context->blink);
  au_tone(TONE_PIN, TONE_FREQ + blink_context->blink * TONE_FREQ, -1);
  blink_context->blink = !blink_context->blink;
}