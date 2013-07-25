#include "fd_activity.h"

#include <math.h>
#include <stdint.h>

#define FD_ACTIVITY_K 0.25f

static float fd_activity_vm;
static float fd_activity_total;
static uint32_t fd_activity_count;

void fd_activity_initialize(void) {
    fd_activity_vm = 1.0f;
    fd_activity_total = 0.0f;
    fd_activity_count = 0;
}

void fd_activity_prime(float x, float y, float z) {
    fd_activity_vm = sqrt(x * x + y * y + z * z);
}

void fd_activity_start(void) {
    fd_activity_total = 0.0f;
    fd_activity_count = 0;
}

void fd_activity_accumulate(float x, float y, float z) {
    float vm = sqrt(x * x + y * y + z * z);
    float d = vm - fd_activity_vm;
    fd_activity_vm += d * FD_ACTIVITY_K;
    fd_activity_total += fabs(d);
    ++fd_activity_count;
}

float fd_activity_value(float time_interval) {
    return fd_activity_total / (fd_activity_count * time_interval);
}