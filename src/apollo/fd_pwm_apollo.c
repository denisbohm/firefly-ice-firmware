#include "fd_pwm.h"

#include "apollo2.h"
#include <am_mcu_apollo.h>

void fd_pwm_initialize(const fd_pwm_module_t *modules, uint32_t module_count) {
}

void fd_pwm_module_enable(const fd_pwm_module_t *module) {
}

void fd_pwm_module_disable(const fd_pwm_module_t *module) {
}

void fd_pwm_channel_start(const fd_pwm_channel_t *channel, float duty_cycle) {
    uint32_t on_count = (uint32_t)(duty_cycle * 256.0f);
    am_hal_ctimer_period_set(0, AM_HAL_CTIMER_TIMERA, 256, on_count);
    am_hal_ctimer_config_single(0, AM_HAL_CTIMER_TIMERA,
                                (AM_HAL_CTIMER_FN_PWM_REPEAT |
                                 AM_HAL_CTIMER_XT_2_048KHZ |
                                 AM_HAL_CTIMER_PIN_ENABLE));
    am_hal_ctimer_start(0, AM_HAL_CTIMER_TIMERA);
}

#define TIMER_OFFSET (AM_REG_CTIMER_TMR1_O - AM_REG_CTIMER_TMR0_O)

bool fd_pwm_channel_is_running(const fd_pwm_channel_t *channel) {
    uint32_t timer_number = 0;
    uint32_t timer_segment = 0;
    volatile uint32_t *pui32ConfigReg = (uint32_t *)(AM_REG_CTIMERn(0) + AM_REG_CTIMER_CTRL0_O + (timer_number * TIMER_OFFSET));
    uint32_t value = AM_REGVAL(pui32ConfigReg);
    return (value & (timer_segment & (AM_REG_CTIMER_CTRL0_TMRA0EN_M | AM_REG_CTIMER_CTRL0_TMRB0EN_M))) != 0;
}

void fd_pwm_channel_stop(const fd_pwm_channel_t *channel) {
    am_hal_ctimer_clear(0, AM_HAL_CTIMER_TIMERA);
}
