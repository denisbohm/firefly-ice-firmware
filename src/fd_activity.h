#ifndef FD_ACTIVITY_H
#define FD_ACTIVITY_H

void fd_activity_initialize(void);

void fd_activity_prime(float x, float y, float z);

void fd_activity_start(void);

void fd_activity_accumulate(float x, float y, float z);

float fd_activity_value(float time_interval);

#endif