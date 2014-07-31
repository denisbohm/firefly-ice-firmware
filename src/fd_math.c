#include "fd_math.h"

#define Minus32 0x80000000

// return square root of 32-bit number in 16.16 format
// (see SLAA024)
uint32_t fd_math_isqrt(uint32_t x) {
    uint32_t h = x;
    x = 0;
    uint32_t y = 0;
    for (uint32_t i = 0; i < 32; i++) {
        x <<= 1; x++; // 4 * x + 1
        if (y < x) {
            x -= 2;
        } else {
            y -= x;
        }
        x++;
        y <<= 1;
        if (h & Minus32) {
            y++;
        }
        h <<= 1;
        y <<= 1;
        if (h & Minus32) {
            y++;
        }
        h <<= 1;
    }
    return x;
}
