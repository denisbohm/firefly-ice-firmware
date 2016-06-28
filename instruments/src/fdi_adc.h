#ifndef FDI_ADC_H
#define FDI_ADC_H

#include <stdint.h>

void fdi_adc_initialize(void);
void fdi_adc_power_up(void);
float fdi_adc_convert(uint32_t channel);
void fdi_adc_power_down(void);

#endif