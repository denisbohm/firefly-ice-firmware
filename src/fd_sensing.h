#ifndef FD_SENSING_H
#define FD_SENSING_H

#include "fd_detour.h"

void fd_sensing_initialize(void);

void fd_sensing_push(
    fd_detour_source_collection_t *detour_source_collection,
    float ax, float ay, float az,
    float mx, float my, float mz
);

#endif