#include <stdbool.h>

#include "aurum/core.h"
#include "aurum/tone.h"

#define TRIMMER_PIN AU_A0
#define BUTTON_PIN 2
#define LED_PIN 8
#define TONE_PIN 12
#define TONE_FREQ 500

au_printer_t serial_printer = au_serial_build_printer();

void setup();
void loop();

int main(void) {
  au_init();

  setup();

  for (;;) {
    loop();
  }

  return 0;
}

void setup() {
  au_serial_begin(115200, AU_SERIAL_8N1);

  au_pin_mode(BUTTON_PIN, AU_INPUT_PULLUP);
  au_pin_mode(LED_PIN, AU_OUTPUT);
  au_pin_mode(TONE_PIN, AU_OUTPUT);
}

uint8_t led = 0;

void loop() {
  uint8_t btn = au_digital_read(2);
  au_print_str(&serial_printer, "btn=");
  au_println_int(&serial_printer, btn);

  uint16_t analog = au_analog_read(TRIMMER_PIN);
  au_print_str(&serial_printer, "analog=");
  au_println_int(&serial_printer, analog);

  au_digital_write(LED_PIN, led);
  au_tone(TONE_PIN, TONE_FREQ + led * TONE_FREQ, -1);
  led = !led;

  au_delay(500);
}