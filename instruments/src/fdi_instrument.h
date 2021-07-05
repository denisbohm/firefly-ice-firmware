#ifndef FDI_INSTRUMENT_H
#define FDI_INSTRUMENT_H

#include "fd_binary.h"

#include "fdi_apic.h"

#include <stddef.h>
#include <stdint.h>

typedef struct fdi_instrument_s {
    const char *category;
    uint64_t identifier;
    void (*reset)(uint64_t identifier, uint64_t type, fd_binary_t *binary);
} fdi_instrument_t;

void fdi_instrument_initialize(void);

void fdi_instrument_register(fdi_instrument_t *instrument);

#endif