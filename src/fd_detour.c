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
    detour->length = fd_binary_unpack_uint16(&data[0]);
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

void fd_detour_source_initialize(fd_detour_source_t *source, fd_detour_supplier_t supplier, uint32_t length) {
    source->next = 0;
    source->previous = 0;
    source->supplier = supplier;
    source->state = fd_detour_state_intermediate;
    source->length = length;
    source->sequence_number = 0;
    source->offset = 0;
}

bool fd_detour_source_get(fd_detour_source_t *source, uint8_t *data, uint32_t length) {
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
    (*source->supplier)(source->offset, &data[index], n);
    source->offset += n;
    ++source->sequence_number;
    if (source->length == source->offset) {
        source->state = fd_detour_state_success;
    }
    return true;
}

void fd_detour_source_collection_initialize(fd_detour_source_collection_t *collection) {
    collection->first = 0;
    collection->last = 0;
}

void fd_detour_source_collection_push(fd_detour_source_collection_t *collection, fd_detour_source_t *source) {
    if (collection->first == 0) {
        collection->first = source;
        collection->last = source;
        return;
    }

    fd_detour_source_t *old_last = collection->last;
    collection->last = source;
    source->previous = old_last;
    old_last->next = source;
}

void fd_detour_source_collection_pop(fd_detour_source_collection_t *collection) {
    if (collection->first == 0) {
        return;
    }

    if (collection->first == collection->last) {
        fd_detour_source_t *old_first = collection->first;
        old_first->next = 0;
        old_first->previous = 0;
        collection->first = 0;
        collection->last = 0;
        return;
    }

    fd_detour_source_t *old_first = collection->first;
    fd_detour_source_t *new_first = old_first->next;
    collection->first = new_first;
    old_first->next = 0;
    new_first->previous = 0;
}