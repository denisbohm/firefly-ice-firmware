#include "fd_binary.h"
#include "fd_log.h"
#include "fd_rtc.h"
#include "fd_storage.h"

#include <string.h>
#include <stdio.h>

#define FD_LOG_STORAGE_TYPE FD_STORAGE_TYPE('F', 'D', 'L', 'O')

bool fd_log_did_log;
bool fd_log_use_storage;

void fd_log_initialize(void) {
    fd_log_did_log = false;
    fd_log_use_storage = false;
}

void fd_log_enable_storage(bool enable) {
    fd_log_use_storage = enable;
}

void fd_log_ram(char *message __attribute__((unused))) {
    fd_log_did_log = true;
}

void fd_log_at(char *file, int line, char *message) {
    if (!fd_log_use_storage) {
        fd_log_ram(message);
        return;
    }

    fd_log_did_log = true;

    uint8_t data[FD_STORAGE_MAX_DATA_LENGTH];
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, FD_STORAGE_MAX_DATA_LENGTH);
    fd_binary_put_time64(&binary, fd_rtc_get_accurate_time());
    if (file != 0) {
        fd_binary_put_bytes(&binary, (uint8_t *)file, strlen(file));
        fd_binary_put_uint8(&binary, ' ');
        char buffer[6];
        sprintf(buffer, "%d", line);
        fd_binary_put_bytes(&binary, (uint8_t *)buffer, strlen(buffer));
        fd_binary_put_uint8(&binary, ' ');
    }
    uint32_t length = strlen(message);
    uint32_t remaining_length = fd_binary_remaining_length(&binary);
    if (length > remaining_length) {
        length = remaining_length;
    }
    fd_binary_put_bytes(&binary, (uint8_t *)message, length);
    fd_storage_append_page(FD_LOG_STORAGE_TYPE, data, binary.put_index);
}

void fd_log(char *message) {
    fd_log_at(0, 0, message);
}

