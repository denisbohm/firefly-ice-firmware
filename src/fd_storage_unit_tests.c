#include "fd_w25q16dw.h"
#include "fd_log.h"
#include "fd_storage.h"

extern uint32_t fd_storage_start_page;
extern uint32_t fd_storage_end_page;
extern uint32_t fd_storage_first_page;
extern uint32_t fd_storage_free_page;

static
void verify_read_fails(void) {
    fd_storage_metadata_t metadata;
    uint8_t bytes[1];
    uint32_t length = 1;
    bool result = fd_storage_read_first_page(&metadata, bytes, length);
    fd_log_assert(result == false);
}

static
void verify_empty_state(void) {
    fd_storage_initialize();
    fd_log_assert(fd_storage_start_page == 1024);
    fd_log_assert(fd_storage_end_page == 8192);
    fd_log_assert(fd_storage_first_page == fd_storage_start_page);
    fd_log_assert(fd_storage_free_page == fd_storage_start_page);

    verify_read_fails();
}

void verify_single_page_add_remove(void) {
    // add
    uint8_t bytes[2] = {0x5a, 0x00};
    fd_storage_append_page(0x1234, bytes, 1);
    // get
    bytes[0] = 0x00;
    bytes[1] = 0xa5;
    fd_storage_metadata_t metadata;
    bool result = fd_storage_read_first_page(&metadata, bytes, 1);
    fd_log_assert(result == true);
    fd_log_assert(bytes[0] = 0x5a);
    fd_log_assert(bytes[1] = 0xa5);
    fd_log_assert(metadata.page == 1024);
    fd_log_assert(metadata.length == 1);
//    fd_log_assert(metadata.hash == ?);
    fd_log_assert(metadata.type = 0x1234);

    // verify the page is found on initialize
    fd_storage_initialize();
    result = fd_storage_read_first_page(&metadata, bytes, 1);
    fd_log_assert(result == true);
    fd_log_assert(bytes[0] = 0x5a);
    fd_log_assert(bytes[1] = 0xa5);
    fd_log_assert(metadata.page == 1024);
    fd_log_assert(metadata.length == 1);
//    fd_log_assert(metadata.hash == ?);
    fd_log_assert(metadata.type = 0x1234);

    // remove
    fd_storage_erase_page(&metadata);
    result = fd_storage_read_first_page(&metadata, bytes, 1);
    fd_log_assert(result == false);
}

void verify_two_page_wrap(void) {
    fd_storage_first_page = 8191;
    fd_storage_free_page = 8191;
    uint8_t bytes[2] = {0x5a, 0xa5};
    // add
    fd_storage_append_page(0x1234, bytes, 1);
    // add
    fd_storage_append_page(0x1234, bytes, 2);

    // verify the pages are found on initialize
    fd_storage_initialize();
    fd_log_assert(fd_storage_first_page == 8191);
    fd_log_assert(fd_storage_free_page == 1025);

    // get
    fd_storage_metadata_t metadata;
    bool result = fd_storage_read_first_page(&metadata, bytes, 1);
    fd_log_assert(result == true);
    fd_log_assert(bytes[0] = 0x5a);
    fd_log_assert(bytes[1] = 0xa5);
    fd_log_assert(metadata.page == 8191);
    fd_log_assert(metadata.length == 1);
    // remove
    fd_storage_erase_page(&metadata);

    // verify the page is found on initialize
    fd_storage_initialize();
    fd_log_assert(fd_storage_first_page == 1024);
    fd_log_assert(fd_storage_free_page == 1025);

    // get
    result = fd_storage_read_first_page(&metadata, bytes, 2);
    fd_log_assert(result == true);
    fd_log_assert(bytes[0] = 0x5a);
    fd_log_assert(bytes[1] = 0xa5);
    fd_log_assert(metadata.page == 1024);
    fd_log_assert(metadata.length == 2);
    // remove
    fd_storage_erase_page(&metadata);
    result = fd_storage_read_first_page(&metadata, bytes, 1);
    fd_log_assert(result == false);
}

void fd_storage_unit_tests(void) {
    fd_log_initialize();
    fd_w25q16dw_initialize();

    // erase external flash
    fd_w25q16dw_wake();
    fd_w25q16dw_enable_write();
    fd_w25q16dw_chip_erase();
    fd_w25q16dw_sleep();

    verify_empty_state();
    verify_single_page_add_remove();
    verify_two_page_wrap();
}