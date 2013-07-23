#ifndef FD_TIMER_H
#define FD_TIMER_H

#include "fd_time.h"

#include <stdbool.h>

typedef void (*fd_timer_callback_t)(void);

typedef struct {
    fd_timer_callback_t callback;
    bool active;
    bool scheduled;
    bool triggered;
    uint32_t countdown;
} fd_timer_t;

void fd_timer_initialize(void);

void fd_timer_time_change(fd_time_t old_time, fd_time_t new_time);

void fd_timer_add(fd_timer_t *timer, fd_timer_callback_t callback);

void fd_timer_start(fd_timer_t *timer, fd_time_t interval);

void fd_timer_stop(fd_timer_t *timer);

#endif