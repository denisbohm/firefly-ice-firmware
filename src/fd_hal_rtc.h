#ifndef FD_HAL_RTC_H
#define FD_HAL_RTC_H

#include "fd_gpio.h"
#include "fd_rtc.h"
#include "fd_time.h"

#include <stdint.h>

fd_time_t fd_hal_rtc_get_time_retained(void);

void fd_hal_rtc_initialize(void);

void fd_hal_rtc_enable_pin_input(const fd_rtc_t *rtc, fd_gpio_t gpio);
void fd_hal_rtc_disable_pin_input(const fd_rtc_t *rtc, fd_gpio_t gpio);

void fd_hal_rtc_enable_pin_output(const fd_rtc_t *rtc, fd_gpio_t gpio);
void fd_hal_rtc_disable_pin_output(const fd_rtc_t *rtc, fd_gpio_t gpio);

void fd_hal_rtc_sleep(void);
void fd_hal_rtc_wake(void);

void fd_hal_rtc_set_utc_offset(int32_t utc_offset);
int32_t fd_hal_rtc_get_utc_offset(void);

void fd_hal_rtc_set_time(fd_time_t time);

uint32_t fd_hal_rtc_get_seconds(void);

fd_time_t fd_hal_rtc_get_time(void);

fd_time_t fd_hal_rtc_get_accurate_time(void);

void fd_hal_rtc_set_countdown(uint32_t countdown);
uint32_t fd_hal_rtc_get_countdown(void);

uint32_t fd_hal_rtc_get_tick(void);

#endif
