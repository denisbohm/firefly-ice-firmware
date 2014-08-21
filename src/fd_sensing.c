#include "fd_activity.h"
#include "fd_binary.h"
#include "fd_hal_accelerometer.h"
#include "fd_hal_rtc.h"
#include "fd_recognition.h"
#include "fd_sensing.h"
#include "fd_storage.h"
#include "fd_storage_buffer.h"
#include "fd_timer.h"

#include <string.h>

// 25 Hz sample rate
#define FD_SENSING_INTERVAL_US 40000
#define FD_SENSING_INTERVAL_MS 40

// ~4 seconds / 118 samples / 2 pages of 32-bit xyz values (3-axis of 10-bits each), each page with 10 byte header containing the time and interval
#define FD_SENSING_HISTORY_LENGTH ((int)(1 * ((FD_STORAGE_MAX_DATA_LENGTH - 10) / sizeof(uint32_t))))
static uint32_t fd_sensing_history[FD_SENSING_HISTORY_LENGTH];
static int fd_sensing_history_tail;
static int fd_sensing_history_count;

static fd_storage_area_t fd_sensing_storage_area;
static fd_storage_buffer_t fd_sensing_storage_buffer;
static fd_storage_buffer_t fd_sensing_stream_storage_buffer;
static uint32_t fd_sensing_interval;
static fd_timer_t fd_sensing_timer;
static fd_time_t fd_sensing_time;
static uint32_t fd_sensing_samples;
static uint32_t fd_sensing_stream_remaining_sample_count;
static fd_time_t fd_sensing_stream_time;

static
fd_time_t fd_sensing_get_sample_time(void) {
    fd_time_t time = fd_hal_rtc_get_time();
    time.microseconds = (time.microseconds / FD_SENSING_INTERVAL_US) * FD_SENSING_INTERVAL_US;
    return time;
}

static
void fd_sensing_history_initialize(void) {
    memset(fd_sensing_history, 0x00, sizeof(fd_sensing_history));
    fd_sensing_history_tail = 0;
    fd_sensing_history_count = 0;
}

static
void fd_sensing_history_add(uint32_t xyz) {
    fd_sensing_history[fd_sensing_history_tail] = xyz;
    if (++fd_sensing_history_tail >= FD_SENSING_HISTORY_LENGTH) {
        fd_sensing_history_tail = 0;
    }
    if (fd_sensing_history_count < FD_SENSING_HISTORY_LENGTH) {
        ++fd_sensing_history_count;
    }
}

void fd_sensing_history_save(void) {
    fd_time_t interval;
    interval.seconds = 0;
    interval.microseconds = FD_SENSING_INTERVAL_US;
    int index = fd_sensing_history_tail - fd_sensing_history_count;
    if (index < 0) {
        index += FD_SENSING_HISTORY_LENGTH;
    }
    fd_time_t time = fd_time_subtract(fd_sensing_get_sample_time(), fd_time_multiply(interval, fd_sensing_history_count));
    for (int i = 0; i < fd_sensing_history_count; ++i) {
        uint32_t xyz = fd_sensing_history[index];
        if (++index >= FD_SENSING_HISTORY_LENGTH) {
            index = 0;
        }
        fd_storage_buffer_add_time_series_ms_uint32(&fd_sensing_stream_storage_buffer, time, FD_SENSING_INTERVAL_MS, xyz);
        time = fd_time_add(time, interval);
    }

    fd_sensing_history_tail = 0;
    fd_sensing_history_count = 0;
}

