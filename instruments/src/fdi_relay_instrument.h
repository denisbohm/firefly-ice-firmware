#ifndef FDI_RELAY_INSTRUMENT_H
#define FDI_RELAY_INSTRUMENT_H

#include "fdi_instrument.h"

typedef struct {
    fdi_instrument_t super;
    uint32_t control;
} fdi_relay_instrument_t;

void fdi_relay_instrument_initialize(void);

uint32_t fdi_relay_instrument_get_count(void);
fdi_relay_instrument_t *fdi_relay_instrument_get_at(uint32_t index);

void fdi_relay_instrument_reset(fdi_relay_instrument_t *instrument);
void fdi_relay_instrument_set(fdi_relay_instrument_t *instrument, bool on);

#endif
