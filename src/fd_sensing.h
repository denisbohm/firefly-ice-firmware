#ifndef FD_SENSING_H
#define FD_SENSING_H

#include "fd_detour.h"

void fd_sensing_initialize(void);

void fd_sensing_wake(void);
void fd_sensing_sleep(void);

void fd_sensing_set_stream_sample_count(uint32_t count);
uint32_t fd_sensing_get_stream_sample_count(void);

void fd_sensing_synthesize(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length);

#endif