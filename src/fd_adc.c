#include "fd_adc.h"
#include "fd_processor.h"

#include <em_adc.h>

void fd_adc_initialize(void) {
  ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
  init.timebase = ADC_TimebaseCalc(0);
  init.prescale = ADC_PrescaleCalc(7000000, 0);
  ADC_Init(ADC0, &init);

  ADC_InitScan_TypeDef scanInit = ADC_INITSCAN_DEFAULT;
  scanInit.reference = adcRefVDD;
  scanInit.input = BAT_VDIV2_ADC_CHANNEL | CHG_RATE_ADC_CHANNEL;
  ADC_InitScan(ADC0, &scanInit);
}