#include "fd_hal_rtc.h"

#include "apollo2.h"
#include <am_mcu_apollo.h>

int32_t fd_hal_rtc_utc_offset;
fd_time_t fd_hal_rtc_time_offset;
uint32_t fd_hal_rtc_countdown;

void fd_hal_rtc_set_utc_offset(int32_t utc_offset) {
    fd_hal_rtc_utc_offset = utc_offset;
}

int32_t fd_hal_rtc_get_utc_offset(void) {
    return fd_hal_rtc_utc_offset;
}

#define FD_HAL_RTC_CTIMER_NUMBER 2

void fd_hal_rtc_initialize(void) {
    fd_hal_rtc_utc_offset = 0;
    fd_hal_rtc_time_offset = (fd_time_t){ .seconds = 0, .microseconds = 0 };
    fd_hal_rtc_countdown = 0;


//    PAD20INPEN - need to set this so can use clock pin input

    am_hal_ctimer_config_t ctimer_config = {
        // link timers
        .ui32Link = 1,
        // Timer A
        .ui32TimerAConfig = AM_HAL_CTIMER_FN_CONTINUOUS | AM_HAL_CTIMER_LFRC_512HZ,
//        AM_HAL_CTIMER_FN_CONTINUOUS | AM_HAL_CTIMER_CLK_PIN,
        // Timer B
        .ui32TimerBConfig = 0,
    };
    am_hal_ctimer_clear(FD_HAL_RTC_CTIMER_NUMBER, AM_HAL_CTIMER_BOTH);
    am_hal_ctimer_config(FD_HAL_RTC_CTIMER_NUMBER, &ctimer_config);
    am_hal_ctimer_start(FD_HAL_RTC_CTIMER_NUMBER, AM_HAL_CTIMER_BOTH);
}

void fd_hal_rtc_sleep(void) {
}

void fd_hal_rtc_wake(void) {
}

uint32_t fd_hal_rtc_get_tick(void) {
    uint32_t ticks = am_hal_ctimer_read(FD_HAL_RTC_CTIMER_NUMBER, AM_HAL_CTIMER_BOTH);
    return ticks;
}

void fd_hal_rtc_set_time(fd_time_t time) {
    uint32_t ticks = am_hal_ctimer_read(FD_HAL_RTC_CTIMER_NUMBER, AM_HAL_CTIMER_BOTH);
    fd_time_t delta = fd_time_from_us(ticks * 1953ULL);
    fd_hal_rtc_time_offset = fd_time_subtract(time, delta);
}

uint32_t fd_hal_rtc_get_seconds(void) {
    return fd_hal_rtc_get_time().seconds;
}

fd_time_t fd_hal_rtc_get_time(void) {
    uint32_t ticks = am_hal_ctimer_read(FD_HAL_RTC_CTIMER_NUMBER, AM_HAL_CTIMER_BOTH);
    fd_time_t delta = fd_time_from_us(ticks * 1953ULL);
    return fd_time_add(fd_hal_rtc_time_offset, delta);
}

fd_time_t fd_hal_rtc_get_accurate_time(void) {
    return fd_hal_rtc_get_time();
}

void fd_hal_rtc_set_countdown(uint32_t countdown) {
    fd_hal_rtc_countdown = countdown;
}

uint32_t fd_hal_rtc_get_countdown(void) {
    return fd_hal_rtc_countdown;
}