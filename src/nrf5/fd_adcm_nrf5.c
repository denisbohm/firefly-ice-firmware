#include "fd_adcm.h"

#include "fd_nrf5.h"

static uint32_t fd_adcm_nrf5_pselp_for_gpio(fd_gpio_t gpio) {
    if (gpio.port == 0) {
        switch (gpio.pin) {
            case 2: return 1;
            case 3: return 2;
            case 4: return 3;
            case 5: return 4;
            case 28: return 5;
            case 29: return 6;
            case 30: return 7;
            case 31: return 8;
            default: break;
        }
    }
    return 0;
}

static uint32_t fd_adcm_nrf5_gain_for_voltage(float max_voltage) {
    const float reference = 0.6;
    if (max_voltage <= reference / 4) {
        return SAADC_CH_CONFIG_GAIN_Gain4;
    }
    if (max_voltage <= reference / 2) {
        return SAADC_CH_CONFIG_GAIN_Gain2;
    }
    if (max_voltage <= reference / 1) {
        return SAADC_CH_CONFIG_GAIN_Gain1;
    }
    if (max_voltage <= reference * 2) {
        return SAADC_CH_CONFIG_GAIN_Gain1_2;
    }
    if (max_voltage <= reference * 3) {
        return SAADC_CH_CONFIG_GAIN_Gain1_3;
    }
    if (max_voltage <= reference * 4) {
        return SAADC_CH_CONFIG_GAIN_Gain1_4;
    }
    if (max_voltage <= reference * 5) {
        return SAADC_CH_CONFIG_GAIN_Gain1_5;
    }
    return SAADC_CH_CONFIG_GAIN_Gain1_6;
}

static float fd_adcm_nrf5_gain_factor(uint32_t gain) {
    switch (gain) {
        case SAADC_CH_CONFIG_GAIN_Gain1_6: return 1.0f / 6.0f;
        case SAADC_CH_CONFIG_GAIN_Gain1_5: return 1.0f / 5.0f;
        case SAADC_CH_CONFIG_GAIN_Gain1_4: return 1.0f / 4.0f;
        case SAADC_CH_CONFIG_GAIN_Gain1_3: return 1.0f / 3.0f;
        case SAADC_CH_CONFIG_GAIN_Gain1_2: return 1.0f / 2.0f;
        case SAADC_CH_CONFIG_GAIN_Gain1: return 1.0f;
        case SAADC_CH_CONFIG_GAIN_Gain2: return 2.0f;
        case SAADC_CH_CONFIG_GAIN_Gain4: return 4.0f;
        default: break;
    }
    return 1.0;
}

float fd_adcm_convert(fd_gpio_t gpio, float max_voltage) {
    int16_t adc_result = 0;
    NRF_SAADC->RESULT.PTR = (uint32_t)&adc_result;
    NRF_SAADC->RESULT.MAXCNT = 1;
    NRF_SAADC->INTENSET = (SAADC_INTENSET_END_Disabled << SAADC_INTENSET_END_Pos);
    NRF_SAADC->RESOLUTION = SAADC_RESOLUTION_VAL_14bit;
    const uint32_t channel = 0;
    NRF_SAADC->CH[channel].PSELP = fd_adcm_nrf5_pselp_for_gpio(gpio);
    const uint32_t gain = fd_adcm_nrf5_gain_for_voltage(max_voltage);
    NRF_SAADC->CH[channel].CONFIG =
        (SAADC_CH_CONFIG_REFSEL_Internal << SAADC_CH_CONFIG_REFSEL_Pos) | // 0.6V reference
        (gain << SAADC_CH_CONFIG_GAIN_Pos) |
        (SAADC_CH_CONFIG_TACQ_40us << SAADC_CH_CONFIG_TACQ_Pos) |
        (SAADC_CH_CONFIG_MODE_SE << SAADC_CH_CONFIG_MODE_Pos);

    NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Enabled;
    NRF_SAADC->TASKS_START = 1;
    NRF_SAADC->TASKS_SAMPLE = 1;
	
    // wait for conversion to end
    while (!NRF_SAADC->EVENTS_END) {}
    NRF_SAADC->EVENTS_END = 0;

    NRF_SAADC->TASKS_STOP = 1;

    NRF_SAADC->CH[channel].PSELP = SAADC_CH_PSELP_PSELP_NC;
    NRF_SAADC->CH[channel].CONFIG = 0;
    NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Disabled;

    if (adc_result < 0) {
        adc_result = 0;
    }
    float factor = fd_adcm_nrf5_gain_factor(gain);
    float voltage = adc_result * (0.6f / (factor * (1 << 14)));
    return voltage;
}

void fd_adcm_initialize(void) {
}
