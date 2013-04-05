#include "fd_log.h"
#include "fd_storage.h"

#include <string.h>

#define FD_LOG_STORAGE_TYPE FD_STORAGE_TYPE('F', 'D', 'L', 'O')

void fd_log(char *message) {
    uint32_t length = strlen(message);
    if (length > FD_STORAGE_MAX_DATA_LENGTH) {
        length = FD_STORAGE_MAX_DATA_LENGTH;
    }
    fd_storage_append_page(FD_LOG_STORAGE_TYPE, message, length);
}

void fd_log_ram(char *message) {
}
