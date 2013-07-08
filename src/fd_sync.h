#ifndef FD_SYNC_H
#define FD_SYNC_H

#include "fd_detour.h"

#include <stdint.h>

void fd_sync_initialize(void);

void fd_sync_start(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length);
void fd_sync_ack(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length);

#endif