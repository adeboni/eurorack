#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/analog_microphone.h"
#include "usb_microphone.h"
#include "tusb.h"

#define HID_UPDATE_MS 10

const struct analog_microphone_config config = {
    .gpio = 26,
    .bias_voltage = 1.25,
    .sample_rate = SAMPLE_RATE,
    .sample_buffer_size = SAMPLE_BUFFER_SIZE,
};

uint8_t BUTTON_IO[7] = { 10, 11, 12, 18, 19, 20, 21 };
uint8_t BUTTON_KEYS[7] = { HID_KEY_1, HID_KEY_2, HID_KEY_3, HID_KEY_4, HID_KEY_5, HID_KEY_6, HID_KEY_7 };
int16_t sample_buffer[SAMPLE_BUFFER_SIZE];

uint32_t millis() {
  return to_ms_since_boot(get_absolute_time());
}

void _tud_task() {
  do {
      tud_task();
  } while (!tud_hid_ready());
}

void on_usb_microphone_tx_ready() {
  usb_microphone_write(sample_buffer, sizeof(sample_buffer));
}

void on_analog_samples_ready() {
  analog_microphone_read(sample_buffer, SAMPLE_BUFFER_SIZE);
}

void key_task(uint8_t scancode) {
  uint8_t keys[6] = { scancode };
  _tud_task();
  tud_hid_keyboard_report(1, 0, keys);
  _tud_task();
  tud_hid_keyboard_report(1, 0, NULL);
}

int main() {
  for (int i = 0; i < 7; i++) {
    gpio_init(BUTTON_IO[i]);
    gpio_set_dir(BUTTON_IO[i], GPIO_IN);
    gpio_pull_up(BUTTON_IO[i]);
  }

  analog_microphone_init(&config);
  analog_microphone_set_samples_ready_handler(on_analog_samples_ready);
  analog_microphone_start();

  tusb_init();
  usb_microphone_init();
  usb_microphone_set_tx_ready_handler(on_usb_microphone_tx_ready);

  uint8_t button_states[7] = { 0, 0, 0, 0, 0, 0, 0 };
  uint32_t start_ms = 0;

  while (1) {
    tud_task();
    
    if (millis() - start_ms > HID_UPDATE_MS) { 
        start_ms = millis();

        for (int i = 0; i < 7; i++) {
            uint8_t state = 1 - gpio_get(BUTTON_IO[i]);
            if (button_states[i] == 0 && state == 1) 
                key_task(BUTTON_KEYS[i]);
            button_states[i] = state;
        }
    }
  }
}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len) {
    (void) instance;
    (void) report;
    (void) len;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;
    return 0;
}