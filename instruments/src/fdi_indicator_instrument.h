#ifndef FDI_INDICATOR_INSTRUMENT_H
#define FDI_INDICATOR_INSTRUMENT_H

#include "fdi_instrument.h"

typedef struct {
    fdi_instrument_t super;
    uint32_t led_r;
    uint32_t led_g;
    uint32_t led_b;
} fdi_indicator_instrument_t;

void fdi_indicator_instrument_initialize(void);

uint32_t fdi_indicator_instrument_get_count(void);
fdi_indicator_instrument_t *fdi_indicator_instrument_get_at(uint32_t index);

void fdi_indicator_instrument_reset(fdi_indicator_instrument_t *instrument);
void fdi_indicator_instrument_set_rgb(fdi_indicator_instrument_t *instrument, float r, float g, float b);

#endif
