#ifndef FD_RECOGNITION_H
#define FD_RECOGNITION_H

#include <stdbool.h>

void fd_recognition_initialize(void);

bool fd_recognition_get_enable(void);
void fd_recognition_set_enable(bool enable);

void fd_recognition_sensing(int16_t x, int16_t y, int16_t z);

#endif