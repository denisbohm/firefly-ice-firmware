#include "fd_binary.h"
#include "fd_log.h"

void fd_binary_unit_tests(void) {
    fd_binary_t binary;
    uint8_t bytes[64];
    fd_binary_initialize(&binary, bytes, sizeof(bytes));
    float f = 1.5;
    fd_binary_put_float16(&binary, f);
    float v = fd_binary_get_float16(&binary);
    fd_log_assert(f == v);
}