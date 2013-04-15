#ifndef FD_CONTROL_H
#define FD_CONTROL_H

#include "fd_detour.h"

#include <stdint.h>

void fd_control_initialize(void);

void fd_control_process(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length);

#endif