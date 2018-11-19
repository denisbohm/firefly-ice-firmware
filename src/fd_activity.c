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

#ifndef FD_ACTIVITY_MAGNITUDE_SCALE
#define FD_ACTIVITY_MAGNITUDE_SCALE (1.0 / 8192.0)
#endif

void fd_activity_accumulate(int16_t xg, int16_t yg, int16_t zg) {
    int32_t x = xg;
    int32_t y = yg;
    int32_t z = zg;
    uint32_t t = sqrt(x * x + y * y + z * z);
    float v = t * FD_ACTIVITY_MAGNITUDE_SCALE;

    float value = v - activity_acc_dc;
    activity_acc_dc += value * ACTIVITY_K;
    if (value < 0.0f) {
        value = -value;
    }
    activity_vm += value;
    ++activity_sample_count;
}

float fd_activity_value(float time_interval __attribute__((unused))) {
    return activity_vm / activity_sample_count;
}
