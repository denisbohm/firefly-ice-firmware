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
        GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 |
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

    GPIOA->ASCR = GPIO_InitStruct.Pin;
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
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.EOCSelection = EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.Overrun = OVR_DATA_OVERWRITTEN;
    HAL_StatusTypeDef status = HAL_ADC_Init(&hadc1);
    fd_log_assert(status == HAL_OK);

    status = HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
    fd_log_assert(status == HAL_OK);
}

void fdi_adc_power_down(void) {
    __HAL_RCC_ADC_CLK_DISABLE();

    HAL_StatusTypeDef status = HAL_ADC_DeInit(&hadc1);
    fd_log_assert(status == HAL_OK);
}

float fdi_adc_convert(uint32_t channel) {
    static const uint32_t adc_channel[] = {
        ADC_CHANNEL_0,
        ADC_CHANNEL_1,
        ADC_CHANNEL_2,
        ADC_CHANNEL_3,
        ADC_CHANNEL_4,
        ADC_CHANNEL_5,
        ADC_CHANNEL_6,
        ADC_CHANNEL_7,
        ADC_CHANNEL_8,
        ADC_CHANNEL_9,
        ADC_CHANNEL_10,
        ADC_CHANNEL_11,
        ADC_CHANNEL_12,
        ADC_CHANNEL_13,
        ADC_CHANNEL_14,
        ADC_CHANNEL_15,
        ADC_CHANNEL_16,
        ADC_CHANNEL_17,
        ADC_CHANNEL_18,
    };
    ADC_ChannelConfTypeDef sConfig;
    sConfig.Channel = adc_channel[channel];
    sConfig.Rank = 1;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    HAL_StatusTypeDef status = HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    fd_log_assert(status == HAL_OK);

    const int conversions = 32;
    uint32_t total = 0;
    for (int i = 0; i < conversions; ++i) {
        status = HAL_ADC_Start(&hadc1);
        fd_log_assert(status == HAL_OK);
        status = HAL_ADC_PollForConversion(&hadc1, 100);
        fd_log_assert(status == HAL_OK);
        uint32_t value = HAL_ADC_GetValue(&hadc1);
        status = HAL_ADC_Stop(&hadc1);
        fd_log_assert(status == HAL_OK);

        total += value;
    }
    return (float)total * (3.3f / 4095.0f / (float)conversions);
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
