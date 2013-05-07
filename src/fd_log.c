#include "fd_binary.h"
#include "fd_log.h"
#include "fd_rtc.h"
#include "fd_storage.h"

#include <string.h>

#define FD_LOG_STORAGE_TYPE FD_STORAGE_TYPE('F', 'D', 'L', 'O')

bool fd_log_did_log;

void fd_log_initialize(void) {
    fd_log_did_log = false;
}

void fd_log(char *message) {
    fd_log_did_log = true;

    uint8_t data[FD_STORAGE_MAX_DATA_LENGTH];
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, FD_STORAGE_MAX_DATA_LENGTH);
    fd_binary_put_time64(&binary, rtc_get_accurate_time());
    uint32_t length = strlen(message);
    uint32_t remaining_length = fd_binary_remaining_length(&binary);
    if (length > remaining_length) {
        length = remaining_length;
    }
    fd_binary_put_bytes(&binary, (uint8_t *)message, length);
    fd_storage_append_page(FD_LOG_STORAGE_TYPE, data, binary.put_index);
}

void fd_log_ram(char *message __attribute__((unused))) {
    fd_log_did_log = true;
}
