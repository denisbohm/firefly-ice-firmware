#ifndef FD_RTC_H
#define FD_RTC_H

#include "fd_gpio.h"

#include <stdbool.h>
#include <stdint.h>

typedef void (*fd_rtc_handler_t)(void);

typedef struct {
    uint32_t instance;
    uint32_t ticks_per_second;
    uint32_t ticks_per_correction;
    int32_t correction_count;
    fd_rtc_handler_t handler;
} fd_rtc_t;

void fd_rtc_initialize(
    const fd_rtc_t *rtcs, uint32_t rtc_count
);

void fd_rtc_enable(const fd_rtc_t *rtc);
void fd_rtc_disable(const fd_rtc_t *rtc);
bool fd_rtc_is_enabled(const fd_rtc_t *rtc);

uint32_t fd_rtc_get_subticks(const fd_rtc_t *fd_rtc);

void fd_rtc_enable_pin(const fd_rtc_t *fd_rtc, fd_gpio_t gpio);
void fd_rtc_disable_pin(const fd_rtc_t *fd_rtc, fd_gpio_t gpio);

#endif