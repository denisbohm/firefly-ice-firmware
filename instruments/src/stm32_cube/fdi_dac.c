#include "fdi_dac.h"

#include "fdi_stm32.h"

#include "fd_log.h"

static const uint32_t fdi_dac_channel_lookup[] = {
    DAC_CHANNEL_1,
    DAC_CHANNEL_2,
};

typedef struct {
    DAC_HandleTypeDef dac;
} fdi_dac_t;

fdi_dac_t fdi_dac;

void fdi_dac_setup(uint32_t channel) {
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = 1 << channel;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

#if defined(GPIO_ASCR_ASC0)
    GPIOA->ASCR |= GPIO_InitStruct.Pin;
#endif

    uint32_t dac_channel = fdi_dac_channel_lookup[channel];
    DAC_ChannelConfTypeDef channel_conf = {
        .DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE,
        .DAC_Trigger = DAC_TRIGGER_NONE,
        .DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE,
        .DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE,
        .DAC_UserTrimming = DAC_TRIMMING_FACTORY,
    };
    HAL_StatusTypeDef status = HAL_DAC_ConfigChannel(&fdi_dac.dac, &channel_conf, dac_channel);
    fd_log_assert(status == HAL_OK);

    status = HAL_DAC_SetValue(&fdi_dac.dac, dac_channel, DAC_ALIGN_12B_R, 0);
    fd_log_assert(status == HAL_OK);

    status = HAL_DAC_Start(&fdi_dac.dac, dac_channel);
    fd_log_assert(status == HAL_OK);
}

void fdi_dac_initialize(void) {
    __HAL_RCC_DAC1_CLK_ENABLE();

    fdi_dac.dac.Instance = DAC1;
    HAL_StatusTypeDef status = HAL_DAC_Init(&fdi_dac.dac);
    fd_log_assert(status == HAL_OK);

#ifdef FDI_INSTRUMENT_POWER
    const uint32_t channel = 0;
    fdi_dac_setup(channel);
    fdi_dac_set(channel, 0x180);
#endif
}

void fdi_dac_set(uint32_t channel, uint32_t value) {
    HAL_StatusTypeDef status = HAL_DAC_SetValue(&fdi_dac.dac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, value);
    fd_log_assert(status == HAL_OK);
}
