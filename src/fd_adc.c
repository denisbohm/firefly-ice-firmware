#include "fd_adc.h"
#include "fd_processor.h"

#include <em_adc.h>
#include <em_cmu.h>

#include <stdint.h>

#define BAT_VDIV2_ADC_CHANNEL adcSingleInpCh6
#define CHG_RATE_ADC_CHANNEL adcSingleInpCh7

static volatile uint32_t adc_bat_vdiv2_value;
static volatile bool adc_complete;

void fd_adc_initialize(void) {
    adc_bat_vdiv2_value = 0;
    adc_complete = true;

    CMU_ClockEnable(cmuClock_ADC0, true);

    ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
    init.timebase = ADC_TimebaseCalc(0);
    init.prescale = ADC_PrescaleCalc(7000000, 0);
    ADC_Init(ADC0, &init);

    ADC_InitSingle_TypeDef singleInit = ADC_INITSINGLE_DEFAULT;
    singleInit.reference = adcRefVDD;
    singleInit.acqTime = adcAcqTime256;
    singleInit.input = adcSingleInpCh7;
    ADC_InitSingle(ADC0, &singleInit);

    ADC_IntEnable(ADC0, ADC_IF_SINGLE);
    NVIC_EnableIRQ(ADC0_IRQn);

    CMU_ClockEnable(cmuClock_ADC0, false);
}

void fd_adc_sleep(void) {
}

void fd_adc_wake(void) {
}

bool fd_adc_in_progress(void) {
    return !adc_complete;
}

void ADC0_IRQHandler(void) {
    adc_bat_vdiv2_value = ADC_DataSingleGet(ADC0);
    adc_complete = true;
    ADC_IntClear(ADC0, ADC_IF_SINGLE);
    NVIC_ClearPendingIRQ(ADC0_IRQn);
    CMU_ClockEnable(cmuClock_ADC0, false);
}

void fd_adc_start(void) {
    adc_complete = false;
    CMU_ClockEnable(cmuClock_ADC0, true);
    ADC_Start(ADC0, adcStartSingle);
}
