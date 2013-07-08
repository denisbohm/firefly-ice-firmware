#ifndef FD_INDICATOR_H
#define FD_INDICATOR_H

#include "fd_time.h"

#include <stdint.h>

typedef struct {
    uint8_t r;
} fd_indicator_r_t;

typedef struct {
    uint8_t o;
    uint8_t g;
} fd_indicator_og_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} fd_indicator_rgb_t;

typedef struct {
    fd_indicator_og_t usb;
    fd_indicator_r_t d0;
    fd_indicator_rgb_t d1;
    fd_indicator_rgb_t d2;
    fd_indicator_rgb_t d3;
    fd_indicator_r_t d4;
} fd_indicator_state_t;

void fd_indicator_initialize(void);

void fd_indicator_wake(void);
void fd_indicator_sleep(void);

void fd_indicator_override(fd_indicator_state_t *state, fd_time_t duration);

#endif