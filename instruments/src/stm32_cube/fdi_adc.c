#include "fdi_adc.h"

#include "fdi_delay.h"

#include "fdi_stm32.h"

#include "fd_log.h"

static fdi_adc_callback_t fdi_adc_callback;

ADC_HandleTypeDef hadc1;

void fdi_adc_initialize(void) {
    fdi_adc_callback = 0;

    __GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct;
#ifdef FDI_INSTRUMENT_SERIAL_WIRE
    GPIO_InitStruct.Pin =
        GPIO_PIN_0 | GPIO_PIN_1;
#endif
#ifdef FDI_INSTRUMENT_INPUT_OUTPUT
    GPIO_InitStruct.Pin =
        GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |
        GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_9 |
        GPIO_PIN_10 | GPIO_PIN_15;
#endif
#ifdef FDI_INSTRUMENT_POWER
    GPIO_InitStruct.Pin =
        GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_5;
#endif
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void fdi_adc_power_up(void) {
    __HAL_RCC_ADC_CLK_ENABLE();

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV1;
    hadc1.Init.Resolution = ADC_RESOLUTION12b;
    hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 2;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.EOCSelection = EOC_SEQ_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.Overrun = OVR_DATA_OVERWRITTEN;
    HAL_ADC_Init(&hadc1);
}

void fdi_adc_power_down(void) {
    __HAL_RCC_ADC_CLK_DISABLE();

    HAL_ADC_DeInit(&hadc1);
}

float fdi_adc_convert(uint32_t channel) {
    ADC_ChannelConfTypeDef sConfig;
    sConfig.Channel = channel; // ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    const int conversions = 32;
    uint32_t value = 0;
    for (int i = 0; i < conversions; ++i) {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 100);
        uint32_t result = HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);

        value += result;
    }
    return value * 3.3f / 4096.0f / conversions;
}

void fdi_adc_convert_continuous(
    uint8_t *channels,
    uint32_t channel_count,
    volatile uint16_t *buffer_0,
    volatile uint16_t *buffer_1,
    uint32_t buffer_length,
    fdi_adc_callback_t callback
) {
    fd_log_assert("unimplemented");
}
