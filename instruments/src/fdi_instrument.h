#ifndef FDI_INSTRUMENT_H
#define FDI_INSTRUMENT_H

#include "fdi_gpio.h"

#include <stdint.h>

typedef struct fdi_instrument_s {
    const char *category;
    uint64_t identifier;
} fdi_instrument_t;

void fdi_instrument_initialize(void);

void fdi_instrument_register(fdi_instrument_t *instrument);

#endif