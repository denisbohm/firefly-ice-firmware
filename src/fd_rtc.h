#ifndef FD_RTC_H
#define FD_RTC_H

#include "fd_time.h"

#include <stdint.h>

void fd_rtc_initialize(void);

void fd_rtc_sleep(void);
void fd_rtc_wake(void);

uint32_t fd_rtc_get_seconds(void);

fd_time_t fd_rtc_get_time(void);

fd_time_t fd_rtc_get_accurate_time(void);

#endif