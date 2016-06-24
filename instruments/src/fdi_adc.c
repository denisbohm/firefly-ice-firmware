#include "fdi_adc.h"

#include <stm32f4xx.h>

// PA3 ADC1_3 ATE_MCU1_VCC2
// PA4 ADC1_4 ATE_BS_SHR
// PA5 ADC1_5 ATE_BS_SLR
// PA6 ADC1_6 ATE_USB_CS_OUT
// PA7 ADC1_7 ATE_MCU2_VCC2
// PC5 ADC1_15 ATE_BATTERY+2

void fdi_adc_initialize(void) {
    GPIO_InitTypeDef gpio_init = { 0 };
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
    adc_init.ADC_DataAlign = ADC_DataAlign_Right;
    adc_init.ADC_Resolution = ADC_Resolution_12b;
    adc_init.ADC_ContinuousConvMode = ENABLE;
    adc_init.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
    adc_init.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    adc_init.ADC_NbrOfConversion = 1;
    adc_init.ADC_ScanConvMode = DISABLE;
    ADC_Init(ADC1, &adc_init);
    ADC_Cmd(ADC1, ENABLE);

    ADC_TempSensorVrefintCmd(ENABLE);

    float voltage = fdi_adc_convert(ADC_Channel_Vrefint);
    float delta = voltage - 1.21f;
}

void fdi_adc_power_down(void) {
    ADC_Cmd(ADC1, DISABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);
    ADC_DeInit();
}

float fdi_adc_convert(uint32_t channel) {
    ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_480Cycles);
    ADC_SoftwareStartConv(ADC1);
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)) {
    }
    uint16_t value = ADC_GetConversionValue(ADC1);
    return value * 3.3f / 4096.0f;
}