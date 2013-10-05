#ifndef FD_STORAGE_H
#define FD_STORAGE_H

#include <stdbool.h>
#include <stdint.h>

#define FD_STORAGE_MAX_DATA_LENGTH 248

#define FD_STORAGE_TYPE(a, b, c, d) (a | (b << 8) | (c << 16) | (d << 24))

typedef struct {
    uint32_t page;
    uint16_t length;
    uint16_t hash;
    uint32_t type;
} fd_storage_metadata_t;

void fd_storage_initialize(void);

uint32_t fd_storage_used_page_count(void);

void fd_storage_append_page(uint32_t type, uint8_t *data, uint32_t length);
bool fd_storage_read_first_page(fd_storage_metadata_t *metadata, uint8_t *data, uint32_t length);
void fd_storage_erase_page(fd_storage_metadata_t *metadata);

#endif