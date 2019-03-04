#include "fd_event.h"
#include "fd_hal_processor.h"
#include "fd_hal_reset.h"
#include "fd_hal_rtc.h"

#include "apollo2.h"
#include <am_mcu_apollo.h>

#include <stdint.h>

int32_t fd_hal_rtc_utc_offset;

static volatile uint32_t rtc_countdown;

void fd_hal_rtc_set_utc_offset(int32_t utc_offset) {
    fd_hal_rtc_utc_offset = utc_offset;
}

int32_t fd_hal_rtc_get_utc_offset(void) {
    return fd_hal_rtc_utc_offset;
}

void fd_hal_rtc_initialize(void) {
    fd_hal_rtc_utc_offset = 0;
    rtc_countdown = 0;

}

void fd_hal_rtc_sleep(void) {
}

void fd_hal_rtc_wake(void) {
}

void fd_hal_rtc_set_time(fd_time_t time) {
    fd_hal_reset_retained_t *retained = fd_hal_reset_retained();
    fd_hal_processor_interrupts_disable();
    retained->rtc.seconds = time.seconds;
    retained->rtc.microseconds = (time.microseconds / 31250) * 31250;
    fd_hal_processor_interrupts_enable();
}

uint32_t fd_hal_rtc_get_seconds(void) {
    fd_hal_reset_retained_t *retained = fd_hal_reset_retained();
    return retained->rtc.seconds;
}

fd_time_t fd_hal_rtc_get_time(void) {
    fd_time_t time;
    fd_hal_reset_retained_t *retained = fd_hal_reset_retained();
    fd_hal_processor_interrupts_disable();
    time.microseconds = retained->rtc.microseconds;
    time.seconds = retained->rtc.seconds;
    fd_hal_processor_interrupts_enable();
    return time;
}

fd_time_t fd_hal_rtc_get_accurate_time(void) {
    return fd_hal_rtc_get_time();
}

void fd_hal_rtc_set_countdown(uint32_t countdown) {
    fd_hal_processor_interrupts_disable();
    rtc_countdown = countdown;
    fd_hal_processor_interrupts_enable();
}

uint32_t fd_hal_rtc_get_countdown(void) {
    fd_hal_processor_interrupts_disable();
    uint32_t countdown = rtc_countdown;
    fd_hal_processor_interrupts_enable();
    return countdown;
}

#if 0

void RTC_IRQHandler(void) {
    RTC_IntClear(RTC_IFC_COMP0);

    fd_hal_reset_retained_t *retained = fd_hal_reset_retained();
    if (RTC->COMP0 == 1023) {
        uint32_t microseconds = retained->rtc.microseconds + 31250;
        if (microseconds >= 1000000) {
            microseconds -= 1000000;
            ++retained->rtc.seconds;
        }
        retained->rtc.microseconds = microseconds;

        fd_event_set(FD_EVENT_RTC_TICK);

        if (rtc_countdown) {
            if (--rtc_countdown == 0) {
                fd_event_set(FD_EVENT_RTC_COUNTDOWN);
            }
        }
    } else {
        retained->rtc.seconds += 2;
    }
}

#endif