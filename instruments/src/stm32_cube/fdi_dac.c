#include "fdi_dac.h"

#include "fdi_stm32.h"

#include "fd_log.h"

typedef struct {
    DAC_HandleTypeDef dac;
} fdi_dac_t;

fdi_dac_t fdi_dac;

void fdi_dac_initialize(void) {
    fdi_dac.dac.Instance = DAC1;
    HAL_StatusTypeDef status = HAL_DAC_Init(&fdi_dac.dac);
    fd_log_assert(status == HAL_OK);

    DAC_ChannelConfTypeDef channel_conf = {
        .DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE,
        .DAC_Trigger = DAC_TRIGGER_NONE,
        .DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE,
        .DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE,
        .DAC_UserTrimming = DAC_TRIMMING_FACTORY,
    };
    // PA4
    status = HAL_DAC_ConfigChannel(&fdi_dac.dac, &channel_conf, DAC1_CHANNEL_1);
    fd_log_assert(status == HAL_OK);

    status = HAL_DAC_SetValue(&fdi_dac.dac, DAC1_CHANNEL_1, DAC_ALIGN_12B_R, 0x180);
    fd_log_assert(status == HAL_OK);

    status = HAL_DAC_Start(&fdi_dac.dac, DAC1_CHANNEL_1);
    fd_log_assert(status == HAL_OK);
}

void fdi_dac_set(uint32_t channel, uint32_t value) {
    HAL_StatusTypeDef status = HAL_DAC_SetValue(&fdi_dac.dac, DAC1_CHANNEL_1, DAC_ALIGN_12B_R, 0x180);
    fd_log_assert(status == HAL_OK);
}
