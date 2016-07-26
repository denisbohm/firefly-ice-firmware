#include "fd_timing.h"

#include <string.h>

void fd_timing_initialize(fd_timing_t *timing, const char *identifier) {
    timing->identifier = identifier;
    fd_timing_clear(timing);
}

fd_timing_t *fd_timing_iterate_array_of_objects(fd_timing_iterator_t *iterator) {
    void *timing = ((uint8_t *)iterator->array) + iterator->type_size * iterator->index + iterator->member_offset;
    return (fd_timing_t *)timing;
}

fd_timing_t *fd_timing_iterate_array_of_pointers(fd_timing_iterator_t *iterator) {
    void **object = (void **)(((uint8_t *)iterator->array) + sizeof(void *) * iterator->index);
    void *timing = ((uint8_t *)*object) + iterator->member_offset;
    return (fd_timing_t *)timing;
}

fd_timing_t *fd_timing_iterate(fd_timing_iterator_t *iterator) {
    if (iterator->index >= iterator->count) {
        return 0;
    }
    fd_timing_t *timing = iterator->iterate(iterator);
    iterator->index += 1;
    return timing;
}

void fd_timing_put_binary(fd_timing_t *timing, fd_binary_t *binary) {
    uint32_t length = (uint32_t)strlen(timing->identifier);
    fd_binary_put_uint8(binary, length);
    fd_binary_put_bytes(binary, (uint8_t *)timing->identifier, length);
    fd_binary_put_uint32(binary, timing->count);
    fd_binary_put_uint32(binary, timing->min_duration);
    fd_binary_put_uint32(binary, timing->max_duration);
    fd_binary_put_float32(binary, timing->total_duration);
    fd_binary_put_float32(binary, timing->sum_squared_duration);
}

void fd_timing_clear(fd_timing_t *timing) {
    timing->count = 0;
    timing->min_duration = 0;
    timing->max_duration = 0;
    timing->total_duration = 0.0;
    timing->sum_squared_duration = 0.0;
    timing->start = 0;
}

void fd_timing_start(fd_timing_t *timing) {
    fd_hal_timing_adjust();
    timing->start = fd_hal_timing_get_timestamp();
}

void fd_timing_end(fd_timing_t *timing) {
    uint32_t now = fd_hal_timing_get_timestamp();
    uint32_t start = timing->start;
    double duration = (now < start) ? ((0xffffffff - start) + 1 + now) : (now - start);
    timing->total_duration += duration;
    timing->sum_squared_duration += duration * duration;
    timing->count += 1;
    if (timing->count == 1) {
        timing->min_duration = duration;
        timing->max_duration = duration;
    } else {
        if (duration < timing->min_duration) {
            timing->min_duration = duration;
        }
        if (duration > timing->max_duration) {
            timing->max_duration = duration;
        }
    }
}
