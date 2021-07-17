#ifndef FDI_ADC_H
#define FDI_ADC_H

#include <stdint.h>

void fdi_adc_initialize(void);

void fdi_adc_power_up(void);
void fdi_adc_power_down(void);

void fdi_adc_setup(uint32_t channel);
float fdi_adc_convert(uint32_t channel);

// callback is called from interrupt context with dma buffer of values, so return ASAP
// (typically by copying data into a buffer and signaling an event to be handled in the main event loop).
typedef void (*fdi_adc_callback_t)(volatile uint16_t *results, uint32_t size);

void fdi_adc_convert_continuous(
    uint8_t *channels,
    uint32_t channel_count,
    volatile uint16_t *buffer_0,
    volatile uint16_t *buffer_1,
    uint32_t buffer_length,
    fdi_adc_callback_t callback
);

#endif