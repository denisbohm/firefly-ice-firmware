#ifndef FDI_GPIO_INSTRUMENT_H
#define FDI_GPIO_INSTRUMENT_H

#include "fdi_instrument.h"

typedef struct {
    fdi_instrument_t super;
    uint32_t control;
} fdi_gpio_instrument_t;

void fdi_gpio_instrument_initialize(void);

uint32_t fdi_gpio_instrument_get_count(void);
fdi_gpio_instrument_t *fdi_gpio_instrument_get_at(uint32_t index);

void fdi_gpio_instrument_reset(fdi_gpio_instrument_t *instrument);
void fdi_gpio_instrument_set_bit(fdi_gpio_instrument_t *instrument, bool on);
bool fdi_gpio_instrument_get_bit(fdi_gpio_instrument_t *instrument);
void fdi_gpio_instrument_set_mode_bit(fdi_gpio_instrument_t *instrument, fdi_gpio_mode_t mode);

#endif
