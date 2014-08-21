#ifndef FD_LED_H
#define FD_LED_H

#include "fd_hal_ui.h"

#include <stdint.h>

void fd_led_initialize(void);

void fd_led_wake(void);
void fd_led_sleep(void);

void fd_led_override(fd_hal_ui_led_state_t *state, fd_time_t duration);

void fd_led_set_usb(uint8_t orange, uint8_t green);
void fd_led_set_d0(uint8_t value);
void fd_led_set_d1(uint8_t red, uint8_t green, uint8_t blue);
void fd_led_set_d2(uint8_t red, uint8_t green, uint8_t blue);
void fd_led_set_d3(uint8_t red, uint8_t green, uint8_t blue);
void fd_led_set_d4(uint8_t value);

#endif