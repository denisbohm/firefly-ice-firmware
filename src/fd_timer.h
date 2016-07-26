#ifndef FD_TIMER_H
#define FD_TIMER_H

#include "fd_time.h"
#include "fd_timing.h"

#include <stdbool.h>

typedef void (*fd_timer_callback_t)(void);

typedef struct {
    fd_timer_callback_t callback;
    bool active;
    bool scheduled;
    bool triggered;
    uint32_t countdown;
#ifdef FD_EVENT_TIMING
    fd_timing_t timing;
#endif
} fd_timer_t;

void fd_timer_initialize(void);
void fd_timer_add_with_identifier(fd_timer_t *timer, fd_timer_callback_t callback, const char *identifier);
#define fd_timer_add(timer, callback) fd_timer_add_with_identifier(timer, callback, #callback)

void fd_timer_start(fd_timer_t *timer, fd_time_t duration);
void fd_timer_start_next(fd_timer_t *timer, uint32_t interval);
void fd_timer_stop(fd_timer_t *timer);

fd_timing_iterator_t fd_timer_timing_iterator(void);

#endif