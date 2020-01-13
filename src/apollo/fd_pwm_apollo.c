#include "fd_pwm.h"

#include "fd_apollo.h"

const fd_pwm_module_t *fd_pwm_modules;
uint32_t fd_pwm_module_count;
const fd_pwm_channel_t *fd_pwm_channels;
uint32_t fd_pwm_channel_count;

void fd_pwm_initialize(const fd_pwm_module_t *modules __attribute__((unused)), uint32_t module_count __attribute__((unused)), const fd_pwm_channel_t *channels, uint32_t channel_count) {
    fd_pwm_modules = modules;
    fd_pwm_module_count = module_count;
    fd_pwm_channels = channels;
    fd_pwm_channel_count = channel_count;

    for (uint32_t i = 0; i < fd_pwm_channel_count; ++i) {
        const fd_pwm_channel_t *channel = &fd_pwm_channels[i];
        if (channel->function != 0) {
            const fd_pwm_module_t *module = channel->module;
            uint32_t interrupt = 1u << (module->instance * 2 + channel->instance);
            am_hal_ctimer_int_clear(interrupt);
            am_hal_ctimer_int_enable(interrupt);
            am_hal_ctimer_int_clear(interrupt << 8);
            am_hal_ctimer_int_enable(interrupt << 8);
        }
    }
    NVIC_SetPriority(CTIMER_IRQn, 0);
    am_hal_interrupt_enable(AM_HAL_INTERRUPT_CTIMER);
    am_hal_interrupt_master_enable();
}

void fd_pwm_module_enable(const fd_pwm_module_t *module __attribute__((unused))) {
}

void fd_pwm_module_disable(const fd_pwm_module_t *module __attribute__((unused))) {
}

void am_ctimer_isr(void) {
    uint32_t status = am_hal_ctimer_int_status_get(false);
    am_hal_ctimer_int_clear(status);
    am_hal_ctimer_int_service(status);

    for (uint32_t i = 0; i < fd_pwm_channel_count; ++i) {
        const fd_pwm_channel_t *channel = &fd_pwm_channels[i];
        if (channel->function != 0) {
            const fd_pwm_module_t *module = channel->module;
            uint32_t interrupt = 1u << (module->instance * 2 + channel->instance);
            if (status & interrupt) {
                channel->function(false);
            }
            if (status & (interrupt << 8)) {
                channel->function(true);
            }
        }
    }
}

#define AM_HAL_CTIMER_HFRC_12MHZ AM_REG_CTIMER_CTRL0_TMRA0CLK(0x1)
#define AM_HAL_CTIMER_LFRC_1KHZ AM_REG_CTIMER_CTRL0_TMRA0CLK(0xD)

void fd_pwm_channel_start(const fd_pwm_channel_t *channel, float duty_cycle) {
    uint32_t timer_number = channel->module->instance;
    uint32_t timer_segment = channel->instance == 0 ? AM_HAL_CTIMER_TIMERA : AM_HAL_CTIMER_TIMERB;
    uint32_t top_count;
    uint32_t control = AM_HAL_CTIMER_FN_PWM_REPEAT;
    if (channel->function == 0) {
        control |= AM_HAL_CTIMER_PIN_ENABLE;
    } else {
        control |= AM_REG_CTIMER_CTRL0_TMRA0IE0_M | AM_REG_CTIMER_CTRL0_TMRA0IE1_M;
    }
    if (channel->module->frequency >= 100) {
        top_count = ((uint32_t)(12000000.0f / channel->module->frequency)) - 1;
        control |= AM_HAL_CTIMER_HFRC_12MHZ;
    } else {
        top_count = ((uint32_t)(1024.0f / channel->module->frequency)) - 1;
        control |= AM_HAL_CTIMER_LFRC_1KHZ;
    }
    am_hal_ctimer_config_single(timer_number, timer_segment, control);
    uint32_t on_count = (uint32_t)(duty_cycle * (float)top_count);
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
