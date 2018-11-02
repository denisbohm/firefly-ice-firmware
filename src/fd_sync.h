#ifndef FD_SYNC_H
#define FD_SYNC_H

#include "fd_packet.h"

#include <stdint.h>

void fd_sync_initialize(void);

void fd_sync_start(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length);
void fd_sync_ack(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length);

#endif
