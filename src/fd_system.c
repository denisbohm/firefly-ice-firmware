#include "fd_system.h"

#include <em_system.h>

// 16 byte hardware id: vendor, product, version (major, minor), unique id
void fd_get_hardware_id(fd_binary_t *binary) {
    fd_binary_put_uint16(binary, VENDOR);
    fd_binary_put_uint16(binary, PRODUCT);
    fd_binary_put_uint16(binary, HARDWARE_MAJOR);
    fd_binary_put_uint16(binary, HARDWARE_MINOR);
    fd_binary_put_uint64(binary, SYSTEM_GetUnique());
}
