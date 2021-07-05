#include "fdi_adc.h"

#include "fdi_delay.h"

#include "fdi_stm32.h"

static fdi_adc_callback_t fdi_adc_callback;

ADC_HandleTypeDef hadc1;

void fdi_adc_initialize(void) {
    fdi_adc_callback = 0;

    __GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin =
        GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |
        GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_9 |
        GPIO_PIN_10 | GPIO_PIN_15;
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

#ifndef FDI_ADC_CONVERT_CONTINUOUS_UNSUPPORTED
    DMA_DeInit(DMA2_Stream0);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, DISABLE);
#endif
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

#ifndef FDI_ADC_CONVERT_CONTINUOUS_UNSUPPORTED
volatile uint16_t *fdi_adc_dma_buffer_0;
volatile uint16_t *fdi_adc_dma_buffer_1;
uint32_t fdi_adc_dma_buffer_length;

void fdi_adc_convert_continuous(
    uint8_t *channels,
    uint32_t channel_count,
    volatile uint16_t *buffer_0,
    volatile uint16_t *buffer_1,
    uint32_t buffer_length,
    fdi_adc_callback_t callback
) {
    fdi_adc_dma_buffer_0 = buffer_0;
    fdi_adc_dma_buffer_1 = buffer_1;
    fdi_adc_dma_buffer_length = buffer_length;
    fdi_adc_callback = callback;

    /* Enable DMA2, thats where ADC is hooked on -> see Table 20 (RM00090) */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);               
    DMA_InitTypeDef dma_init;
    DMA_StructInit(&dma_init);
    dma_init.DMA_Channel = DMA_Channel_0;                     
    dma_init.DMA_BufferSize = buffer_length;
    dma_init.DMA_DIR = DMA_DIR_PeripheralToMemory;
    dma_init.DMA_FIFOMode = DMA_FIFOMode_Disable;
    dma_init.DMA_FIFOThreshold = 0;
    dma_init.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    dma_init.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    dma_init.DMA_Mode = DMA_Mode_Circular;
    dma_init.DMA_Priority = DMA_Priority_High;
    dma_init.DMA_Memory0BaseAddr = (uint32_t)fdi_adc_dma_buffer_0;
    dma_init.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    dma_init.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma_init.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    dma_init.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_Init(DMA2_Stream0, &dma_init);
    DMA_DoubleBufferModeConfig(DMA2_Stream0, (uint32_t)fdi_adc_dma_buffer_1, DMA_Memory_0);
    DMA_DoubleBufferModeCmd(DMA2_Stream0, ENABLE);
    DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);
    DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, ENABLE);
    DMA_Cmd(DMA2_Stream0, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    ADC_CommonInitTypeDef adc_common_init;
    ADC_CommonStructInit(&adc_common_init);
    ADC_CommonInit(&adc_common_init);

    ADC_InitTypeDef adc_init;
    ADC_StructInit(&adc_init);
    adc_init.ADC_Resolution = ADC_Resolution_12b;
    adc_init.ADC_ScanConvMode = ENABLE;
    adc_init.ADC_ContinuousConvMode = ENABLE;
    adc_init.ADC_ExternalTrigConvEdge = 0;
    adc_init.ADC_ExternalTrigConv = 0;
    adc_init.ADC_DataAlign = ADC_DataAlign_Right;
    adc_init.ADC_NbrOfConversion = channel_count;
    ADC_Init(ADC1, &adc_init);

    /* Configure channels */
    for (uint32_t i = 0; i < channel_count; ++i) {
        uint32_t channel = channels[i];
        ADC_RegularChannelConfig(ADC1, channel, i + 1, ADC_SampleTime_28Cycles);
    }

    // ADC Rate:
    // Conversion Clocks = 28 sample cycles + 12 bit conversion cycles = 40 cycles
    // Conversion Count = 1 high current range + 1 low current range
    // ADC Clock Rate = 84 MHz
    // 84 MHz / (40 cycles * 2) = 1.05 M samples per second

    /* Enable ADC interrupts */
//    ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);

    ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);

    ADC_DMACmd(ADC1, ENABLE);

    NVIC_InitTypeDef nvic_init;
    nvic_init.NVIC_IRQChannel = DMA2_Stream0_IRQn;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 0x0F;
    nvic_init.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_Init(&nvic_init);

    ADC_Cmd(ADC1, ENABLE);
    ADC_SoftwareStartConv(ADC1);
}

void DMA2_Stream0_IRQHandler(void) {
    if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_TCIF0) != RESET) {
        if (fdi_adc_callback) {
            volatile uint16_t *buffer = (DMA_GetCurrentMemoryTarget(DMA2_Stream0) == 0) ? fdi_adc_dma_buffer_1 : fdi_adc_dma_buffer_0;
            (*fdi_adc_callback)(buffer, fdi_adc_dma_buffer_length);
        }
        DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);
    }
}

#endif