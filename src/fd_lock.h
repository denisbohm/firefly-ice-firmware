#ifndef FD_LOCK_H
#define FD_LOCK_H

#include <stdint.h>

#define FD_LOCK_OWNER_ENCODE(a, b, c, d) ((a << 24) | (b << 16) | (c << 8) | d)

void fd_lock_initialize(void);

enum {
    fd_lock_owner_none = 0,
    fd_lock_owner_ble = FD_LOCK_OWNER_ENCODE('B', 'L', 'E', ' '),
    fd_lock_owner_usb = FD_LOCK_OWNER_ENCODE('U', 'S', 'B', ' '),
};
typedef uint32_t fd_lock_owner_t;

enum {
    fd_lock_operation_none,
    fd_lock_operation_acquire,
    fd_lock_operation_release,
};
typedef uint8_t fd_lock_operation_t;

enum {
    fd_lock_identifier_sync,
};
typedef uint8_t fd_lock_identifier_t;

fd_lock_owner_t fd_lock(fd_lock_identifier_t identifier, fd_lock_operation_t operation, fd_lock_owner_t owner);

void fd_lock_close(fd_lock_owner_t owner);

#endif