#include "fd_binary.h"
#include "fd_control.h"
#include "fd_storage.h"
#include "fd_sync.h"
#include "fd_system.h"

#include <string.h>

#define COMMAND_SIZE 1
#define METADATA_SIZE 12
#define SYNC_SIZE (COMMAND_SIZE + HARDWARE_ID_SIZE + METADATA_SIZE + FD_STORAGE_MAX_DATA_LENGTH)

fd_detour_source_t fd_sync_detour_source;
uint8_t fd_detour_buffer[SYNC_SIZE];

void fd_sync_initialize(void) {
    fd_detour_source_initialize(&fd_sync_detour_source);
}

static
void fd_sync_detour_supplier(uint32_t offset, uint8_t *data, uint32_t length) {
    memcpy(data, &fd_detour_buffer[offset], length);
}

void fd_sync_start(fd_detour_source_collection_t *detour_source_collection, uint8_t *data __attribute__((unused)), uint32_t length __attribute__((unused))) {
    fd_storage_metadata_t metadata;
    bool has_page = fd_storage_read_first_page(&metadata, &fd_detour_buffer[COMMAND_SIZE + HARDWARE_ID_SIZE + METADATA_SIZE], FD_STORAGE_MAX_DATA_LENGTH);
    if (!has_page) {
        return;
    }

    fd_binary_t binary;
    fd_binary_initialize(&binary, fd_detour_buffer, COMMAND_SIZE + HARDWARE_ID_SIZE + METADATA_SIZE);
    fd_binary_put_uint8(&binary, FD_SYNC_DATA);
    fd_get_hardware_id(&binary);
    fd_binary_put_uint32(&binary, metadata.page);
    fd_binary_put_uint16(&binary, metadata.length);
    fd_binary_put_uint16(&binary, metadata.hash);
    fd_binary_put_uint32(&binary, metadata.type);

    uint32_t sync_length = COMMAND_SIZE + HARDWARE_ID_SIZE + METADATA_SIZE + metadata.length;
    // encrypt

    fd_detour_source_set(&fd_sync_detour_source, fd_sync_detour_supplier, sync_length);
    fd_detour_source_collection_push(detour_source_collection, &fd_sync_detour_source);
}

void fd_sync_ack(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    // decrypt

    fd_storage_metadata_t metadata;
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    metadata.page = fd_binary_get_uint32(&binary);
    metadata.length = fd_binary_get_uint16(&binary);
    metadata.hash = fd_binary_get_uint16(&binary);
    metadata.type = fd_binary_get_uint32(&binary);
    fd_storage_erase_page(&metadata);
}