static
void fd_sensing_stream(int16_t x, int16_t y, int16_t z) {
    // 8G range, 10-bit accuracy
    uint32_t x10 = (x >> 5) & 0x03ff;
    uint32_t y10 = (y >> 5) & 0x03ff;
    uint32_t z10 = (z >> 5) & 0x03ff;
    uint32_t xyz = (x10 << 20) | (y10 << 10) | z10;

    if (fd_sensing_stream_remaining_sample_count > 0) {
        fd_time_t interval;
        interval.seconds = 0;
        interval.microseconds = FD_SENSING_INTERVAL_US;
        fd_sensing_stream_time = fd_time_add(fd_sensing_stream_time, interval);
        fd_storage_buffer_add_time_series_ms_uint32(&fd_sensing_stream_storage_buffer, fd_sensing_stream_time, FD_SENSING_INTERVAL_MS, xyz);
        --fd_sensing_stream_remaining_sample_count;
    } else {
        fd_sensing_history_add(xyz);
    }
}

static
void fd_sensing_sample_callback(int16_t x, int16_t y, int16_t z) {
    fd_activity_accumulate(x, y, z);
    ++fd_sensing_samples;

    fd_sensing_stream(x, y, z);

    fd_recognition_sensing(x, y, z);
}

static
void fd_sensing_timer_callback(void) {
    fd_hal_accelerometer_read_fifo();
    if (fd_sensing_samples > 0) {
        float activity = fd_activity_value(fd_sensing_interval);
        fd_storage_buffer_add_time_series_s_float16(&fd_sensing_storage_buffer, fd_sensing_time.seconds, fd_sensing_interval, activity);
    }

    fd_sensing_wake();
}

void fd_sensing_synthesize(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint32_t samples = fd_binary_get_uint32(&binary);
    float activity = fd_binary_get_float32(&binary);

    // clear any pending records out of the RAM buffer
    fd_sensing_storage_buffer.index = 0;

    uint32_t time = fd_sensing_time.seconds - samples * fd_sensing_interval;
    for (uint32_t i = 0; i < samples; ++i) {
        fd_storage_buffer_add_time_series_s_float16(&fd_sensing_storage_buffer, time, fd_sensing_interval, activity);
        time += fd_sensing_interval;
    }
}

void fd_sensing_initialize(void) {
    fd_sensing_history_initialize();

    // sensing storage will use sectors 64-511 (sectors 0-63 are for firmware updates)
    fd_storage_area_initialize(&fd_sensing_storage_area, 64, 511);
    fd_storage_buffer_initialize(&fd_sensing_storage_buffer, &fd_sensing_storage_area, FD_STORAGE_TYPE('F', 'D', 'V', '2'));
    fd_storage_buffer_collection_push(&fd_sensing_storage_buffer);

    fd_storage_buffer_initialize(&fd_sensing_stream_storage_buffer, &fd_sensing_storage_area, FD_STORAGE_TYPE('F', 'D', 'S', 'A'));
    fd_storage_buffer_collection_push(&fd_sensing_stream_storage_buffer);
    fd_sensing_stream_remaining_sample_count = 0;

    fd_hal_accelerometer_set_sample_callback(fd_sensing_sample_callback);

    fd_sensing_interval = 10;
    fd_timer_add(&fd_sensing_timer, fd_sensing_timer_callback);
}

void fd_sensing_set_stream_sample_count(uint32_t count) {
    fd_sensing_stream_remaining_sample_count = count;
    fd_sensing_stream_time = fd_sensing_get_sample_time();
}

uint32_t fd_sensing_get_stream_sample_count(void) {
    return fd_sensing_stream_remaining_sample_count;
}

void fd_sensing_wake(void) {
    fd_sensing_samples = 0;
    fd_activity_start();

    fd_time_t now = fd_hal_rtc_get_time();
    fd_sensing_time.microseconds = 0;
    fd_sensing_time.seconds = (now.seconds / fd_sensing_interval) * fd_sensing_interval;
    fd_time_t at;
    at.microseconds = 0;
    at.seconds = fd_sensing_time.seconds + fd_sensing_interval;
    fd_time_t duration = fd_time_subtract(at, now);
    fd_timer_start(&fd_sensing_timer, duration);
}

void fd_sensing_sleep(void) {
    fd_timer_stop(&fd_sensing_timer);
}
