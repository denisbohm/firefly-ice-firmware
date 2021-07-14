#ifndef FD_TIMING_H
#define FD_TIMING_H

#include "fd_binary.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *identifier;

    uint32_t count;
    uint32_t min_duration;
    uint32_t max_duration;
    double total_duration;
    double sum_squared_duration;

    uint32_t start;
} fd_timing_t;

void fd_timing_initialize(fd_timing_t *timing, const char *identifier);
void fd_timing_clear(fd_timing_t *timing);
void fd_timing_start(fd_timing_t *timing);
void fd_timing_end(fd_timing_t *timing);
void fd_timing_put_binary(fd_timing_t *timing, fd_binary_t *binary);
void fd_timing_format(const fd_timing_t *timing, char *buffer, size_t size);

void fd_hal_timing_initialize(void);
bool fd_hal_timing_get_enable(void);
void fd_hal_timing_set_enable(bool enable);
void fd_hal_timing_adjust(void);
uint32_t fd_hal_timing_get_timestamp(void);

typedef struct fd_timing_iterator_s {
    fd_timing_t *(*iterate)(struct fd_timing_iterator_s *iterator);
    size_t type_size;
    size_t member_offset;
    void *array;
    size_t count;
    size_t index;
} fd_timing_iterator_t;

fd_timing_t *fd_timing_iterate_array_of_objects(fd_timing_iterator_t *iterator);

#define fd_timing_iterator_array_of_objects(a_type, a_member, a_array, a_count) {\
.iterate = fd_timing_iterate_array_of_objects,\
.type_size = sizeof(a_type),\
.member_offset = offsetof(a_type, a_member),\
.array = a_array,\
.count = a_count,\
.index = 0\
}

fd_timing_t *fd_timing_iterate_array_of_pointers(fd_timing_iterator_t *iterator);

#define fd_timing_iterator_array_of_pointers(a_type, a_member, a_array, a_count) {\
.iterate = fd_timing_iterate_array_of_pointers,\
.type_size = sizeof(a_type),\
.member_offset = offsetof(a_type, a_member),\
.array = a_array,\
.count = a_count,\
.index = 0\
}

#define fd_timing_iterator_nil() {\
.type_size = 0,\
.member_offset = 0,\
.array = 0,\
.count = 0,\
.index = 0\
}

fd_timing_t *fd_timing_iterate(fd_timing_iterator_t *iterator);

#endif