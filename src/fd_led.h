#ifndef FD_LED_H
#define FD_LED_H

#include "fd_time.h"

#include <stdint.h>

typedef struct {
    uint8_t r;
} fd_led_r_t;

typedef struct {
    uint8_t o;
    uint8_t g;
} fd_led_og_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} fd_led_rgb_t;

typedef struct {
    fd_led_og_t usb;
    fd_led_r_t d0;
    fd_led_rgb_t d1;
    fd_led_rgb_t d2;
    fd_led_rgb_t d3;
    fd_led_r_t d4;
} fd_led_state_t;

void fd_led_initialize(void);

void fd_led_wake(void);
void fd_led_sleep(void);

void fd_led_override(fd_led_state_t *state, fd_time_t duration);

void fd_led_set_usb(uint8_t orange, uint8_t green);
void fd_led_set_d0(uint8_t value);
void fd_led_set_d1(uint8_t red, uint8_t green, uint8_t blue);
void fd_led_set_d2(uint8_t red, uint8_t green, uint8_t blue);
void fd_led_set_d3(uint8_t red, uint8_t green, uint8_t blue);
void fd_led_set_d4(uint8_t value);

#endif