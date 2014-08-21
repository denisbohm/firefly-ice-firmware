#include "fd_event.h"
#include "fd_hal_processor.h"
#include "fd_hal_rtc.h"
#include "fd_log.h"
#include "fd_timer.h"

#define TIMERS_LIMIT 16

static fd_timer_t *timers[TIMERS_LIMIT];
static uint32_t timer_count;
static uint32_t scheduled_countdown;

void fd_timer_update(void);

void fd_timer_initialize(void) {
    timer_count = 0;
    scheduled_countdown = 0;

    fd_event_add_callback(FD_EVENT_RTC_COUNTDOWN, fd_timer_update);
    fd_event_add_callback(FD_EVENT_TIMER_SCHEDULE, fd_timer_update);
}

void fd_timer_add(fd_timer_t *timer, fd_timer_callback_t callback) {
    timer->callback = callback;
    timer->active = false;
    timer->scheduled = false;
    timer->triggered = false;
    timer->countdown = 0;

    if (timer_count >= TIMERS_LIMIT) {
        fd_log_assert_fail("timer limit");
        return;
    }

    timers[timer_count++] = timer;
}

static
void fd_timer_schedule_countdown(void) {
    uint32_t new_countdown = UINT32_MAX;
    uint32_t remaining_countdown = fd_hal_rtc_get_countdown();
    uint32_t elapsed_countdown = scheduled_countdown - remaining_countdown;
    for (uint32_t i = 0; i < timer_count; ++i) {
        fd_timer_t *timer = timers[i];
        if (!timer->active) {
            continue;
        }
        if (timer->scheduled) {
            if (timer->countdown <= elapsed_countdown) {
                timer->active = false;
                timer->triggered = true;
                timer->countdown = 0;
                continue;
            }
            timer->countdown -= elapsed_countdown;
        } else {
            timer->scheduled = true;
        }
        if (timer->countdown < new_countdown) {
            new_countdown = timer->countdown;
        }
    }
    if (new_countdown == 0) {
        new_countdown = 1;
    }
    if (new_countdown == UINT32_MAX) {
        // nothing to do, but schedule a countdown in 5 seconds as a fail-safe
        new_countdown = 5 * 32;
    }
    scheduled_countdown = new_countdown;
    fd_hal_rtc_set_countdown(new_countdown);
}

static
void fd_timer_callback_triggered(void) {
    for (uint32_t i = 0; i < timer_count; ++i) {
        fd_timer_t *timer = timers[i];
        if (timer->triggered) {
            timer->triggered = false;
            (*timer->callback)();
        }
    }
}

void fd_timer_update(void) {
    fd_timer_schedule_countdown();
    fd_timer_callback_triggered();
}

void fd_timer_start(fd_timer_t *timer, fd_time_t duration) {
    uint32_t countdown = duration.seconds * 32 + (duration.microseconds + 31250 - 1) / 31250;
    timer->countdown = countdown;
    timer->active = true;
    timer->scheduled = false;
    timer->triggered = false;

    fd_event_set_exclusive(FD_EVENT_TIMER_SCHEDULE);
}

void fd_timer_start_next(fd_timer_t *timer, uint32_t interval) {
    fd_time_t now = fd_hal_rtc_get_time();
    fd_time_t at;
    at.microseconds = 0;
    at.seconds = (now.seconds / interval) * interval + interval;
    fd_time_t duration = fd_time_subtract(at, now);
    fd_timer_start(timer, duration);
}

void fd_timer_stop(fd_timer_t *timer) {
    timer->active = false;
}