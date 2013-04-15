#ifndef FD_RTC_H
#define FD_RTC_H

#include "fd_time.h"

#include <stdint.h>

void fd_rtc_initialize(void);

uint32_t rtc_get_seconds(void);

fd_time_t rtc_get_time(void);

fd_time_t rtc_get_accurate_time(void);

#endif