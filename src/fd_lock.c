#include "fd_event.h"
#include "fd_lock.h"

#include <stdint.h>

#define FD_LOCK_COUNT 8

fd_lock_owner_t fd_lock_owners[FD_LOCK_COUNT];

void fd_lock_initialize(void) {
    for (uint32_t i = 0; i < FD_LOCK_COUNT; ++i) {
        fd_lock_owners[i] = fd_lock_owner_none;
    }
}

fd_lock_owner_t fd_lock_owner(fd_lock_identifier_t identifier) {
    return fd_lock_owners[identifier];
}

void fd_lock_close(fd_lock_owner_t owner) {
    for (uint32_t i = 0; i < FD_LOCK_COUNT; ++i) {
        if (fd_lock_owners[i] == owner) {
            fd_lock_owners[i] = fd_lock_owner_none;
        }
    }
}

fd_lock_owner_t fd_lock(fd_lock_identifier_t identifier, fd_lock_operation_t operation, fd_lock_owner_t owner) {
    fd_lock_owner_t lock_owner = fd_lock_owner_none;
    if (identifier < FD_LOCK_COUNT) {
        fd_lock_owner_t current_lock_owner = fd_lock_owners[identifier];
        if (operation == fd_lock_operation_acquire) {
            if ((current_lock_owner == fd_lock_owner_none) || (current_lock_owner == owner)) {
                fd_lock_owners[identifier] = owner;
            }
        } else
        if (operation == fd_lock_operation_release) {
            if (current_lock_owner == owner) {
                fd_lock_owners[identifier] = fd_lock_owner_none;
            }
        }
        lock_owner = fd_lock_owners[identifier];
    }
    fd_event_set(FD_EVENT_LOCK_STATE);
    return lock_owner;
}