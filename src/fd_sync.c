#include "fd_binary.h"
#include "fd_control_codes.h"
#include "fd_hal_processor.h"
#include "fd_hal_system.h"
#include "fd_log.h"
#include "fd_storage.h"
#include "fd_storage_buffer.h"
#include "fd_sync.h"

#include <string.h>

#define COMMAND_SIZE 1
#define METADATA_SIZE 12
#define SYNC_SIZE (COMMAND_SIZE + HARDWARE_ID_SIZE + METADATA_SIZE + FD_STORAGE_MAX_DATA_LENGTH)

uint8_t fd_sync_output_buffer[SYNC_SIZE];

void fd_sync_initialize(void) {
}

void fd_sync_start(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    uint32_t flags = 0;
    uint32_t offset = 0;
    if (length >= 4) {
        fd_binary_t binary;
        fd_binary_initialize(&binary, data, length);
        flags = fd_binary_get_uint32(&binary);
        if (flags & FD_CONTROL_SYNC_AHEAD) {
            offset = fd_binary_get_uint32(&binary);
        }
    }

    fd_storage_metadata_t metadata;
    uint32_t shortage = fd_storage_read_nth_page(offset, &metadata, &fd_sync_output_buffer[COMMAND_SIZE + HARDWARE_ID_SIZE + METADATA_SIZE], FD_STORAGE_MAX_DATA_LENGTH);
    if (shortage > 0) {
        bool has_page = fd_storage_buffer_get_first_page(shortage - 1, &metadata, &fd_sync_output_buffer[COMMAND_SIZE + HARDWARE_ID_SIZE + METADATA_SIZE], FD_STORAGE_MAX_DATA_LENGTH);
        if (has_page) {
            --shortage;
        } else {
            // send indication that there is nothing to sync -denis
            metadata.page = 0xfffffffe;
            metadata.length = 0;
            metadata.hash = 0;
            metadata.type = 0;
        }
    }

    if (metadata.length > FD_STORAGE_MAX_DATA_LENGTH) {
        fd_log_assert_fail("");
        metadata.length = FD_STORAGE_MAX_DATA_LENGTH;
    }

    fd_binary_t binary;
    fd_binary_initialize(&binary, fd_sync_output_buffer, COMMAND_SIZE + HARDWARE_ID_SIZE + METADATA_SIZE);
    fd_binary_put_uint8(&binary, FD_CONTROL_SYNC_DATA);
    fd_hal_processor_get_hardware_id(&binary);
    fd_binary_put_uint32(&binary, metadata.page);
    fd_binary_put_uint16(&binary, metadata.length);
    fd_binary_put_uint16(&binary, metadata.hash);
    fd_binary_put_uint32(&binary, metadata.type);

    uint32_t sync_length = COMMAND_SIZE + HARDWARE_ID_SIZE + METADATA_SIZE + metadata.length;
    // encrypt

    bool result = packet_output->write(fd_sync_output_buffer, sync_length);
    if (!result) {
        fd_log_assert_fail("");
    }
}

void fd_sync_ack(fd_packet_output_t *packet_output __attribute__((unused)), uint8_t *data, uint32_t length) {
    // decrypt

    fd_storage_metadata_t metadata;
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    metadata.page = fd_binary_get_uint32(&binary);
    metadata.length = fd_binary_get_uint16(&binary);
    metadata.hash = fd_binary_get_uint16(&binary);
    metadata.type = fd_binary_get_uint32(&binary);
    if (metadata.page == 0xfffffffe) {
        // !!! shouldn't get ack for empty sync... -denis
    } else
    if (metadata.page == 0xffffffff) {
        fd_storage_buffer_clear_page(&metadata);
    } else {
        fd_storage_erase_page(&metadata);
    }
}
