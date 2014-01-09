/** @file
    @brief Circular pages of storage.

    Each page has the following information header:
    1. 1-byte page marker.  0xff if page is unused, 0xfe if used, 0xfc if free
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

#define INVALID_PAGE 0xffffffff

typedef struct {
    fd_storage_area_t *first;
    fd_storage_area_t *last;
} fd_storage_area_collection_t;

static fd_storage_area_collection_t storage_area_collection;

void fd_storage_initialize(void) {
    storage_area_collection.first = 0;
    storage_area_collection.last = 0;
}

void fd_storage_area_collection_push(fd_storage_area_t *storage_area) {
    if (storage_area_collection.first == 0) {
        storage_area_collection.first = storage_area;
        storage_area_collection.last = storage_area;
        storage_area->next = 0;
        storage_area->previous = 0;
    } else {
        fd_storage_area_t *old_last = storage_area_collection.last;
        storage_area_collection.last = storage_area;
        storage_area->previous = old_last;
        old_last->next = storage_area;
    }
}

bool fd_storage_is_page_used(uint32_t page) {
    uint32_t address = page * FD_W25Q16DW_PAGE_SIZE;
    uint8_t marker;
    fd_w25q16dw_read(address, &marker, 1);
    return marker == PAGE_USED;
}

void fd_storage_area_initialize(fd_storage_area_t *area, uint32_t start_sector, uint32_t end_sector) {
    fd_storage_area_collection_push(area);

    area->start_page = start_sector * FD_W25Q16DW_PAGES_PER_SECTOR;
    area->end_page = (end_sector + 1) * FD_W25Q16DW_PAGES_PER_SECTOR;

    fd_w25q16dw_wake();

    area->first_page = INVALID_PAGE;
    area->free_page = INVALID_PAGE;
    bool wraps = fd_storage_is_page_used(area->start_page) && fd_storage_is_page_used(area->end_page - 1);
    if (wraps) {
        for (uint32_t page = area->start_page; page < area->end_page; ++page) {
            if (!fd_storage_is_page_used(page)) {
                if (area->free_page == INVALID_PAGE) {
                    area->free_page = page;
                }
            } else {
                if ((area->first_page == INVALID_PAGE) && (area->free_page != INVALID_PAGE)) {
                    area->first_page = page;
                    break;
                }
            }
        }
    } else {
        for (uint32_t page = area->start_page; page < area->end_page; ++page) {
            if (fd_storage_is_page_used(page)) {
                if (area->first_page == INVALID_PAGE) {
                    area->first_page = page;
                }
            } else {
                if ((area->free_page == INVALID_PAGE) && (area->first_page != INVALID_PAGE)) {
                    area->free_page = page;
                    break;
                }
            }
        }
    }
    if (area->first_page == INVALID_PAGE) {
        area->first_page = area->start_page;
        area->free_page = area->first_page;
    }

    fd_w25q16dw_sleep();
}

uint32_t fd_storage_area_used_page_count(fd_storage_area_t *area) {
    if (area->first_page <= area->free_page) {
        return area->free_page - area->first_page;
    }
    return (area->end_page - area->first_page) + (area->free_page - area->start_page);
}

#define increment_page(page) if (++page >= area->end_page) page = area->start_page

void fd_storage_free_first_page(fd_storage_area_t *area) {
    uint32_t address = area->first_page * FD_W25Q16DW_PAGE_SIZE;
    fd_w25q16dw_wake();
    fd_w25q16dw_enable_write();
    uint8_t marker = PAGE_FREE;
    fd_w25q16dw_write_page(address, &marker, sizeof(marker));
    fd_w25q16dw_sleep();

    increment_page(area->first_page);
}

void fd_storage_area_append_page(fd_storage_area_t *area, uint32_t type, uint8_t *data, uint32_t length) {
    if (length > FD_STORAGE_MAX_DATA_LENGTH) {
        length = FD_STORAGE_MAX_DATA_LENGTH;
    }

    fd_w25q16dw_wake();

    uint32_t address = area->free_page * FD_W25Q16DW_PAGE_SIZE;

    if ((area->free_page % FD_W25Q16DW_PAGES_PER_SECTOR) == 0) {
        // sector erase takes 50 ms typical, so only erase if there is data present -denis
        uint8_t marker;
        fd_w25q16dw_read(address, &marker, 1);
        if (marker != PAGE_UNUSED) {
            fd_w25q16dw_enable_write();
            fd_w25q16dw_erase_sector(address);
        }
        if ((area->free_page < area->first_page) && (area->first_page < area->free_page + FD_W25Q16DW_PAGES_PER_SECTOR)) {
            // need to move first page outside this erased sector
            area->first_page = ((area->first_page + FD_W25Q16DW_PAGES_PER_SECTOR) / FD_W25Q16DW_PAGES_PER_SECTOR) * FD_W25Q16DW_PAGES_PER_SECTOR;
            if (area->first_page >= area->end_page) {
                area->first_page = area->start_page;
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
    increment_page(area->free_page);
    if (area->free_page == area->first_page) {
        fd_storage_free_first_page(area);
    }
}

void fd_storage_area_read_nth_page(fd_storage_area_t *area, uint32_t n, fd_storage_metadata_t *metadata, uint8_t *data, uint32_t length) {
    if (length > FD_STORAGE_MAX_DATA_LENGTH) {
        length = FD_STORAGE_MAX_DATA_LENGTH;
    }

    metadata->page = area->first_page + n;
    if (metadata->page >= area->end_page) {
        metadata->page = area->start_page + (metadata->page - area->end_page);
    }
    uint32_t address = area->first_page * FD_W25Q16DW_PAGE_SIZE;
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
}

bool fd_storage_area_read_first_page(fd_storage_area_t *area, fd_storage_metadata_t *metadata, uint8_t *data, uint32_t length) {
    if (area->first_page == area->free_page) {
        return false;
    }

    if (length > FD_STORAGE_MAX_DATA_LENGTH) {
        length = FD_STORAGE_MAX_DATA_LENGTH;
    }

    metadata->page = area->first_page;
    uint32_t address = area->first_page * FD_W25Q16DW_PAGE_SIZE;
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

void fd_storage_area_erase_page(fd_storage_area_t *area, fd_storage_metadata_t *metadata) {
    if (metadata->page != area->first_page) {
        // page has already been overwritten, so don't free it
        return;
    }

    fd_storage_free_first_page(area);
}

uint32_t fd_storage_used_page_count(void) {
    uint32_t count = 0;
    fd_storage_area_t *area = storage_area_collection.first;
    while (area != 0) {
        count += fd_storage_area_used_page_count(area);
        area = area->next;
    }
    return count;
}

uint32_t fd_storage_read_nth_page(uint32_t offset, fd_storage_metadata_t *metadata, uint8_t *data, uint32_t length) {
    uint32_t n = offset;
    fd_storage_area_t *area = storage_area_collection.first;
    while (area != 0) {
        uint32_t count = fd_storage_area_used_page_count(area);
        if (n < count) {
            fd_storage_area_read_nth_page(area, n, metadata, data, length);
            return 0;
        }
        n -= count;
        area = area->next;
    }
    uint32_t shortage = n + 1;
    return shortage;
}

bool fd_storage_read_first_page(fd_storage_metadata_t *metadata, uint8_t *data, uint32_t length) {
    fd_storage_area_t *area = storage_area_collection.first;
    while (area != 0) {
        bool result = fd_storage_area_read_first_page(area, metadata, data, length);
        if (result) {
            return true;
        }
        area = area->next;
    }
    return false;
}

void fd_storage_erase_page(fd_storage_metadata_t *metadata) {
    fd_storage_area_t *area = storage_area_collection.first;
    while (area != 0) {
        if ((area->start_page <= metadata->page) && (metadata->page < area->end_page)) {
            fd_storage_area_erase_page(area, metadata);
            return;
        }
        area = area->next;
    }
}
