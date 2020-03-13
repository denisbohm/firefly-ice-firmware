#include "fd_pwm.h"

#include "fd_nrf5.h"

#include <math.h>
#include <string.h>

typedef struct {
    float prescaler_divider;
    uint32_t prescaler;
    uint32_t countertop;
    uint16_t sequence[4];
} fd_pwm_module_state_t;

fd_pwm_module_state_t fd_pwm_module_states[4];

void fd_pwm_initialize(const fd_pwm_module_t *modules, uint32_t module_count, const fd_pwm_channel_t *channels __attribute__((unused)), uint32_t channel_count __attribute__((unused))) {
    memset(fd_pwm_module_states, 0, sizeof(fd_pwm_module_states));
    for (uint32_t i = 0; i < module_count; ++i) {
        const fd_pwm_module_t *module = &modules[i];
        fd_pwm_module_state_t *state = &fd_pwm_module_states[i];

        int prescaler_divider_int = (int)ceil(((16000000.0f / (32768.0f)) / module->frequency));
        switch (prescaler_divider_int) {
            case 1:
                state->prescaler_divider = 1.0;
                state->prescaler = PWM_PRESCALER_PRESCALER_DIV_1;
                break;
            case 2:
                state->prescaler_divider = 2.0;
                state->prescaler = PWM_PRESCALER_PRESCALER_DIV_2;
                break;
            case 4:
                state->prescaler_divider = 4.0;
                state->prescaler = PWM_PRESCALER_PRESCALER_DIV_4;
                break;
            case 8:
                state->prescaler_divider = 8.0;
                state->prescaler = PWM_PRESCALER_PRESCALER_DIV_8;
                break;
            case 16:
                state->prescaler_divider = 16.0;
                state->prescaler = PWM_PRESCALER_PRESCALER_DIV_16;
                break;
            case 32:
                state->prescaler_divider = 32.0;
                state->prescaler = PWM_PRESCALER_PRESCALER_DIV_32;
                break;
            case 64:
                state->prescaler_divider = 64.0;
                state->prescaler = PWM_PRESCALER_PRESCALER_DIV_64;
                break;
            default:
                state->prescaler_divider = 128.0;
                state->prescaler = PWM_PRESCALER_PRESCALER_DIV_128;
                break;
        }
        // countertop is 15-bits, so with prescaler divider of 128 the minimum frequency is 16000000/(128*32767) is ~3.82 Hz
        state->countertop = (uint32_t)(16000000.0f / (state->prescaler_divider * module->frequency));
    }
}

static
fd_pwm_module_state_t *fd_pwm_get_state(uint32_t instance) {
    if (instance == (uint32_t)NRF_PWM0) {
        return &fd_pwm_module_states[0];
    }
    if (instance == (uint32_t)NRF_PWM1) {
        return &fd_pwm_module_states[1];
    }
    if (instance == (uint32_t)NRF_PWM2) {
        return &fd_pwm_module_states[2];
    }
#ifdef NRF52840_XXAA
    if (instance == (uint32_t)NRF_PWM3) {
        return &fd_pwm_module_states[3];
    }
#endif
    return 0;
}

void fd_pwm_module_enable(const fd_pwm_module_t *module) {
    NRF_PWM_Type *pwm = (NRF_PWM_Type *)module->instance;
    fd_pwm_module_state_t *state = fd_pwm_get_state(module->instance);
    pwm->ENABLE = PWM_ENABLE_ENABLE_Enabled << PWM_ENABLE_ENABLE_Pos;
    pwm->MODE = PWM_MODE_UPDOWN_Up << PWM_MODE_UPDOWN_Pos;
    pwm->PRESCALER = state->prescaler << PWM_PRESCALER_PRESCALER_Pos;
    pwm->COUNTERTOP = state->countertop << PWM_COUNTERTOP_COUNTERTOP_Pos;
    pwm->LOOP = PWM_LOOP_CNT_Disabled << PWM_LOOP_CNT_Pos;
    pwm->DECODER = (PWM_DECODER_LOAD_Individual << PWM_DECODER_LOAD_Pos) | (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);
    pwm->SEQ[0].PTR = ((uint32_t)(&state->sequence[0])) << PWM_SEQ_PTR_PTR_Pos;
    pwm->SEQ[0].CNT = ((sizeof(state->sequence) / sizeof(uint16_t)) << PWM_SEQ_CNT_CNT_Pos);
    pwm->SEQ[0].REFRESH = 0;
    pwm->SEQ[0].ENDDELAY = 0;
    pwm->TASKS_SEQSTART[0] = 1;
}

void fd_pwm_module_disable(const fd_pwm_module_t *module) {
    NRF_PWM_Type *pwm = (NRF_PWM_Type *)module->instance;
    pwm->EVENTS_STOPPED = 0;
    pwm->TASKS_STOP = 1;
    while (pwm->EVENTS_STOPPED == 0);
    pwm->ENABLE = PWM_ENABLE_ENABLE_Disabled << PWM_ENABLE_ENABLE_Pos;

    pwm->EVENTS_STOPPED = 0;
    pwm->EVENTS_SEQSTARTED[0] = 0;
    pwm->EVENTS_SEQEND[0] = 0;
    pwm->EVENTS_PWMPERIODEND = 0;
    pwm->COUNTERTOP = 0;
    pwm->LOOP = 0;
    pwm->DECODER = 0;
    pwm->SEQ[0].PTR = 0;
    pwm->SEQ[0].CNT = 0;
    pwm->SEQ[0].REFRESH = 0;
    pwm->SEQ[0].ENDDELAY = 0;
}

void fd_pwm_channel_start(const fd_pwm_channel_t *channel, float duty_cycle) {
    const fd_pwm_module_t *module = channel->module;
    NRF_PWM_Type *pwm = (NRF_PWM_Type *)module->instance;
    fd_pwm_module_state_t *state = fd_pwm_get_state(module->instance);

    uint16_t sequence_duty_cycle = (uint16_t)(duty_cycle * state->countertop);
    if (channel->polarity == fd_pwm_polarity_falling) {
        sequence_duty_cycle |= 0x8000;
    }
    state->sequence[channel->instance] = sequence_duty_cycle;
    pwm->TASKS_SEQSTART[0] = 1;

    uint32_t pin = NRF_GPIO_PIN_MAP(channel->gpio.port, channel->gpio.pin);
    pwm->PSEL.OUT[channel->instance] = (pin << PWM_PSEL_OUT_PIN_Pos) | (PWM_PSEL_OUT_CONNECT_Connected << PWM_PSEL_OUT_CONNECT_Pos);
}

bool fd_pwm_channel_is_running(const fd_pwm_channel_t *channel) {
    const fd_pwm_module_t *module = channel->module;
    NRF_PWM_Type *pwm = (NRF_PWM_Type *)module->instance;

    return pwm->PSEL.OUT[channel->instance] != 0xFFFFFFFF;
}

void fd_pwm_channel_stop(const fd_pwm_channel_t *channel) {
    const fd_pwm_module_t *module = channel->module;
    NRF_PWM_Type *pwm = (NRF_PWM_Type *)module->instance;
    fd_pwm_module_state_t *state = fd_pwm_get_state(module->instance);

    state->sequence[channel->instance] = 0;
    pwm->PSEL.OUT[channel->instance] = 0xFFFFFFFF;
}
