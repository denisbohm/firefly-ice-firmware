#ifndef FDI_VOLTAGE_INSTRUMENT_H
#define FDI_VOLTAGE_INSTRUMENT_H

#include "fdi_instrument.h"

typedef struct {
    fdi_instrument_t super;
    uint32_t channel;
    float multiplier;
} fdi_voltage_instrument_t;

void fdi_voltage_instrument_initialize(void);

uint32_t fdi_voltage_instrument_get_count(void);
fdi_voltage_instrument_t *fdi_voltage_instrument_get_at(uint32_t index);

void fdi_voltage_instrument_reset(fdi_voltage_instrument_t *instrument);
float fdi_relay_instrument_convert(fdi_voltage_instrument_t *instrument);

#endif
