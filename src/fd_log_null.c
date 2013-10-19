#include "fd_log.h"

#include <stdint.h>

bool fd_log_did_log;
uint32_t fd_log_count;
char fd_log_message[128];

void fd_log_initialize(void) {
    fd_log_did_log = false;
    fd_log_count = 0;
}

void fd_log_enable_storage(bool enable __attribute__((unused))) {
}

void fd_log_at(char *file, int line, char *message) {
    if (!fd_log_did_log) {
        sprintf(fd_log_message, "%s %d: %s", file, line, message);
    }
    fd_log_did_log = true;
    ++fd_log_count;
}

void fd_log(char *message) {
    fd_log_at("fd_log", 0, message);
}

uint32_t fd_log_get_count(void) {
    return fd_log_count;
}

char *fd_log_get_message(void) {
    return fd_log_message;
}