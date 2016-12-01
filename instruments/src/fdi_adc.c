#include "fdi_adc.h"

#include "fdi_delay.h"

#include <stm32f4xx.h>

static fdi_adc_callback_t fdi_adc_callback;

void fdi_adc_initialize(void) {
    fdi_adc_callback = 0;

    GPIO_InitTypeDef gpio_init;
    GPIO_StructInit(&gpio_init);
    gpio_init.GPIO_Mode = GPIO_Mode_AN;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    gpio_init.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOA, &gpio_init);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    gpio_init.GPIO_Pin = GPIO_Pin_5;
    GPIO_Init(GPIOC, &gpio_init);
}

void fdi_adc_power_up(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    ADC_DeInit();
    ADC_InitTypeDef adc_init;
    ADC_StructInit(&adc_init);
    adc_init.ADC_DataAlign = ADC_DataAlign_Right;
    adc_init.ADC_Resolution = ADC_Resolution_12b;
    adc_init.ADC_ContinuousConvMode = ENABLE;
    adc_init.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
    adc_init.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    adc_init.ADC_NbrOfConversion = 1;
    adc_init.ADC_ScanConvMode = DISABLE;
    ADC_Init(ADC1, &adc_init);
    ADC_Cmd(ADC1, ENABLE);

    ADC_CommonInitTypeDef adc_common_init;
    ADC_CommonStructInit(&adc_common_init);
    adc_common_init.ADC_Prescaler = ADC_Prescaler_Div8;
    ADC_CommonInit(&adc_common_init);

    ADC_TempSensorVrefintCmd(ENABLE);
}

void fdi_adc_power_down(void) {
    ADC_Cmd(ADC1, DISABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);
    ADC_DeInit();
    DMA_DeInit(DMA2_Stream0);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, DISABLE);               
}

float fdi_adc_convert(uint32_t channel) {
    const int conversions = 32;
    uint32_t value = 0;
    for (int i = 0; i < conversions; ++i) {
        ADC_ClearFlag(ADC1, ADC_FLAG_OVR | ADC_FLAG_EOC);
        ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_480Cycles);
        fdi_delay_us(10);
        do {
            ADC_SoftwareStartConv(ADC1);
            if (ADC_GetFlagStatus(ADC1, ADC_FLAG_OVR)) {
                ADC1->SR &= ~ADC_FLAG_OVR;
            }
        } while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));

        value += ADC_GetConversionValue(ADC1);
    }
    return value * 3.3f / 4096.0f / conversions;
}

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