#ifndef FD_SYNC_H
#define FD_SYNC_H

#define FD_SYNC_START 1
#define FD_SYNC_DATA 2
#define FD_SYNC_ACK 3

void fd_sync_initialize(void);

void fd_sync_start(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length);
void fd_sync_ack(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length);

#endif