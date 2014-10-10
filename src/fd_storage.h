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

typedef struct fd_storage_area_t {
    struct fd_storage_area_t *next;
    struct fd_storage_area_t *previous;

    uint32_t start_page;
    uint32_t end_page;
    uint32_t first_page;
    uint32_t free_page;
} fd_storage_area_t;

void fd_storage_initialize(void);
uint32_t fd_storage_used_page_count(void);
bool fd_storage_read_first_page(fd_storage_metadata_t *metadata, uint8_t *data, uint32_t length);
uint32_t fd_storage_read_nth_page(uint32_t n, fd_storage_metadata_t *metadata, uint8_t *data, uint32_t length);
void fd_storage_erase_page(fd_storage_metadata_t *metadata);

void fd_storage_area_initialize(fd_storage_area_t *area, uint32_t start_sector, uint32_t end_sector);
uint32_t fd_storage_area_used_page_count(fd_storage_area_t *area);
void fd_storage_area_append_page(fd_storage_area_t *area, uint32_t type, uint8_t *data, uint32_t length);
bool fd_storage_area_read_first_page(fd_storage_area_t *area, fd_storage_metadata_t *metadata, uint8_t *data, uint32_t length);
void fd_storage_area_read_nth_page(fd_storage_area_t *area, uint32_t n, fd_storage_metadata_t *metadata, uint8_t *data, uint32_t length);
void fd_storage_area_erase_page(fd_storage_area_t *area, fd_storage_metadata_t *metadata);

void fd_storage_area_free_all_pages(fd_storage_area_t *area);

#endif