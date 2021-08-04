#include "fdi_detour.h"

#include "fd_binary.h"
#include "fd_log.h"

#include <string.h>

void fdi_detour_initialize(fdi_detour_t *detour, uint8_t *data, uint32_t size) {
    detour->data = data;
    detour->size = size;
    detour->state = fdi_detour_state_clear;
    detour->length = 0;
    detour->sequence_number = 0;
    detour->offset = 0;
}

void fdi_detour_clear(fdi_detour_t *detour) {
    detour->state = fdi_detour_state_clear;
    detour->length = 0;
    detour->sequence_number = 0;
    detour->offset = 0;
}

fdi_detour_state_t fdi_detour_state(fdi_detour_t *detour) {
    return detour->state;
}

static
void fdi_detour_error(fdi_detour_t *detour) {
    detour->state = fdi_detour_state_error;
}

static
void fdi_detour_continue(fdi_detour_t *detour, uint8_t *data, uint32_t length) {
    uint32_t total = detour->offset + length;
    if (total > detour->length) {
        // ignore any extra data at the end of the transfer
        length = detour->length - detour->offset;
    }
    memcpy(&detour->data[detour->offset], data, length);
    detour->offset += length;
    if (detour->offset == detour->length) {
        detour->state = fdi_detour_state_success;
    } else {
        ++detour->sequence_number;
    }
}

static
void fdi_detour_start(fdi_detour_t *detour, uint8_t *data, uint32_t length) {
    if (length < 2) {
        fdi_detour_error(detour);
        return;
    }
    detour->state = fdi_detour_state_intermediate;
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    detour->length = fd_binary_get_varuint(&binary);
    detour->sequence_number = 0;
    detour->offset = 0;
    if (detour->length > detour->size) {
        fdi_detour_error(detour);
        return;
    }
    fdi_detour_continue(detour, &data[binary.get_index], length - binary.get_index);
}

void fdi_detour_event(fdi_detour_t *detour, uint8_t *data, uint32_t length) {
    if (length < 1) {
        fdi_detour_error(detour);
        return;
    }
    uint8_t sequence_number = data[0];
    if (sequence_number == 0) {
        if (detour->sequence_number != 0) {
            fdi_detour_error(detour);
        }
        fdi_detour_start(detour, &data[1], length - 1);
    } else
    if (sequence_number != detour->sequence_number) {
        fdi_detour_error(detour);
    } else {
        fdi_detour_continue(detour, &data[1], length - 1);
    }
}
