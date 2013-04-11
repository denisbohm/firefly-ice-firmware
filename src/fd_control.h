#ifndef FD_CONTROL_H
#define FD_CONTROL_H

#include <stdint.h>

void fd_control_initialize(void);

void fd_control_process(uint8_t *data, uint32_t length);

#endif