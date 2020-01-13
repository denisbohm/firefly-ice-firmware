#include "fd_hal_rtc.h"

#include "fd_apollo.h"

int32_t fd_hal_rtc_utc_offset;
fd_time_t fd_hal_rtc_time_offset;
uint32_t fd_hal_rtc_countdown;

void fd_hal_rtc_set_utc_offset(int32_t utc_offset) {
    fd_hal_rtc_utc_offset = utc_offset;
}

int32_t fd_hal_rtc_get_utc_offset(void) {
    return fd_hal_rtc_utc_offset;
}

#define FD_HAL_RTC_CTIMER_NUMBER 0
#define FD_HAL_RTC_MS_CTIMER_NUMBER 1

void fd_hal_rtc_initialize(void) {
    fd_hal_rtc_utc_offset = 0;
    fd_hal_rtc_time_offset = (fd_time_t){ .seconds = 0, .microseconds = 0 };
    fd_hal_rtc_countdown = 0;

    am_hal_ctimer_config_t ctimer_config = {
        // link timers
        .ui32Link = 1,
        // Timer A
        .ui32TimerAConfig = AM_HAL_CTIMER_FN_CONTINUOUS | AM_HAL_CTIMER_LFRC_512HZ,
        // Timer B
        .ui32TimerBConfig = 0,
    };
    am_hal_ctimer_clear(FD_HAL_RTC_CTIMER_NUMBER, AM_HAL_CTIMER_BOTH);
    am_hal_ctimer_config(FD_HAL_RTC_CTIMER_NUMBER, &ctimer_config);
    am_hal_ctimer_start(FD_HAL_RTC_CTIMER_NUMBER, AM_HAL_CTIMER_BOTH);

#if 0
    // clock from nRF is 16Hz, so create a ms timer on this side...
    !!! start ms timer - every second rollover - for accurate timing -denis
    am_hal_ctimer_clear(FD_HAL_RTC_MS_CTIMER_NUMBER, AM_HAL_CTIMER_TIMERA);
#define AM_HAL_CTIMER_LFRC_1KHZ AM_REG_CTIMER_CTRL0_TMRA0CLK(0xB)
    am_hal_ctimer_config_single(
        FD_HAL_RTC_MS_CTIMER_NUMBER, AM_HAL_CTIMER_TIMERA,
        AM_HAL_CTIMER_FN_ONCE | AM_HAL_CTIMER_LFRC_1KHZ
    );
    am_hal_ctimer_period_set(FD_HAL_RTC_MS_CTIMER_NUMBER, AM_HAL_CTIMER_TIMERA, 32 - 1, 16);
    am_hal_ctimer_start(FD_HAL_RTC_MS_CTIMER_NUMBER, AM_HAL_CTIMER_TIMERA);
#endif
}

void fd_hal_rtc_enable_pin_input(const fd_rtc_t *rtc __attribute__((unused)), fd_gpio_t gpio) {
//    PAD20 TCTA2 INPEN - need to set this so can use clock pin input
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
#ifdef AM_PART_APOLLO2
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_FUNC(2) | AM_HAL_GPIO_INPEN);
#else
    am_hal_gpio_pincfg_t pincfg = {
        .uFuncSel = 2,
        .eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,
    };
    am_hal_gpio_pin_config(pin_number, pincfg);
#endif
    am_hal_ctimer_config_t ctimer_config = {
        // link timers
        .ui32Link = 1,
        // Timer A
        .ui32TimerAConfig = AM_HAL_CTIMER_FN_CONTINUOUS | AM_HAL_CTIMER_CLK_PIN,
        // Timer B
        .ui32TimerBConfig = 0,
    };
    am_hal_ctimer_config(FD_HAL_RTC_CTIMER_NUMBER, &ctimer_config);
    am_hal_ctimer_start(FD_HAL_RTC_CTIMER_NUMBER, AM_HAL_CTIMER_BOTH);
}

void fd_hal_rtc_disable_pin_input(const fd_rtc_t *rtc __attribute__((unused)), fd_gpio_t gpio) {
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
#ifdef AM_PART_APOLLO2
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_FUNC(0) | AM_HAL_GPIO_INPEN);
#else
    am_hal_gpio_pin_config(pin_number, g_AM_HAL_GPIO_INPUT);
#endif
    am_hal_ctimer_config_t ctimer_config = {
        // link timers
        .ui32Link = 1,
        // Timer A
        .ui32TimerAConfig = AM_HAL_CTIMER_FN_CONTINUOUS | AM_HAL_CTIMER_LFRC_512HZ,
        // Timer B
        .ui32TimerBConfig = 0,
    };
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