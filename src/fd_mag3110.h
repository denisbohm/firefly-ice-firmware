#ifndef FD_MAG3110_H
#define FD_MAG3110_H

void fd_mag3110_initialize(void);

void fd_mag3110_sleep(void);
void fd_mag3110_wake(void);

void fd_mag3110_read(float *x, float *y, float *z);

#endif