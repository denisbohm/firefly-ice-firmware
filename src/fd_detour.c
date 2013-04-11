#include "fd_binary.h"
#include "fd_detour.h"

#include <string.h>

void fd_detour_initialize(fd_detour_t *detour, uint8_t *data, uint32_t size) {
    detour->data = data;
    detour->size = size;
    detour->state = fd_detour_state_clear;
    detour->length = 0;
    detour->sequence_number = 0;
    detour->offset = 0;
}

void fd_detour_clear(fd_detour_t *detour) {
    detour->state = fd_detour_state_clear;
    detour->length = 0;
    detour->sequence_number = 0;
    detour->offset = 0;
}

fd_detour_state_t fd_detour_state(fd_detour_t *detour) {
    return detour->state;
}

static
void fd_detour_error(fd_detour_t *detour) {
    detour->state = fd_detour_state_error;
}

static
void fd_detour_continue(fd_detour_t *detour, uint8_t *data, uint32_t length) {
    uint32_t total = detour->offset + length;
    if (total > detour->length) {
        // ignore any extra data at the end of the transfer
        length = detour->length - detour->offset;
    }
    memcpy(&detour->data[detour->offset], data, length);
    detour->offset += length;
    if (detour->offset == detour->length) {
        detour->state = fd_detour_state_success;
    } else {
        ++detour->sequence_number;
    }
}

static
void fd_detour_start(fd_detour_t *detour, uint8_t *data, uint32_t length) {
    if (length < 2) {
        fd_detour_error(detour);
        return;
    }
    detour->state = fd_detour_state_intermediate;
    detour->length = fd_binary_get_uint16(&data[0]);
    detour->sequence_number = 0;
    detour->offset = 0;
    if (detour->length > detour->size) {
        fd_detour_error(detour);
        return;
    }
    fd_detour_continue(detour, &data[2], length - 2);
}

void fd_detour_event(fd_detour_t *detour, uint8_t *data, uint32_t length) {
    if (length < 1) {
        fd_detour_error(detour);
        return;
    }
    uint8_t sequence_number = data[0];
    if (sequence_number == 0) {
        if (detour->sequence_number != 0) {
            fd_detour_error(detour);
        }
        fd_detour_start(detour, &data[1], length - 1);
    } else
    if (sequence_number != detour->sequence_number) {
        fd_detour_error(detour);
    } else {
        fd_detour_continue(detour, &data[1], length - 1);
    }
}