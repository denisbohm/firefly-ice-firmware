#ifndef FD_PACKET_H
#define FD_PACKET_H

#include "fd_lock.h"

#include <stdbool.h>
#include <stdint.h>

typedef bool (*fd_packet_write_t)(const uint8_t *data, uint32_t length);

typedef bool (*fd_packet_is_available_t)(void);

typedef void (*fd_packet_callback_t)(void);

typedef struct fd_packet_output_s {
    fd_packet_write_t write;
    
    fd_lock_owner_t owner;
    fd_packet_is_available_t is_available;
    fd_packet_callback_t callback;
    uint32_t subscribed_properties;
    uint32_t notify_properties;
    struct fd_packet_output_s *next;
} fd_packet_output_t;

extern fd_packet_output_t *fd_packet_output_head;

void fd_packet_initialize(void);

void fd_packet_output_initialize(fd_packet_output_t *packet_output, fd_lock_owner_t owner, fd_packet_is_available_t is_available, fd_packet_write_t write);

void fd_packet_output_unavailable(fd_packet_output_t *packet_output);

#endif
