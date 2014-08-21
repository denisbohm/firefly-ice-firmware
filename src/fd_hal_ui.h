#ifndef FD_HAL_UI_H
#define FD_HAL_UI_H

#include "fd_lock.h"
#include "fd_time.h"

#include <stdbool.h>
#include <stdint.h>

void fd_hal_ui_initialize(void);

void fd_hal_ui_sleep(void);
void fd_hal_ui_wake(void);

bool fd_hal_ui_get_indicate(fd_lock_owner_t owner);
void fd_hal_ui_set_indicate(fd_lock_owner_t owner, bool indicate);

void fd_hal_ui_set_identify(fd_time_t duration);
void fd_hal_ui_clear_identify(void);

typedef struct {
    uint8_t r;
} fd_hal_ui_led_r_t;

typedef struct {
    uint8_t o;
    uint8_t g;
} fd_hal_ui_led_og_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} fd_hal_ui_led_rgb_t;

typedef struct {
    fd_hal_ui_led_og_t usb;
    fd_hal_ui_led_r_t d0;
    fd_hal_ui_led_rgb_t d1;
    fd_hal_ui_led_rgb_t d2;
    fd_hal_ui_led_rgb_t d3;
    fd_hal_ui_led_r_t d4;
} fd_hal_ui_led_state_t;

void fd_hal_ui_set_led(fd_hal_ui_led_state_t *state, fd_time_t duration);

#endif