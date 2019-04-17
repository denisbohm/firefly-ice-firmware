#ifndef FD_PWM_H
#define FD_PWM_H

#include "fd_gpio.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t instance;
    float frequency;
} fd_pwm_module_t;

typedef enum {
    fd_pwm_polarity_rising,
    fd_pwm_polarity_falling,
} fd_pwm_polarity_t;

typedef void (*fd_pwm_function_t)(bool on);

typedef struct {
    const fd_pwm_module_t *module;
    uint32_t instance;
    fd_pwm_function_t function;
    fd_gpio_t gpio;
    fd_pwm_polarity_t polarity;
} fd_pwm_channel_t;

void fd_pwm_initialize(const fd_pwm_module_t *modules, uint32_t module_count, const fd_pwm_channel_t *channels, uint32_t channel_count);

void fd_pwm_module_enable(const fd_pwm_module_t *module);
void fd_pwm_module_disable(const fd_pwm_module_t *module);

void fd_pwm_channel_start(const fd_pwm_channel_t *channel, float duty_cycle);
bool fd_pwm_channel_is_running(const fd_pwm_channel_t *channel);
void fd_pwm_channel_stop(const fd_pwm_channel_t *channel);

#endif
