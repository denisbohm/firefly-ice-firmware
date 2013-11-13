#include "fd_log.h"
#include "fd_storage.h"
#include "fd_storage_buffer.h"

void fd_storage_buffer_unit_tests(void) {
    fd_log_assert(fd_storage_used_page_count() == 0);

    fd_storage_buffer_collection_initialize();
    fd_storage_buffer_t storage_buffer;
    fd_storage_buffer_initialize(&storage_buffer, 0x4321);
    fd_storage_buffer_collection_push(&storage_buffer);

    uint32_t time = 665193600; // Jan 30, 1991 00:00
    uint16_t interval = 10;
    fd_storage_buffer_add_time_series(&storage_buffer, time, interval, 1.0);
    time += interval;
    fd_storage_buffer_add_time_series(&storage_buffer, time, interval, 2.0);
    fd_log_assert(fd_storage_used_page_count() == 0);
    fd_storage_buffer_flush(&storage_buffer);
    fd_log_assert(fd_storage_used_page_count() == 1);
    time += interval;
    fd_storage_buffer_add_time_series(&storage_buffer, time, interval, 3.0);
    fd_log_assert(fd_storage_used_page_count() == 1);
    fd_storage_buffer_flush(&storage_buffer);
    fd_log_assert(fd_storage_used_page_count() == 2);

    fd_storage_metadata_t metadata;
    uint8_t bytes[64];
    bool result = fd_storage_buffer_get_first_page(&metadata, bytes, sizeof(bytes));
    fd_log_assert(result == false);
    time += interval;
    fd_storage_buffer_add_time_series(&storage_buffer, time, interval, 3.0);
    result = fd_storage_buffer_get_first_page(&metadata, bytes, sizeof(bytes));
    fd_log_assert(result == true);
    fd_storage_buffer_clear_page(&metadata);
    result = fd_storage_buffer_get_first_page(&metadata, bytes, sizeof(bytes));
    fd_log_assert(result == false);
}