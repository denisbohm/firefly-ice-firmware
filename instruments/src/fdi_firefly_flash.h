#ifndef FDI_FIREFLY_FLASH_H
#define FDI_FIREFLY_FLASH_H

#include "fd_range.h"

#include <stdint.h>

typedef struct {
    fd_range_t executable_range;
    uint8_t *executable_data;
    uint32_t page_length;
    uint32_t halt_address;
    uint32_t write_pages_address;
} fdi_firefly_flash_t;

#endif
