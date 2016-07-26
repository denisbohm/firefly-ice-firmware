#ifndef FDI_INSTRUMENTS_H
#define FDI_INSTRUMENTS_H

#include "fdi_gpio.h"

#include <stdint.h>

void fdi_instruments_initialize(void);

uint64_t fdi_instruments_register(void);

#endif