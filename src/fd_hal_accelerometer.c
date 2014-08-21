#include "fd_hal_accelerometer.h"

#include "fd_lis3dh.h"

void fd_hal_accelerometer_initialize(void) {
    fd_lis3dh_initialize();
}

void fd_hal_accelerometer_set_sample_callback(fd_hal_accelerometer_sample_callback_t callback) {
    fd_lis3dh_set_sample_callback(callback);
}

void fd_hal_accelerometer_sleep(void) {
    fd_lis3dh_sleep();
}

void fd_hal_accelerometer_wake(void) {
    fd_lis3dh_wake();
}

void fd_hal_accelerometer_read_fifo(void) {
    fd_lis3dh_read_fifo();
}

void fd_hal_accelerometer_read(int16_t *x, int16_t *y, int16_t *z) {
    fd_lis3dh_read(x, y, z);
}
