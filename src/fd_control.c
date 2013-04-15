#include "fd_control.h"
#include "fd_sync.h"

typedef void (*fd_control_command_t)(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length);

void fd_control_initialize(void) {
}

void fd_control_process(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length) {
    if (length < 1) {
        return;
    }
    uint8_t code = data[0];
    fd_control_command_t command = 0;
    switch (code) {
        case FD_SYNC_START:
            command = fd_sync_start;
            break;
        case FD_SYNC_ACK:
            command = fd_sync_ack;
            break;
    }
    if (command) {
        (*command)(detour_source_collection, &data[1], length - 1);
    }
}