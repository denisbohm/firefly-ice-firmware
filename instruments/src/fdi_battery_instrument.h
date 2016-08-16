#ifndef FDI_BATTERY_INSTRUMENT_H
#define FDI_BATTERY_INSTRUMENT_H

#include "fdi_instrument.h"

typedef struct {
    fdi_instrument_t super;
    uint32_t enable;
    uint32_t channel_high;
    float multiplier_high;
    uint32_t channel_low;
    float multiplier_low;
} fdi_battery_instrument_t;

void fdi_battery_instrument_initialize(void);

uint32_t fdi_battery_instrument_get_count(void);
fdi_battery_instrument_t *fdi_battery_instrument_get_at(uint32_t index);

void fdi_battery_instrument_reset(fdi_battery_instrument_t *instrument);
float fdi_battery_instrument_convert(fdi_battery_instrument_t *instrument);
void fdi_battery_instrument_set_voltage(fdi_battery_instrument_t *instrument, float voltage);
void fdi_battery_instrument_set_enabled(fdi_battery_instrument_t *instrument, bool enabled);

#endif
