/** @file
    @brief Circular pages of storage.

    Each page has the following information header:
    1. 1-byte page marker.  0xff if page is unused, 0x01 if used, 0x81 if free
    2. 1-byte length.  The length of the data after the header.
    3. 2-byte hash.  A hash of the type and valid data in the page.
    4. 4-byte type.  The type of data stored in the page.
 */

#include "fd_w25q16dw.h"
#include "fd_binary.h"
#include "fd_crc.h"
#include "fd_log.h"
#include "fd_storage.h"

#include <string.h>

#define PAGE_UNUSED 0xff
#define PAGE_USED 0xfe
#define PAGE_FREE 0xfc

uint32_t fd_storage_start_page;
uint32_t fd_storage_end_page;
uint32_t fd_storage_first_page;
uint32_t fd_storage_free_page;

bool fd_storage_is_page_used(uint32_t page) {
    uint32_t address = page * FD_W25Q16DW_PAGE_SIZE;
    uint8_t marker;
    fd_w25q16dw_read(address, &marker, 1);
    return marker == PAGE_USED;
}

#define INVALID_PAGE 0xffffffff

void fd_storage_initialize(void) {
    // storage will use sectors 64-511 (sectors 0-63 are for firmware updates)
    fd_storage_start_page = 64 * FD_W25Q16DW_PAGES_PER_SECTOR;
    fd_storage_end_page = 512 * FD_W25Q16DW_PAGES_PER_SECTOR;

    fd_w25q16dw_wake();

    fd_storage_first_page = INVALID_PAGE;
    fd_storage_free_page = INVALID_PAGE;
    bool wraps = fd_storage_is_page_used(fd_storage_start_page) && fd_storage_is_page_used(fd_storage_end_page - 1);
    if (wraps) {
        for (uint32_t page = fd_storage_start_page; page < fd_storage_end_page; ++page) {
            if (!fd_storage_is_page_used(page)) {
                if (fd_storage_free_page == INVALID_PAGE) {
                    fd_storage_free_page = page;
                }
            } else {
                if ((fd_storage_first_page == INVALID_PAGE) && (fd_storage_free_page != INVALID_PAGE)) {
                    fd_storage_first_page = page;
                    break;
                }
            }
        }
    } else {
        for (uint32_t page = fd_storage_start_page; page < fd_storage_end_page; ++page) {
            if (fd_storage_is_page_used(page)) {
                if (fd_storage_first_page == INVALID_PAGE) {
                    fd_storage_first_page = page;
                }
            } else {
                if ((fd_storage_free_page == INVALID_PAGE) && (fd_storage_first_page != INVALID_PAGE)) {
                    fd_storage_free_page = page;
                    break;
                }
            }
        }
    }
    if (fd_storage_first_page == INVALID_PAGE) {
        fd_storage_first_page = fd_storage_start_page;
        fd_storage_free_page = fd_storage_first_page;
    }

    fd_w25q16dw_sleep();
}

uint32_t fd_storage_used_page_count(void) {
    if (fd_storage_first_page <= fd_storage_free_page) {
        return fd_storage_free_page - fd_storage_first_page;
    }
    return (fd_storage_end_page - fd_storage_first_page) + (fd_storage_free_page - fd_storage_first_page);
}

#define increment_page(page) if (++page >= fd_storage_end_page) page = fd_storage_start_page

void fd_storage_erase_first_page(void) {
    uint32_t address = fd_storage_first_page * FD_W25Q16DW_PAGE_SIZE;
    fd_w25q16dw_wake();
    fd_w25q16dw_enable_write();
    uint8_t marker = PAGE_FREE;
    fd_w25q16dw_write_page(address, &marker, sizeof(marker));
    fd_w25q16dw_sleep();

    increment_page(fd_storage_first_page);
}

void fd_storage_append_page(uint32_t type, uint8_t *data, uint32_t length) {
    if (length > FD_STORAGE_MAX_DATA_LENGTH) {
        fd_log_assert_fail("");
        length = FD_STORAGE_MAX_DATA_LENGTH;
    }

    fd_w25q16dw_wake();

    uint32_t address = fd_storage_free_page * FD_W25Q16DW_PAGE_SIZE;

    if ((fd_storage_free_page % FD_W25Q16DW_PAGES_PER_SECTOR) == 0) {
        // sector erase takes 50 ms typical, so only erase if there is data present -denis
        uint8_t marker;
        fd_w25q16dw_read(address, &marker, 1);
        if (marker != PAGE_UNUSED) {
            fd_w25q16dw_enable_write();
            fd_w25q16dw_erase_sector(address);
        }
        if ((fd_storage_free_page < fd_storage_first_page) && (fd_storage_first_page < fd_storage_free_page + FD_W25Q16DW_PAGES_PER_SECTOR)) {
            // need to move first page outside this erased sector
            fd_storage_first_page = ((fd_storage_first_page + FD_W25Q16DW_PAGES_PER_SECTOR) / FD_W25Q16DW_PAGES_PER_SECTOR) * FD_W25Q16DW_PAGES_PER_SECTOR;
            if (fd_storage_first_page >= fd_storage_end_page) {
                fd_storage_first_page = fd_storage_start_page;
            }
        }
    }

    uint8_t buffer[FD_W25Q16DW_PAGE_SIZE] = {PAGE_USED, length, 0, 0, type, type >> 8, type >> 16, type >> 24};
    memcpy(&buffer[8], data, length);
    uint16_t hash = fd_crc_16(0xffff, &buffer[4], 4 + length);
    buffer[2] = hash;
    buffer[3] = hash >> 8;
    fd_w25q16dw_enable_write();
    fd_w25q16dw_write_page(address, buffer, 8 + length);
    fd_w25q16dw_sleep();
    increment_page(fd_storage_free_page);
    if (fd_storage_free_page == fd_storage_first_page) {
        fd_storage_erase_first_page();
    }
}

bool fd_storage_read_first_page(fd_storage_metadata_t *metadata, uint8_t *data, uint32_t length) {
    if (fd_storage_first_page == fd_storage_free_page) {
        return false;
    }

    if (length > FD_STORAGE_MAX_DATA_LENGTH) {
        length = FD_STORAGE_MAX_DATA_LENGTH;
    }

    metadata->page = fd_storage_first_page;
    uint32_t address = fd_storage_first_page * FD_W25Q16DW_PAGE_SIZE;
    uint8_t buffer[FD_W25Q16DW_PAGE_SIZE];
    // !!! might be better to read length and then content -denis
    fd_w25q16dw_wake();
    fd_w25q16dw_read(address, buffer, FD_W25Q16DW_PAGE_SIZE);
    fd_w25q16dw_sleep();
    metadata->length = buffer[1];
    if (metadata->length > length) {
        metadata->length = length;
    }
    metadata->hash = fd_binary_unpack_uint16(&buffer[2]);
    metadata->type = fd_binary_unpack_uint32(&buffer[4]);
    memcpy(data, &buffer[8], metadata->length);
    return true;
}

void fd_storage_erase_page(fd_storage_metadata_t *metadata) {
    if (metadata->page != fd_storage_first_page) {
        // page has already been overwritten, so don't erase
        return;
    }

    fd_storage_erase_first_page();
}