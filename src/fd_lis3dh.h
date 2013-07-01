#ifndef FD_LIS3DH_H
#define FD_LIS3DH_H

void fd_lis3dh_initialize(void);

typedef void (*fd_lish3dh_sample_callback_t)(int16_t x, int16_t y, int16_t z);

void fd_lis3dh_set_sample_callback(fd_lish3dh_sample_callback_t callback);

void fd_lis3dh_sleep(void);
void fd_lis3dh_wake(void);

void fd_lis3dh_read(float *x, float *y, float *z);

#endif