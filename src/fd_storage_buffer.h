#ifndef FD_STORAGE_BUFFER_H
#define FD_STORAGE_BUFFER_H

#include "fd_storage.h"
#include "fd_time.h"

#include <stdint.h>

typedef struct fd_storage_buffer {
    struct fd_storage_buffer *next;
    struct fd_storage_buffer *previous;

    uint32_t type;
    uint8_t data[FD_STORAGE_MAX_DATA_LENGTH];
    uint32_t index;
} fd_storage_buffer_t;

void fd_storage_buffer_initialize(fd_storage_buffer_t *storage_buffer, uint32_t type);

void fd_storage_buffer_flush(fd_storage_buffer_t *storage_buffer);

void fd_storage_buffer_add(fd_storage_buffer_t *storage_buffer, uint8_t *data, uint32_t length);

void fd_storage_buffer_add_time_series(
    fd_storage_buffer_t *storage_buffer, uint32_t time, uint16_t interval, float value
);

void fd_storage_buffer_collection_initialize(void);

void fd_storage_buffer_collection_push(fd_storage_buffer_t *storage_buffer);

bool fd_storage_buffer_get_first_page(fd_storage_metadata_t *metadata, uint8_t *data, uint32_t length);

void fd_storage_buffer_clear_page(fd_storage_metadata_t *metadata);

#endif