#ifndef FDI_STORAGE_INSTRUMENT_H
#define FDI_STORAGE_INSTRUMENT_H

#include "fdi_instrument.h"

#include "ff.h"

typedef struct {
    fdi_instrument_t super;
    FATFS fs;
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

bool fdi_storage_instrument_file_mkfs(fdi_storage_instrument_t *instrument);
bool fdi_storage_instrument_file_open(fdi_storage_instrument_t *instrument, const char *name, uint8_t mode);
bool fdi_storage_instrument_file_unlink(fdi_storage_instrument_t *instrument, const char *name);
bool fdi_storage_instrument_file_address(fdi_storage_instrument_t *instrument, const char *name, uint32_t *address);
bool fdi_storage_instrument_file_expand(fdi_storage_instrument_t *instrument, const char *name, uint32_t size);
bool fdi_storage_instrument_file_write(fdi_storage_instrument_t *instrument, const char *name, uint32_t offset, const uint8_t *data, uint32_t length);
bool fdi_storage_instrument_file_read(fdi_storage_instrument_t *instrument, const char *name, uint32_t offset, uint8_t *data, uint32_t length);

#endif
