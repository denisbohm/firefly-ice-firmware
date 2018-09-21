#ifndef FD_RTC_H
#define FD_RTC_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*fd_rtc_handler_t)(void);

typedef struct {
    uint32_t instance;
    uint32_t ticks_per_second;
    fd_rtc_handler_t handler;
} fd_rtc_t;

void fd_rtc_initialize(
    const fd_rtc_t *rtcs, uint32_t rtc_count
);

void fd_rtc_enable(const fd_rtc_t *rtc);
void fd_rtc_disable(const fd_rtc_t *rtc);
bool fd_rtc_is_enabled(const fd_rtc_t *rtc);

uint32_t fd_rtc_get_subticks(const fd_rtc_t *fd_rtc);

#endif