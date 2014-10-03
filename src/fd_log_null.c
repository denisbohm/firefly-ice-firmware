#include "fd_log.h"

#include <stdint.h>
#include <stdio.h>

bool fd_log_did_log;
uint32_t fd_log_count;
char fd_log_message[128];

void fd_log_initialize(void) {
    fd_log_did_log = false;
    fd_log_count = 0;
}

void fd_log_set_count(uint32_t count) {
    fd_log_count = count;
}

uint32_t fd_log_get_count(void) {
    return fd_log_count;
}

bool fd_log_get_storage(void) {
    return false;
}

void fd_log_set_storage(bool enable __attribute__((unused))) {
}

void fd_log_at(char *file, int line, char *message) {
    if (!fd_log_did_log) {
        char *name = strrchr(file, '/');
        if (name == 0) {
            name = file;
        } else {
            ++name;
        }
        sprintf(fd_log_message, "%s %d: %s", name, line, message);
    }
    fd_log_did_log = true;
    ++fd_log_count;
}

void fd_log(char *message) {
    fd_log_at("fd_log", 0, message);
}

char *fd_log_get_message(void) {
    return fd_log_message;
}
