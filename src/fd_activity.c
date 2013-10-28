#include "fd_activity.h"
#include "fd_lis3dh.h"

#include <math.h>
#include <stdint.h>

#define ACTIVITY_K 0.1f

static float activity_acc_dc;
static float activity_vm;
static uint32_t activity_sample_count;

void fd_activity_initialize(void) {
    activity_acc_dc = 1.0f;
    activity_sample_count = 0;
    activity_vm = 0.0f;
}

void fd_activity_prime(int16_t x __attribute__((unused)), int16_t y __attribute__((unused)), int16_t z __attribute__((unused))) {
    activity_acc_dc = 1.0f;
}

void fd_activity_start(void) {
    activity_sample_count = 0;
    activity_vm = 0.0f;
}

#define Minus32 0x80000000

// return square root of 32-bit number in 16.16 format
// (see SLAA024)
static
uint32_t isqrt(uint32_t x) {
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

void fd_activity_accumulate(int16_t xg, int16_t yg, int16_t zg) {
    uint32_t x = xg;
    uint32_t y = yg;
    uint32_t z = zg;
    uint32_t t = isqrt(x * x + y * y + z * z);
    float v = t * (FD_LIS3DH_SCALE / 65536.0f);

    float value = v - activity_acc_dc;
    activity_acc_dc += value * ACTIVITY_K;
    if (value < 0.0f) {
        value = -value;
    }
    activity_vm += value;
    ++activity_sample_count;
}

float fd_activity_value(float time_interval __attribute__((unused))) {
    return 10.0f * activity_vm / activity_sample_count;
}