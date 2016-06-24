#include "fdi_detour.h"

#include <string.h>

void fdi_detour_source_initialize(fd_detour_t *source, uint8_t *data, uint32_t size) {
    source->data = data;
    source->size = size;

    source->state = fd_detour_state_clear;
    source->length = 0;
    source->sequence_number = 0;
    source->offset = 0;
}

bool fdi_detour_source_is_transferring(fd_detour_t *source) {
    return (source->state == fd_detour_state_intermediate) && (source->sequence_number != 0);
}

void fdi_detour_source_set(fd_detour_t *source, uint32_t length) {
    source->state = fd_detour_state_intermediate;
    source->length = length;
    source->sequence_number = 0;
    source->offset = 0;
}

bool fdi_detour_source_get(fd_detour_t *source, uint8_t *data, uint32_t length) {
    if (source->state != fd_detour_state_intermediate) {
        return false;
    }
    data[0] = source->sequence_number;
    uint32_t index = 1;
    if (source->sequence_number == 0) {
        data[1] = source->length;
        data[2] = source->length >> 8;
        index = 3;
    }
    uint32_t n = source->length - source->offset;
    if (n > (length - index)) {
        n = length - index;
    }
    memcpy(&data[index], &source->data[source->offset], n);
    source->offset += n;
    ++source->sequence_number;
    if (source->length == source->offset) {
        source->state = fd_detour_state_success;
    }
    return true;
}

