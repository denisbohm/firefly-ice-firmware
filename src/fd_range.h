#ifndef FD_RANGE_H
#define FD_RANGE_H

#include <stdint.h>

typedef struct {
    uint32_t address;
    uint32_t length;
} fd_range_t;

static inline fd_range_t fd_range_make(uint32_t address, uint32_t length) {
    fd_range_t range = {
        .address = address,
        .length = length
    };
    return range;
}

#endif