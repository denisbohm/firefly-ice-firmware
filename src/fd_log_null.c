#include "fd_log.h"

#include <stdint.h>

bool fd_log_did_log;
uint32_t fd_log_count;

void fd_log_initialize(void) {
    fd_log_did_log = false;
    fd_log_count = 0;
}

void fd_log_enable_storage(bool enable __attribute__((unused))) {
}

void fd_log(char *message __attribute__((unused))) {
    fd_log_did_log = true;
    ++fd_log_count;
}

uint32_t fd_log_get_count(void) {
    return fd_log_count;
}