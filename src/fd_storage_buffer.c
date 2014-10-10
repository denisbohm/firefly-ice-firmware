#include "fd_binary.h"
#include "fd_crc.h"
#include "fd_storage.h"
#include "fd_storage_buffer.h"

#include <string.h>

typedef struct {
    fd_storage_buffer_t *first;
    fd_storage_buffer_t *last;
} fd_storage_buffer_collection_t;

static fd_storage_buffer_collection_t storage_buffer_collection;

void fd_storage_buffer_collection_initialize(void) {
    storage_buffer_collection.first = 0;
    storage_buffer_collection.last = 0;
}

void fd_storage_buffer_collection_push(fd_storage_buffer_t *storage_buffer) {
    if (storage_buffer_collection.first == 0) {
        storage_buffer_collection.first = storage_buffer;
        storage_buffer_collection.last = storage_buffer;
        return;
    }

    // the storage buffer is already in the collection - nothing to do
    if ((storage_buffer_collection.first == storage_buffer) || (storage_buffer->previous != 0)) {
        return;
    }

    fd_storage_buffer_t *old_last = storage_buffer_collection.last;
    storage_buffer_collection.last = storage_buffer;
    storage_buffer->previous = old_last;
    old_last->next = storage_buffer;
}

bool fd_storage_buffer_get_first_page(fd_storage_metadata_t *metadata, uint8_t *data, uint32_t length) {
    fd_storage_buffer_t *storage_buffer = storage_buffer_collection.first;
    while (storage_buffer) {
        uint8_t storage_buffer_length = storage_buffer->index;
        if (storage_buffer_length > 0) {
            if (storage_buffer_length > FD_STORAGE_MAX_DATA_LENGTH) {
                storage_buffer_length = FD_STORAGE_MAX_DATA_LENGTH;
            }
            uint32_t type = storage_buffer->type;
            uint8_t buffer[8 + 256] = {0x00, storage_buffer_length, 0, 0, type, type >> 8, type >> 16, type >> 24};
            memcpy(&buffer[8], storage_buffer->data, storage_buffer_length);
            uint16_t hash = fd_crc_16(0xffff, &buffer[4], 4 + storage_buffer_length);
            buffer[2] = hash;
            buffer[3] = hash >> 8;

            metadata->page = 0xffffffff;
            metadata->length = storage_buffer_length;
            metadata->hash = hash;
            metadata->type = type;
            if (storage_buffer_length < length) {
                length = storage_buffer_length;
            }
            memcpy(data, &buffer[8], length);
            return true;
        }
        storage_buffer = storage_buffer->next;
    }
    return false;
}

void fd_storage_buffer_clear_page(fd_storage_metadata_t *metadata) {
    fd_storage_buffer_t *storage_buffer = storage_buffer_collection.first;
    while (storage_buffer) {
        uint32_t type = storage_buffer->type;
        if (type == metadata->type) {
            uint8_t length = storage_buffer->index;
            uint8_t buffer[8 + 256] = {0x00, length, 0, 0, type, type >> 8, type >> 16, type >> 24};
            memcpy(&buffer[8], storage_buffer->data, length);
            uint16_t hash = fd_crc_16(0xffff, &buffer[4], 4 + length);
            if (hash == metadata->hash) {
                storage_buffer->index = 0;
            }
            break;
        }
        storage_buffer = storage_buffer->next;
    }
}

void fd_storage_buffer_initialize(fd_storage_buffer_t *storage_buffer, fd_storage_area_t *area, uint32_t type) {
    storage_buffer->next = 0;
    storage_buffer->previous = 0;

    storage_buffer->area = area;
    storage_buffer->type = type;
    storage_buffer->index = 0;
}

void fd_storage_buffer_erase(fd_storage_buffer_t *storage_buffer) {
    storage_buffer->index = 0;
}

void fd_storage_buffer_flush(fd_storage_buffer_t *storage_buffer) {
    if (storage_buffer->index > 0) {
        fd_storage_area_append_page(storage_buffer->area, storage_buffer->type, storage_buffer->data, storage_buffer->index);
        storage_buffer->index = 0;
    }
}

void fd_storage_buffer_add(fd_storage_buffer_t *storage_buffer, uint8_t *data, uint32_t length) {
    if ((storage_buffer->index + length) > FD_STORAGE_MAX_DATA_LENGTH) {
        fd_storage_buffer_flush(storage_buffer);
    }
    memcpy(&storage_buffer->data[storage_buffer->index], data, length);
    storage_buffer->index += length;
}

#define SIZEOF_UINT16 2
#define SIZEOF_UINT32 4
#define SIZEOF_FLOAT16 2

void fd_storage_buffer_add_time_series_s_float16(
    fd_storage_buffer_t *storage_buffer, uint32_t time_s, uint16_t interval_s, float value
) {
    if ((storage_buffer->index + SIZEOF_FLOAT16) > FD_STORAGE_MAX_DATA_LENGTH) {
        fd_storage_buffer_flush(storage_buffer);
    }
    if (storage_buffer->index == 0) {
        fd_binary_pack_uint32(&storage_buffer->data[storage_buffer->index], time_s);
        storage_buffer->index += SIZEOF_UINT32;
        fd_binary_pack_uint16(&storage_buffer->data[storage_buffer->index], interval_s);
        storage_buffer->index += SIZEOF_UINT16;
    }
    fd_binary_pack_float16(&storage_buffer->data[storage_buffer->index], value);
    storage_buffer->index += SIZEOF_FLOAT16;
}

void fd_storage_buffer_add_time_series_ms_uint32(
    fd_storage_buffer_t *storage_buffer, fd_time_t time, uint16_t interval_ms, uint32_t value
) {
    if ((storage_buffer->index + SIZEOF_UINT32) > FD_STORAGE_MAX_DATA_LENGTH) {
        fd_storage_buffer_flush(storage_buffer);
    }
    if (storage_buffer->index == 0) {
        fd_binary_pack_uint32(&storage_buffer->data[storage_buffer->index], time.seconds);
        storage_buffer->index += SIZEOF_UINT32;
        fd_binary_pack_uint32(&storage_buffer->data[storage_buffer->index], time.microseconds);
        storage_buffer->index += SIZEOF_UINT32;
        fd_binary_pack_uint16(&storage_buffer->data[storage_buffer->index], interval_ms);
        storage_buffer->index += SIZEOF_UINT16;
    }
    fd_binary_pack_uint32(&storage_buffer->data[storage_buffer->index], value);
    storage_buffer->index += SIZEOF_UINT32;
}