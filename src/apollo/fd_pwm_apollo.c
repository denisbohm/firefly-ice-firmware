#include "fd_pwm.h"

#include "apollo2.h"
#include <am_mcu_apollo.h>

void fd_pwm_initialize(const fd_pwm_module_t *modules __attribute__((unused)), uint32_t module_count __attribute__((unused))) {
}

void fd_pwm_module_enable(const fd_pwm_module_t *module __attribute__((unused))) {
}

void fd_pwm_module_disable(const fd_pwm_module_t *module __attribute__((unused))) {
}

#define AM_HAL_CTIMER_LFRC_1KHZ AM_REG_CTIMER_CTRL0_TMRA0CLK(0xD)

void fd_pwm_channel_start(const fd_pwm_channel_t *channel, float duty_cycle) {
    uint32_t timer_number = channel->module->instance;
    uint32_t timer_segment = channel->instance == 0 ? AM_HAL_CTIMER_TIMERA : AM_HAL_CTIMER_TIMERB;
    uint32_t top_count = ((uint32_t)(1024.0f / channel->module->frequency)) - 1;
    uint32_t on_count = (uint32_t)(duty_cycle * (float)top_count);
    am_hal_ctimer_config_single(
        timer_number, timer_segment,
        AM_HAL_CTIMER_FN_PWM_REPEAT | AM_HAL_CTIMER_LFRC_1KHZ | AM_HAL_CTIMER_PIN_ENABLE
    );
    am_hal_ctimer_period_set(timer_number, timer_segment, top_count, on_count);
    am_hal_ctimer_start(timer_number, timer_segment);
}

#define TIMER_OFFSET (AM_REG_CTIMER_TMR1_O - AM_REG_CTIMER_TMR0_O)

bool fd_pwm_channel_is_running(const fd_pwm_channel_t *channel) {
    uint32_t timer_number = channel->module->instance;
    volatile uint32_t *pui32ConfigReg = (uint32_t *)(AM_REG_CTIMERn(0) + AM_REG_CTIMER_CTRL0_O + (timer_number * TIMER_OFFSET));
    uint32_t value = AM_REGVAL(pui32ConfigReg);
    uint32_t mask = channel->instance == 0 ? AM_REG_CTIMER_CTRL0_TMRA0EN_M : AM_REG_CTIMER_CTRL0_TMRB0EN_M;
    return (value & mask) != 0;
}

void fd_pwm_channel_stop(const fd_pwm_channel_t *channel) {
    uint32_t timer_number = channel->module->instance;
    uint32_t timer_segment = channel->instance == 0 ? AM_HAL_CTIMER_TIMERA : AM_HAL_CTIMER_TIMERB;
    am_hal_ctimer_clear(timer_number, timer_segment);
}
