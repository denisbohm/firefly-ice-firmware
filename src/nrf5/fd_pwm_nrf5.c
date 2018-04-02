#include "fd_pwm.h"

#include "fd_nrf5.h"

typedef struct {
    uint16_t sequence[4];
} fd_pwm_module_state_t;

fd_pwm_module_state_t fd_pwm_module_states[4];

void fd_pwm_initialize(const fd_pwm_module_t *modules, uint32_t module_count) {
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
    if (instance == (uint32_t)NRF_PWM3) {
        return &fd_pwm_module_states[3];
    }
    return 0;
}

uint32_t fd_pwm_get_countertop(const fd_pwm_module_t *module) {
    // countertop is 15-bits, so with prescaler of 1 the minimum frequency possible is 16000000/32767 is ~489Hz
    return (uint32_t)(16000000.0f / module->frequency);
}

void fd_pwm_module_enable(const fd_pwm_module_t *module) {
    NRF_PWM_Type *pwm = (NRF_PWM_Type *)module->instance;
    fd_pwm_module_state_t *state = fd_pwm_get_state(module->instance);
    pwm->ENABLE = PWM_ENABLE_ENABLE_Enabled << PWM_ENABLE_ENABLE_Pos;
    pwm->MODE = PWM_MODE_UPDOWN_Up << PWM_MODE_UPDOWN_Pos;
    pwm->PRESCALER = PWM_PRESCALER_PRESCALER_DIV_1 << PWM_PRESCALER_PRESCALER_Pos;
    uint32_t countertop = fd_pwm_get_countertop(module);
    pwm->COUNTERTOP = countertop << PWM_COUNTERTOP_COUNTERTOP_Pos; //1 msec
    pwm->LOOP = PWM_LOOP_CNT_Disabled << PWM_LOOP_CNT_Pos;
    pwm->DECODER = (PWM_DECODER_LOAD_Individual << PWM_DECODER_LOAD_Pos) | (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);
    pwm->SEQ[0].PTR = ((uint32_t)(state->sequence) << PWM_SEQ_PTR_PTR_Pos);
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
}

void fd_pwm_start(const fd_pwm_channel_t *channel, float duty_cycle) {
    const fd_pwm_module_t *module = channel->module;
    NRF_PWM_Type *pwm = (NRF_PWM_Type *)module->instance;
    fd_pwm_module_state_t *state = fd_pwm_get_state(module->instance);

    // countertop is 15-bits, so with prescaler of 1 the minimum frequency possible is 16000000/32767 is ~489Hz
    uint32_t countertop = (uint32_t)(16000000.0f / module->frequency);

    uint16_t sequence_duty_cycle = (uint16_t)(duty_cycle * countertop);
    state->sequence[channel->instance] = sequence_duty_cycle;
    pwm->PSEL.OUT[channel->instance] = NRF_GPIO_PIN_MAP(channel->gpio.port, channel->gpio.pin);
}

bool fd_pwm_is_running(const fd_pwm_channel_t *channel) {
    const fd_pwm_module_t *module = channel->module;
    NRF_PWM_Type *pwm = (NRF_PWM_Type *)module->instance;
    for (uint32_t i = 0; i < 4; ++i) {
        if (pwm->PSEL.OUT[i] != 0xFFFFFFFF) {
            return true;
        }
    }
    return false;
}

void fd_pwm_stop(const fd_pwm_channel_t *channel) {
    const fd_pwm_module_t *module = channel->module;
    NRF_PWM_Type *pwm = (NRF_PWM_Type *)module->instance;
    pwm->PSEL.OUT[channel->instance] = 0xFFFFFFFF;
}
