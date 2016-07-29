#ifndef FDI_TCS3471_H
#define FDI_TCS3471_H

#include <stdbool.h>
#include <stdint.h>

extern const uint8_t fdi_tcs34711_address;
extern const uint8_t fdi_tcs34715_address;

typedef struct {
    uint16_t c;
    uint16_t r;
    uint16_t g;
    uint16_t b;
} fdi_tcs3471_conversion_t;

bool fdi_tcs3471_convert(uint8_t address, uint8_t atime, uint8_t again, fdi_tcs3471_conversion_t *conversion);

#endif
