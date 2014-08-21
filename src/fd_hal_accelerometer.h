#ifndef FD_HAL_ACCELEROMETER_H
#define FD_HAL_ACCELEROMETER_H

#include <stdint.h>

#define FD_HAL_ACCELEROMETER_SCALE (1.0f / 4096.0f)

void fd_hal_accelerometer_initialize(void);

typedef void (*fd_hal_accelerometer_sample_callback_t)(int16_t x, int16_t y, int16_t z);

void fd_hal_accelerometer_set_sample_callback(fd_hal_accelerometer_sample_callback_t callback);

void fd_hal_accelerometer_sleep(void);
void fd_hal_accelerometer_wake(void);

void fd_hal_accelerometer_read_fifo(void);

void fd_hal_accelerometer_read(int16_t *x, int16_t *y, int16_t *z);

#endif