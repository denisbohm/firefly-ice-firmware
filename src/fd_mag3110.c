#include "fd_i2c1.h"
#include "fd_mag3110.h"

#define ADDRESS 0x1c
#define WHO_AM_I 0x07

void fd_mag3110_initialize(void) {
}

void fd_mag3110_test(void) {
    uint8_t who_am_i;
    bool result = fd_i2c1_read(ADDRESS, WHO_AM_I, &who_am_i);
    if (!result) {
        return;
    }
    if (who_am_i != 0xc4) {
        // log diagnostic
        return;
    }
}