/** @file
    @brief Circular pages of storage.

    Each page has the following information header:
    1. 1-byte page marker.  0xff if page is unused, 0x00 if used.
    2. 1-byte length.  The length of the data after the header.
    3. 2-byte hash.  A hash of the type and valid data in the page.
    4. 4-byte type.  The type of data stored in the page.
 */

#include "fd_at24c512c.h"
#include "fd_binary.h"
#include "fd_crc.h"
#include "fd_storage.h"

#include <string.h>

static uint32_t first_page;
static uint32_t free_page;

void fd_storage_initialize(void) {
    first_page = 0;
    free_page = 0;
}

uint32_t fd_storage_used_page_count(void) {
    if (first_page <= free_page) {
        return free_page - first_page;
    }
    return first_page - free_page;
}

#define increment_page(page) if (++page >= FD_AT24C512C_PAGES) page = 0

void fd_storage_append_page(uint32_t type, uint8_t *data, uint32_t length) {
    uint8_t buffer[128] = {0x00, length, 0, 0, type, type >> 8, type >> 16, type >> 24};
    memcpy(&buffer[4], data, 4 + length);
    uint16_t hash = fd_crc_16(0xffff, &buffer[4], 4 + length);
    buffer[2] = hash;
    buffer[3] = hash >> 8;
    uint32_t address = free_page * FD_AT24C512C_PAGE_SIZE;
    fd_at24c512c_write_page(address, buffer, 8 + length);
    increment_page(free_page);
    if (free_page == first_page) {
        increment_page(first_page);
    }
}

bool fd_storage_read_first_page(fd_storage_metadata_t *metadata, uint8_t *data, uint32_t length) {
    if (first_page == free_page) {
        return false;
    }
    metadata->page = first_page;
    uint32_t address = first_page * FD_AT24C512C_PAGE_SIZE;
    uint8_t buffer[128];
    fd_at24c512c_read_page(address, buffer, 128);
    metadata->length = buffer[1];
    metadata->hash = fd_binary_unpack_uint16(&buffer[2]);
    metadata->type = fd_binary_unpack_uint32(&buffer[4]);
    uint32_t len = metadata->length;
    if (length < len) {
        len = length;
    }
    memcpy(data, &buffer[8], len);
    return true;
}

void fd_storage_erase_page(fd_storage_metadata_t *metadata) {
    if (metadata->page != first_page) {
        // page has already been overwritten, so don't erase
        return;
    }

    uint32_t address = first_page * FD_AT24C512C_PAGE_SIZE;
    fd_at24c512c_erase_page(address);
    increment_page(first_page);
}