#include "fd_adc.h"
#include "fd_processor.h"

#include <em_adc.h>
#include <em_cmu.h>
#include <em_gpio.h>

#include <stdint.h>

static volatile uint32_t adc_temperature_value;
static volatile uint32_t adc_battery_voltage_value;
static volatile uint32_t adc_charge_current_value;

static volatile bool adc_complete;
static volatile fd_adc_channel_t adc_channel;

void fd_adc_initialize(void) {
    fd_adc_start(fd_adc_channel_temperature, true);
    fd_adc_start(fd_adc_channel_battery_voltage, true);
    fd_adc_start(fd_adc_channel_charge_current, true);
}

#define CAL_TEMP_0 ((uint8_t *)0x0FE081B2)
#define ADC0_TEMP_0_READ_1V25 ((uint16_t *)0x0FE081BE)

#define TGRAD_ADCTH -6.3f

float fd_adc_get_temperature(void) {
    uint8_t cal_temp_0 = *CAL_TEMP_0;
    uint16_t temp_0_read = (*ADC0_TEMP_0_READ_1V25) >> 4;
    return cal_temp_0 + (temp_0_read - adc_temperature_value) * (1.0f / TGRAD_ADCTH);
}

float fd_adc_get_battery_voltage(void) {
    return adc_battery_voltage_value * (2.0f * 2.2f / 4095.0f);
}

float fd_adc_get_charge_current(void) {
    return adc_charge_current_value * (2.2f / 4095.0f) * (50.0f / 1.22f);
}

void fd_adc_sleep(void) {
}

void fd_adc_wake(void) {
}

bool fd_adc_in_progress(void) {
    return !adc_complete;
}

void ADC0_IRQHandler(void) {
    uint32_t value = ADC_DataSingleGet(ADC0);
    switch (adc_channel) {
        case fd_adc_channel_temperature:
            adc_temperature_value = value;
        break;
        case fd_adc_channel_battery_voltage:
            GPIO_PinOutClear(BAT_VDIV2EN_PORT_PIN);
            adc_battery_voltage_value = value;
        break;
        case fd_adc_channel_charge_current:
            adc_charge_current_value = value;
        break;
    }
    adc_complete = true;
    ADC_IntClear(ADC0, ADC_IF_SINGLE);
    NVIC_ClearPendingIRQ(ADC0_IRQn);
    CMU_ClockEnable(cmuClock_ADC0, false);
}

void fd_adc_start(fd_adc_channel_t channel, bool asynchronous) {
    CMU_ClockEnable(cmuClock_ADC0, true);

    ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
    init.timebase = ADC_TimebaseCalc(0);
    init.prescale = ADC_PrescaleCalc(7000000, 0);

    ADC_InitSingle_TypeDef singleInit = ADC_INITSINGLE_DEFAULT;
    singleInit.reference = adcRefVDD;
    singleInit.acqTime = adcAcqTime256;

    switch (channel) {
        case fd_adc_channel_temperature:
            singleInit.reference = adcRef1V25;
            singleInit.input = adcSingleInpTemp;
        break;
        case fd_adc_channel_battery_voltage:
            GPIO_PinOutSet(BAT_VDIV2EN_PORT_PIN);
            singleInit.input = adcSingleInpCh6;
        break;
        case fd_adc_channel_charge_current:
            singleInit.input = adcSingleInpCh7;
        break;
    }

    ADC_Init(ADC0, &init);
    ADC_InitSingle(ADC0, &singleInit);

    ADC_IntEnable(ADC0, ADC_IF_SINGLE);
    if (asynchronous) {
        NVIC_EnableIRQ(ADC0_IRQn);
    }

    adc_channel = channel;
    adc_complete = false;
    ADC_Start(ADC0, adcStartSingle);

    if (!asynchronous) {
        while ((ADC_IntGet(ADC0) & ADC_IF_SINGLE) == 0);
        ADC0_IRQHandler();
    }
}