#include "fd_activity.h"
#include "fd_binary.h"
#include "fd_lis3dh.h"
#include "fd_rtc.h"
#include "fd_sensing.h"
#include "fd_storage_buffer.h"
#include "fd_timer.h"

static fd_storage_buffer_t fd_sensing_storage_buffer;
static uint32_t fd_sensing_interval;
static fd_timer_t fd_sensing_timer;
static fd_time_t fd_sensing_time;
static uint32_t fd_sensing_samples;

static
void fd_sensing_sample_callback(int16_t x, int16_t y, int16_t z) {
    float ax = x / 16384.0f;
    float ay = y / 16384.0f;
    float az = z / 16384.0f;
    fd_activity_accumulate(ax, ay, az);
    ++fd_sensing_samples;
}

static
void fd_sensing_timer_callback(void) {
    fd_lis3dh_read_fifo();
    if (fd_sensing_samples > 0) {
        float activity = fd_activity_value(fd_sensing_interval);
        fd_storage_buffer_add_time_series(&fd_sensing_storage_buffer, fd_sensing_time.seconds, fd_sensing_interval, activity);
    }

    fd_sensing_wake();
}

void fd_sensing_initialize(void) {
    fd_storage_buffer_initialize(&fd_sensing_storage_buffer, FD_STORAGE_TYPE('F', 'D', 'V', 'M'));
    fd_storage_buffer_collection_push(&fd_sensing_storage_buffer);

    fd_lis3dh_set_sample_callback(fd_sensing_sample_callback);

    fd_sensing_interval = 10;
    fd_timer_add(&fd_sensing_timer, fd_sensing_timer_callback);
}

void fd_sensing_wake(void) {
    fd_sensing_samples = 0;
    fd_activity_start();

    fd_timer_start_next(&fd_sensing_timer, fd_sensing_interval);
}

void fd_sensing_sleep(void) {
    fd_timer_stop(&fd_sensing_timer);
}
