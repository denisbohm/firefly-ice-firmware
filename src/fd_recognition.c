#include "fd_lis3dh.h"
#include "fd_math.h"
#include "fd_recognition.h"
#include "fd_sensing.h"

#include <stdbool.h>

// !!! Super simple human activity recognition just for testing all the other bits and pieces. -denis
// Match any activity that results in greater than 2g acceleration.

// recognize acceleration over 2g
#define FD_RECOGNITION_THRESHOLD 2.0f
// record raw activity for 2 seconds after event detection
#define FD_RECOGNITION_AFTER_COUNT 50
// ignore raw activity for 2 seconds after event detection
#define FD_RECOGNITION_AFTER_SKIP 50

static bool fd_recognition_enable;
static uint32_t fd_recognition_skip_count;

void fd_recognition_initialize(void) {
    fd_recognition_enable = false;
    fd_recognition_skip_count = 0;
}

bool fd_recognition_get_enable(void) {
    return fd_recognition_enable;
}

void fd_recognition_set_enable(bool enable) {
    fd_recognition_enable = enable;
}

void fd_recognition_match(void) {
    fd_sensing_history_save();

    if (fd_sensing_get_stream_sample_count() < FD_RECOGNITION_AFTER_COUNT) {
        fd_sensing_set_stream_sample_count(FD_RECOGNITION_AFTER_COUNT);
    }
}

void fd_recognition_sensing(int16_t x, int16_t y, int16_t z) {
    if (fd_recognition_skip_count > 0) {
        --fd_recognition_skip_count;
        return;
    }

    if (!fd_recognition_enable) {
        return;
    }

    bool match = false;

    float a = fd_math_isqrt(x * x + y * y + z * z) * (FD_LIS3DH_SCALE / 65536.0f);
    if (a > FD_RECOGNITION_THRESHOLD) {
        match = true;
    }

    if (match) {
        fd_recognition_match();
        fd_recognition_skip_count = FD_RECOGNITION_AFTER_SKIP;
    }
}