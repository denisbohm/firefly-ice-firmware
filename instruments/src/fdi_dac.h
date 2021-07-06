#ifndef FDI_DAC_H
#define FDI_DAC_H

#include <stdint.h>

void fdi_dac_initialize(void);

void fdi_dac_set(uint32_t channel, uint32_t value);

#endif