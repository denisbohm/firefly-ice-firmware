#include "fd_binary.h"
#include "fd_detour.h"
#include "fd_log.h"
#include "fd_storage.h"
#include "fd_storage_buffer.h"
#include "fd_sync.h"
#include "fd_system.h"
#include "fd_w25q16dw.h"

static
fd_storage_metadata_t get_metadata(uint8_t *bytes, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, bytes, length);
    uint8_t packet_sequence_number = fd_binary_get_uint8(&binary);
    uint8_t packet_length = fd_binary_get_uint16(&binary);
    uint8_t packet_command = fd_binary_get_uint8(&binary);
    binary.get_index += HARDWARE_ID_SIZE;
    fd_storage_metadata_t metadata;
    metadata.page = fd_binary_get_uint32(&binary);
    metadata.length = fd_binary_get_uint16(&binary);
    metadata.hash = fd_binary_get_uint16(&binary);
    metadata.type = fd_binary_get_uint32(&binary);
    return metadata;
}

void fd_sync_unit_tests(void) {
    fd_storage_initialize();
    fd_storage_area_t area;
    fd_storage_area_initialize(&area, 0, 1);
    fd_log_assert(fd_storage_used_page_count() == 0);

    fd_sync_initialize();

    fd_detour_t detour;
    uint8_t detour_data[128];
    fd_detour_initialize(&detour, detour_data, sizeof(detour_data));

    fd_detour_source_t source;
    fd_detour_source_initialize(&source);

    fd_detour_source_collection_t collection;
    uint8_t collection_bytes[128];
    fd_detour_source_collection_initialize(&collection, fd_lock_owner_usb, 64, collection_bytes, sizeof(collection_bytes));

    fd_sync_start(&collection, (uint8_t *)0, 0);
    fd_log_assert(collection.bufferCount == 64);
    fd_storage_metadata_t metadata = get_metadata(collection_bytes, sizeof(collection_bytes));
    fd_log_assert(metadata.page == 0xfffffffe);

    // reset detour...
    fd_detour_clear(&detour);
    fd_detour_source_initialize(&source);
    fd_detour_source_collection_initialize(&collection, fd_lock_owner_usb, 64, collection_bytes, sizeof(collection_bytes));

    uint8_t bytes[2] = {0x5a, 0x00};
    fd_storage_area_append_page(&area, 0x1234, bytes, 1);

    fd_sync_start(&collection, (uint8_t *)0, 0);
    fd_log_assert(collection.bufferCount == 64);
    metadata = get_metadata(collection_bytes, sizeof(collection_bytes));
    fd_log_assert(metadata.page == 0);

    fd_detour_source_collection_initialize(&collection, fd_lock_owner_usb, 64, collection_bytes, sizeof(collection_bytes));
    uint8_t data[64];
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, sizeof(data));
    fd_binary_put_uint32(&binary, metadata.page);
    fd_binary_put_uint16(&binary, metadata.length);
    fd_binary_put_uint16(&binary, metadata.hash);
    fd_binary_put_uint32(&binary, metadata.type);
    fd_sync_ack(&collection, data, sizeof(data));
    fd_log_assert(fd_storage_used_page_count() == 0);

    fd_detour_source_collection_initialize(&collection, fd_lock_owner_usb, 64, collection_bytes, sizeof(collection_bytes));
    fd_sync_start(&collection, (uint8_t *)0, 0);
    fd_log_assert(collection.bufferCount == 64);
    metadata = get_metadata(collection_bytes, sizeof(collection_bytes));
    fd_log_assert(metadata.page == 0xfffffffe);
}