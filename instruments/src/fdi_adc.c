#include "fdi_adc.h"

#include "fdi_delay.h"

#include <stm32f4xx.h>

void fdi_adc_initialize(void) {
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