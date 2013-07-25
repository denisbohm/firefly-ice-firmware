#include "fd_event.h"
#include "fd_log.h"
#include "fd_processor.h"
#include "fd_rtc.h"
#include "fd_timer.h"

#define TIMERS_LIMIT 8

static fd_timer_t *timers[TIMERS_LIMIT];
static uint32_t timer_count;
static uint32_t scheduled_countdown;

static
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
        fd_log_ram_assert_fail("timer limit");
        return;
    }

    timers[timer_count++] = timer;
}

static
void fd_timer_schedule_countdown(void) {
    uint32_t new_countdown = UINT32_MAX;
    fd_interrupts_disable();
    uint32_t remaining_countdown = fd_rtc_get_countdown();
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
    if (new_countdown == UINT32_MAX) {
        new_countdown = 0;
    }
    scheduled_countdown = new_countdown;
    fd_rtc_set_countdown(new_countdown);
    fd_interrupts_enable();
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

static
void fd_timer_update(void) {
    fd_timer_schedule_countdown();
    fd_timer_callback_triggered();
}

void fd_timer_start(fd_timer_t *timer, fd_time_t interval) {
    uint32_t countdown = interval.seconds * 32 + (interval.microseconds + 31250 - 1) / 31250;
    timer->countdown = countdown;
    timer->active = true;
    timer->scheduled = false;
    timer->triggered = false;

    fd_event_set(FD_EVENT_TIMER_SCHEDULE);
}

void fd_timer_stop(fd_timer_t *timer) {
    timer->active = false;
}