#ifndef FDI_STORAGE_INSTRUMENT_H
#define FDI_STORAGE_INSTRUMENT_H

#include "fdi_instrument.h"

typedef struct {
    fdi_instrument_t super;
} fdi_storage_instrument_t;

void fdi_storage_instrument_initialize(void);

uint32_t fdi_storage_instrument_get_count(void);
fdi_storage_instrument_t *fdi_storage_instrument_get_at(uint32_t index);
fdi_storage_instrument_t *fdi_storage_instrument_get(uint64_t identifier);

void fdi_storage_instrument_reset(fdi_storage_instrument_t *instrument);
void fdi_storage_instrument_read(
    fdi_storage_instrument_t *instrument,
    uint32_t address, uint32_t length, uint32_t sublength, uint32_t substride,
    uint8_t *buffer, uint32_t size
);

#endif
