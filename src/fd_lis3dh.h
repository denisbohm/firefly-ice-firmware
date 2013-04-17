#ifndef FD_LIS3DH_H
#define FD_LIS3DH_H

void fd_lis3dh_initialize(void);

void fd_lis3dh_read(float *x, float *y, float *z);

#endif