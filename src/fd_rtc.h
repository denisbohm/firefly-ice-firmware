#ifndef FD_RTC_H
#define FD_RTC_H

#include "fd_time.h"

#include <stdint.h>

fd_time_t fd_rtc_get_time_retained(void);

void fd_rtc_initialize(void);

void fd_rtc_sleep(void);
void fd_rtc_wake(void);

void fd_rtc_set_time(fd_time_t time);

uint32_t fd_rtc_get_seconds(void);

fd_time_t fd_rtc_get_time(void);

fd_time_t fd_rtc_get_accurate_time(void);

typedef void (*fd_rtc_callback_t)(void);

void fd_rtc_set_countdown(uint32_t countdown);
uint32_t fd_rtc_get_countdown(void);

#endif