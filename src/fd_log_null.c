#include "fd_log.h"

bool fd_log_did_log;

void fd_log_initialize(void) {
    fd_log_did_log = false;
}

void fd_log_enable_storage(bool enable __attribute__((unused))) {
}

void fd_log(char *message __attribute__((unused))) {
}