#ifndef FDI_CURRENT_INSTRUMENT_H
#define FDI_CURRENT_INSTRUMENT_H

#include "fdi_instrument.h"

typedef struct {
    fdi_instrument_t super;
    uint32_t enable;
    uint32_t channel;
    float multiplier;
} fdi_current_instrument_t;

void fdi_current_instrument_initialize(void);

uint32_t fdi_current_instrument_get_count(void);
fdi_current_instrument_t *fdi_current_instrument_get_at(uint32_t index);

void fdi_current_instrument_reset(fdi_current_instrument_t *instrument);
float fdi_current_instrument_convert(fdi_current_instrument_t *instrument);
void fdi_current_instrument_set_enabled(fdi_current_instrument_t *instrument, bool enabled);

#endif
