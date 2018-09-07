#ifndef FD_ACTIVITY_H
#define FD_ACTIVITY_H

#include <stdint.h>

void fd_activity_initialize(void);
void fd_activity_prime(int16_t x, int16_t y, int16_t z);

void fd_activity_start(void);
void fd_activity_accumulate(int16_t x, int16_t y, int16_t z);
uint32_t fd_activity_get_sample_count(void);
float fd_activity_value(float time_interval);

#endif
