#ifndef FDI_COLOR_INSTRUMENT_H
#define FDI_COLOR_INSTRUMENT_H

#include "fdi_instrument.h"
#include "fdi_tcs3471.h"

typedef struct {
    fdi_instrument_t super;
    uint8_t address;
} fdi_color_instrument_t;

void fdi_color_instrument_initialize(void);

uint32_t fdi_color_instrument_get_count(void);
fdi_color_instrument_t *fdi_color_instrument_get_at(uint32_t index);

void fdi_color_instrument_reset(fdi_color_instrument_t *instrument);
fdi_tcs3471_conversion_t fdi_color_instrument_convert(fdi_color_instrument_t *instrument, float integration_time, float gain);

#endif
