#include "fd_rtc.h"
#include "fd_timing.h"

typedef struct {
    fd_time_t time;
    uint32_t index;
    uint32_t code;
    uint32_t arg;
} fd_timing_t;

#define TIMING_SIZE 64
static fd_timing_t timings[TIMING_SIZE];
static uint32_t timing_index;

void fd_timing_add(uint32_t code, uint32_t arg) {
    if (timing_index >= TIMING_SIZE) {
        timing_index = 0;
    }
    fd_timing_t *timing = &timings[timing_index];
    timing->time = fd_rtc_get_accurate_time();
    timing->index = timing_index;
    timing->code = code;
    timing->arg = arg;
    ++timing_index;
}