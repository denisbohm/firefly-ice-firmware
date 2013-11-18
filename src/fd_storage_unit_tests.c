#include "fd_w25q16dw.h"
#include "fd_log.h"
#include "fd_storage.h"

#include <string.h>

static
void erase_flash(void) {
    fd_w25q16dw_wake();
    fd_w25q16dw_enable_write();
    fd_w25q16dw_chip_erase();
    fd_w25q16dw_sleep();
}

static
void verify_read_fails(fd_storage_area_t *area) {
    fd_storage_metadata_t metadata;
    uint8_t bytes[1];
    uint32_t length = 1;
    bool result = fd_storage_area_read_first_page(area, &metadata, bytes, length);
    fd_log_assert(result == false);
}

static
void verify_empty_state() {
    fd_storage_area_t area;
    fd_storage_initialize();
    fd_storage_area_initialize(&area, 64, 511);
    fd_log_assert(area.start_page == 1024);
    fd_log_assert(area.end_page == 8192);
    fd_log_assert(area.first_page == area.start_page);
    fd_log_assert(area.free_page == area.start_page);

    verify_read_fails(&area);
}

static
void verify_single_page_add_remove(void) {
    fd_storage_area_t area;
    fd_storage_initialize();
    fd_storage_area_initialize(&area, 64, 511);
    // add
    uint8_t bytes[2] = {0x5a, 0x00};
    fd_storage_area_append_page(&area, 0x1234, bytes, 1);
    // get
    bytes[0] = 0x00;
    bytes[1] = 0xa5;
    fd_storage_metadata_t metadata;
    bool result = fd_storage_area_read_first_page(&area, &metadata, bytes, 1);
    fd_log_assert(result == true);
    fd_log_assert(bytes[0] == 0x5a);
    fd_log_assert(bytes[1] == 0xa5);
    fd_log_assert(metadata.page == 1024);
    fd_log_assert(metadata.length == 1);
//    fd_log_assert(metadata.hash == ?);
    fd_log_assert(metadata.type == 0x1234);

    // verify the page is found on initialize
    fd_storage_initialize();
    fd_storage_area_initialize(&area, 64, 511);
    result = fd_storage_area_read_first_page(&area, &metadata, bytes, 1);
    fd_log_assert(result == true);
    fd_log_assert(bytes[0] == 0x5a);
    fd_log_assert(bytes[1] == 0xa5);
    fd_log_assert(metadata.page == 1024);
    fd_log_assert(metadata.length == 1);
//    fd_log_assert(metadata.hash == ?);
    fd_log_assert(metadata.type == 0x1234);

    // remove
    fd_storage_erase_page(&metadata);
    result = fd_storage_area_read_first_page(&area, &metadata, bytes, 1);
    fd_log_assert(result == false);
}

static
void verify_two_page_wrap(void) {
    fd_storage_area_t area;
    fd_storage_initialize();
    fd_storage_area_initialize(&area, 64, 511);
    area.first_page = 8191;
    area.free_page = 8191;
    uint8_t bytes[2] = {0x5a, 0xa5};
    // add
    fd_storage_area_append_page(&area, 0x1234, bytes, 1);
    // add
    fd_storage_area_append_page(&area, 0x1234, bytes, 2);

    // verify the pages are found on initialize
    fd_storage_initialize();
    fd_storage_area_initialize(&area, 64, 511);
    fd_log_assert(area.first_page == 8191);
    fd_log_assert(area.free_page == 1025);

    // get
    fd_storage_metadata_t metadata;
    bool result = fd_storage_read_first_page(&metadata, bytes, 1);
    fd_log_assert(result == true);
    fd_log_assert(bytes[0] == 0x5a);
    fd_log_assert(bytes[1] == 0xa5);
    fd_log_assert(metadata.page == 8191);
    fd_log_assert(metadata.length == 1);
    // remove
    fd_storage_erase_page(&metadata);

    // verify the page is found on initialize
    fd_storage_initialize();
    fd_storage_area_initialize(&area, 64, 511);
    fd_log_assert(area.first_page == 1024);
    fd_log_assert(area.free_page == 1025);

    // get
    result = fd_storage_read_first_page(&metadata, bytes, 2);
    fd_log_assert(result == true);
    fd_log_assert(bytes[0] == 0x5a);
    fd_log_assert(bytes[1] == 0xa5);
    fd_log_assert(metadata.page == 1024);
    fd_log_assert(metadata.length == 2);
    // remove
    fd_storage_erase_page(&metadata);
    result = fd_storage_read_first_page(&metadata, bytes, 1);
    fd_log_assert(result == false);
}

static
void verify_add_sector_erase(void) {
    erase_flash();

    fd_storage_area_t area;
    fd_storage_initialize();
    fd_storage_area_initialize(&area, 64, 511);
    fd_log_assert(area.first_page == 1024);
    fd_log_assert(area.free_page == 1024);

    // put junk into first page of the sector (all zeros)
    fd_w25q16dw_wake();
    fd_w25q16dw_enable_write();
    uint8_t data[FD_W25Q16DW_PAGE_SIZE];
    memset(data, 0, sizeof(data));
    uint32_t address = area.first_page * FD_W25Q16DW_PAGE_SIZE;
    fd_w25q16dw_write_page(address, data, sizeof(data));
    fd_w25q16dw_sleep();

    uint8_t bytes[2] = {0x5a, 0xa5};
    fd_storage_area_append_page(&area, 0x1234, bytes, sizeof(bytes));
    memset(bytes, 0, sizeof(bytes));
    fd_storage_metadata_t metadata;
    bool result = fd_storage_read_first_page(&metadata, bytes, sizeof(bytes));
    fd_log_assert(result == true);
    fd_log_assert(bytes[0] == 0x5a);
    fd_log_assert(bytes[1] == 0xa5);
}

void fd_storage_unit_tests(void) {
    fd_log_initialize();
    fd_w25q16dw_initialize();

    verify_add_sector_erase();

    erase_flash();
    verify_empty_state();
    verify_single_page_add_remove();
    verify_two_page_wrap();
}