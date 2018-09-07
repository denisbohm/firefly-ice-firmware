#include "fd_activity.h"
#include "fd_hal_accelerometer.h"
#include "fd_math.h"

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

uint32_t fd_activity_get_sample_count(void) {
    return activity_sample_count;
}

void fd_activity_start(void) {
    activity_sample_count = 0;
    activity_vm = 0.0f;
}

void fd_activity_accumulate(int16_t xg, int16_t yg, int16_t zg) {
    uint32_t x = xg;
    uint32_t y = yg;
    uint32_t z = zg;
    uint32_t t = fd_math_isqrt(x * x + y * y + z * z);
    float v = t * (FD_HAL_ACCELEROMETER_SCALE / 65536.0f);

    float value = v - activity_acc_dc;
    activity_acc_dc += value * ACTIVITY_K;
    if (value < 0.0f) {
        value = -value;
    }
    activity_vm += value;
    ++activity_sample_count;
}

// Scale activity value by a constant for compatibility with other meters.
// If compatibility isn't needed the scale can be removed (set to 1.0f).
#define SCALE (10.0f * 1.25f)

float fd_activity_value(float time_interval __attribute__((unused))) {
    return SCALE * activity_vm / activity_sample_count;
}
