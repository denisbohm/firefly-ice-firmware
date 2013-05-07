#include "fd_binary.h"
#include "fd_sensing.h"

#include <string.h>

#define SENSING_SIZE 25

static fd_detour_source_t fd_sensing_detour_source;
static uint8_t fd_sensing_buffer[SENSING_SIZE];

static
void fd_sensing_detour_supplier(uint32_t offset, uint8_t *data, uint32_t length) {
    memcpy(data, &fd_sensing_buffer[offset], length);
}

void fd_sensing_initialize(void) {
    fd_detour_source_initialize(&fd_sensing_detour_source);
}

void fd_sensing_push(
    fd_detour_source_collection_t *detour_source_collection,
    float ax, float ay, float az,
    float mx, float my, float mz
) {
    if (fd_detour_source_is_transferring(&fd_sensing_detour_source)) {
        return;
    }

    fd_binary_t binary;
    fd_binary_initialize(&binary, fd_sensing_buffer, SENSING_SIZE);
    fd_binary_put_uint8(&binary, 0xff);
    fd_binary_put_float32(&binary, ax);
    fd_binary_put_float32(&binary, ay);
    fd_binary_put_float32(&binary, az);
    fd_binary_put_float32(&binary, mx);
    fd_binary_put_float32(&binary, my);
    fd_binary_put_float32(&binary, mz);

    fd_detour_source_set(&fd_sensing_detour_source, fd_sensing_detour_supplier, SENSING_SIZE);
    fd_detour_source_collection_push(detour_source_collection, &fd_sensing_detour_source);
}
