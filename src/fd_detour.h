#ifndef FD_DETOUR_H
#define FD_DETOUR_H

/*
A "detour" is used to send arbitrary data via some other transport, such as USB or Bluetooth Low Energy.
Each of these other transports has small limits on the size of data that can be sent in one chunk.  The
detour can split and reassemble the arbitrary data into chunks that can be sent over the transport.

The first byte of each chunk is a sequence number, with the first chunk indicated by the sequence number 0.
In the first chunk the sequence number is followed by a uint16 length.  All subsequent data in chunks is
the content data.  The last chunk can have extra data (which will be ignored).
*/

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    fd_detour_state_clear,
    fd_detour_state_intermediate,
    fd_detour_state_success,
    fd_detour_state_error
} fd_detour_state_t;

typedef struct {
    uint8_t *data;
    uint32_t size;

    fd_detour_state_t state;
    uint32_t length;
    uint32_t sequence_number;
    uint32_t offset;
} fd_detour_t;

typedef void (*fd_detour_supplier_t)(uint32_t offset, uint8_t *data, uint32_t length);

typedef struct fd_detour_source {
    struct fd_detour_source *next;
    struct fd_detour_source *previous;

    fd_detour_supplier_t supplier;

    fd_detour_state_t state;
    uint32_t length;
    uint32_t sequence_number;
    uint32_t offset;
} fd_detour_source_t;

typedef struct {
    fd_detour_source_t *first;
    fd_detour_source_t *last;
} fd_detour_source_collection_t;

void fd_detour_initialize(fd_detour_t *detour, uint8_t *data, uint32_t size);

void fd_detour_clear(fd_detour_t *detour);

fd_detour_state_t fd_detour_state(fd_detour_t *detour);

void fd_detour_event(fd_detour_t *detour, uint8_t *data, uint32_t length);

void fd_detour_source_initialize(fd_detour_source_t *source, fd_detour_supplier_t supplier, uint32_t length);

bool fd_detour_source_get(fd_detour_source_t *source, uint8_t *data, uint32_t length);

void fd_detour_source_collection_initialize(fd_detour_source_collection_t *collection);

void fd_detour_source_collection_push(fd_detour_source_collection_t *collection, fd_detour_source_t *source);

void fd_detour_source_collection_pop(fd_detour_source_collection_t *collection);

#endif