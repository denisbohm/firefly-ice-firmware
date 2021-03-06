#include "fd_lsm6dsl.h"

#include "fd_delay.h"
#include "fd_log.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const float fd_lsm6dsl_accelerometer_scales[] = {
    2.0f / 0x7fff,
    16.0f / 0x7fff,
    4.0f / 0x7fff,
    8.0f / 0x7fff,
};

float fd_lsm6dsl_accelerometer_scale(uint32_t fs) {
    return fd_lsm6dsl_accelerometer_scales[fs & 0x3];
}

static const float fd_lsm6dsl_gyro_scales[] = {
    250.0f / 0x7fff,
    500.0f / 0x7fff,
    1000.0f / 0x7fff,
    2000.0f / 0x7fff,
};

uint8_t fd_lsm6ds3_who_am_i;
bool fd_lsm6dsl_gyro_enabled;
bool fd_lsm6dsl_accelerometer_enabled;
bool fd_lsm6dsl_timestamp_and_steps_enabled;

float fd_lsm6dsl_gyro_scale(uint32_t fs) {
    return fd_lsm6dsl_gyro_scales[fs & 0x3];
}

uint8_t fd_lsm6dsl_read(const fd_spim_device_t *device, uint8_t location) {
    return fd_spim_device_sequence_tx1_rx1(device, FD_LSM6DSL_READ | location);
}

void fd_lsm6dsl_write(const fd_spim_device_t *device, uint8_t location, uint8_t byte) {
    fd_spim_device_tx2(device, location, byte);
}

void fd_lsm6dsl_write16(const fd_spim_device_t *device, uint8_t location, uint16_t word) {
    uint8_t bytes[] = {location, word, word >> 8};
    fd_spim_device_txn(device, bytes, sizeof(bytes));
}

static inline
uint16_t fd_lsm6dsl_to_uint16(uint8_t *bytes, uint32_t offset) {
    uint8_t b0 = bytes[offset];
    uint8_t b1 = bytes[offset + 1];
    return (uint16_t)((b1 << 8) | b0);
}

uint32_t fd_lsm6dsl_get_axis_count(void) {
    uint32_t count = 0;
    if (fd_lsm6dsl_gyro_enabled) {
        count += 3;
    }
    if (fd_lsm6dsl_accelerometer_enabled) {
        count += 3;
    }
    if (fd_lsm6dsl_timestamp_and_steps_enabled) {
        count += 3;
    }
    return count;
}

uint32_t fd_lsm6dsl_read_fifo_word_count(const fd_spim_device_t *device) {
    uint16_t last_fifo_status_12 = 0;
    uint16_t last_unread_words = 0;
    uint16_t last_pattern = 0;
    for (int retry = 0; retry < 10; ++retry) {
        bool int1 __attribute__((unused)) = fd_gpio_get((fd_gpio_t){ .port = 1, .pin = 6 });
        fd_spim_device_select(device);
        uint8_t location = FD_LSM6DSL_READ | FD_LSM6DSL_REGISTER_FIFO_STATUS1;
        uint8_t buffer[4];
        fd_spim_bus_sequence_txn_rxn(device->bus, &location, 1, buffer, sizeof(buffer));
        fd_spim_bus_wait(device->bus);
        uint16_t fifo_status_12 = fd_lsm6dsl_to_uint16(buffer, 0);
        uint16_t unread_words = fifo_status_12 & 0x0fff;
        uint16_t pattern = fd_lsm6dsl_to_uint16(buffer, 2) & 0x03ff;

        uint8_t fifo_status2 = buffer[1];
        if (fifo_status2 & FD_LSM6DSL_FIFO_STATUS2_EMPTY) {
            fd_spim_device_deselect(device);
            return 0;
        }

        if (retry > 0) {
            static uint32_t retries = 0;
            ++retries;
        }

        // Check FIFO overrun.
        if (fifo_status2 & FD_LSM6DSL_FIFO_STATUS2_OVER_RUN) {
            static uint32_t count;
            ++count;
            // Note that:
            //   "When a FIFO overrun event occurs (OVER_RUN bit is set high), the value of the DIFF_FIFO_[10:0] field is set to 0."
            // Which means the FIFO is at capacity (2048 words). -denis
            unread_words = 2048;
        }

        last_fifo_status_12 = fifo_status_12;
        last_unread_words = unread_words;
        last_pattern = pattern;

        // pattern will be 0 at the start of a sample set, otherwise try to align pattern by discarding a fifo word
        if (pattern != 0) {
            uint32_t axis_count = fd_lsm6dsl_get_axis_count();
            uint8_t bytes[18 * 2];
            uint32_t skip = (axis_count - pattern) + axis_count;
            if (skip > unread_words) {
                skip = unread_words;
            }
            fd_spim_bus_rxn(device->bus, bytes, 2 * skip);
            fd_spim_bus_wait(device->bus);
            fd_spim_device_deselect(device);
            unread_words -= skip;
            continue;
        }

        fd_spim_device_deselect(device);
#if 0
        if (unread_words == 0) {
            fd_log_assert(int1);
        }
#endif
        return unread_words; // normal return
    }
    return 0; // failed to align pattern
}

uint32_t fd_lsm6dsl_read_fifo_samples(const fd_spim_device_t *device, fd_lsm6dsl_sample_t *samples, uint32_t sample_count) {
    uint32_t word_count = fd_lsm6dsl_read_fifo_word_count(device);
    uint32_t axis_count = fd_lsm6dsl_get_axis_count();
    fd_log_assert(axis_count != 0);
    if (axis_count == 0) {
        axis_count = 1;
    }
    uint32_t count = word_count / axis_count;
    if (count > sample_count) {
        count = sample_count;
    }

    fd_spim_device_select(device);
    fd_spim_bus_tx1(device->bus, FD_LSM6DSL_READ | FD_LSM6DSL_REGISTER_FIFO_DATA_OUT_L);
    fd_spim_bus_wait(device->bus);
    for (int i = 0; i < count; ++i) {
        uint8_t bytes[18];
        uint32_t byte_index = 0;
        fd_spim_bus_rxn(device->bus, bytes, axis_count * 2);
        fd_spim_bus_wait(device->bus);
        fd_lsm6dsl_sample_t *sample = &samples[i];
        memset(sample, 0, sizeof(fd_lsm6dsl_sample_t));
        if (fd_lsm6dsl_gyro_enabled) {
            sample->gyro.x = fd_lsm6dsl_to_uint16(bytes, byte_index);
            byte_index += 2;
            sample->gyro.y = fd_lsm6dsl_to_uint16(bytes, byte_index);
            byte_index += 2;
            sample->gyro.z = fd_lsm6dsl_to_uint16(bytes, byte_index);
            byte_index += 2;
        }
        if (fd_lsm6dsl_accelerometer_enabled) {
            sample->accelerometer.x = fd_lsm6dsl_to_uint16(bytes, byte_index);
            byte_index += 2;
            sample->accelerometer.y = fd_lsm6dsl_to_uint16(bytes, byte_index);
            byte_index += 2;
            sample->accelerometer.z = fd_lsm6dsl_to_uint16(bytes, byte_index);
            byte_index += 2;
        }
        if (fd_lsm6dsl_timestamp_and_steps_enabled) {
            uint32_t timestamp1 = bytes[byte_index];
            byte_index += 1;
            uint32_t timestamp2 = bytes[byte_index];
            byte_index += 1;
            uint32_t unused __attribute__((unused)) = bytes[byte_index];
            byte_index += 1;
            uint32_t timestamp0 = bytes[byte_index];
            byte_index += 1;
            uint32_t timestamp = (timestamp2 << 16) | (timestamp1 << 8) | timestamp0;
            sample->timestamp = timestamp;
            uint32_t steps0 = bytes[byte_index];
            byte_index += 1;
            uint32_t steps1 = bytes[byte_index];
            byte_index += 1;
            sample->steps = (steps1 << 8) | steps0;
        }
    }
    fd_spim_device_deselect(device);
    return count;
}

void fd_lsm6dsl_fifo_flush(const fd_spim_device_t *device) {
    fd_spim_device_select(device);
    uint8_t location = FD_LSM6DSL_READ | FD_LSM6DSL_REGISTER_FIFO_STATUS1;
    uint8_t buffer[4];
    fd_spim_bus_sequence_txn_rxn(device->bus, &location, 1, buffer, sizeof(buffer));
    fd_spim_bus_wait(device->bus);
    uint16_t fifo_status_12 = fd_lsm6dsl_to_uint16(buffer, 0);
    uint16_t unread_words = fifo_status_12 & 0x0fff;
    uint16_t pattern __attribute__((unused)) = fd_lsm6dsl_to_uint16(buffer, 2) & 0x03ff;

    uint8_t fifo_status2 = buffer[1];
    if (fifo_status2 & FD_LSM6DSL_FIFO_STATUS2_EMPTY) {
        fd_spim_device_deselect(device);
        return;
    }

    // Check FIFO overrun.
    if (fifo_status2 & FD_LSM6DSL_FIFO_STATUS2_OVER_RUN) {
        // Note that:
        //   "When a FIFO overrun event occurs (OVER_RUN bit is set high), the value of the DIFF_FIFO_[10:0] field is set to 0."
        // Which means the FIFO is at capacity (2048 words). -denis
        unread_words = 2048;
    }

    // read all the words currently in the buffer
    for (int i = 0; i < unread_words; ++i) {
        uint8_t bytes[2];
        fd_spim_bus_rxn(device->bus, bytes, sizeof(bytes));
        fd_spim_bus_wait(device->bus);
    }
    fd_spim_device_deselect(device);
}

uint32_t fd_lsm6dsl_get_step_count(const fd_spim_device_t *device) {
    uint32_t step_counter_l = fd_lsm6dsl_read(device, FD_LSM6DSL_REGISTER_STEP_COUNTER_L);
    uint32_t step_counter_h = fd_lsm6dsl_read(device, FD_LSM6DSL_REGISTER_STEP_COUNTER_H);
    uint32_t step_counter = (step_counter_h << 8) | step_counter_l;
    return step_counter;
}

void fd_lsm6dsl_clear_step_count(const fd_spim_device_t *device) {
    uint8_t ctrl10_c = fd_lsm6dsl_read(device, FD_LSM6DSL_REGISTER_CTRL10_C);
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL10_C, ctrl10_c | 0x2); // reset step counter
    uint32_t step_counter = 0;
    for (int i = 0; i < 20; ++i) {
        fd_delay_ms(1);
        step_counter = fd_lsm6dsl_get_step_count(device);
        if (step_counter == 0) {
            break;
        }
    }
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL10_C, ctrl10_c);
    if (step_counter != 0) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "step count! %d", step_counter);
        fd_log(buffer);
    }
}

void fd_lsm6ds3_configure(const fd_spim_device_t *device, const fd_lsm6dsl_configuration_t *configuration) {
    fd_lsm6ds3_who_am_i = fd_lsm6dsl_read(device, FD_LSM6DSL_REGISTER_WHO_AM_I);

    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL4_C, 0b00000100); // disable I2C
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL3_C, 0b01100100); // block data update, int1/2 push pull, address automatically incremented

    fd_lsm6dsl_accelerometer_enabled = configuration->accelerometer_enable;
    uint32_t accelerometer_output_data_rate = configuration->accelerometer_output_data_rate;
    if (!configuration->accelerometer_enable) {
        accelerometer_output_data_rate = FD_LSM6DSL_ODR_POWER_DOWN;
    }
    
    fd_lsm6dsl_gyro_enabled = configuration->gyro_enable;
    uint32_t gyro_output_data_rate = configuration->gyro_output_data_rate;
    if (!configuration->gyro_enable) {
        gyro_output_data_rate = FD_LSM6DSL_ODR_POWER_DOWN;
    }

    if (configuration->timestamp_and_steps_enable) {
        fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_FUNC_CFG_ACCESS, 0x80);
        uint8_t value = configuration->steps_threshold;
        if (configuration->accelerometer_full_scale_range >= FD_LSM6DSL_ODR_26_HZ) {
            value |= 0x80; // PEDO_FS = ±4 g
        }
        fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_FUNC_CONFIG_PEDO_THS_MIN, value);
        fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_FUNC_CFG_ACCESS, 0x00);
    }
    
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL1_XL,
        (accelerometer_output_data_rate << 4) |
        (configuration->accelerometer_full_scale_range << 2) |
        configuration->accelerometer_bandwidth_filter
    );
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL6_C,
        configuration->accelerometer_low_power ? 0b00010000 : 0b00000000
    );
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL9_XL,
        configuration->accelerometer_enable ? 0b11100000 : 0b00000000
    );

    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL2_G,
        (gyro_output_data_rate << 4) |
        (configuration->gyro_full_scale_range << 1)
    );
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL7_G,
        ((configuration->gyro_low_power ? 1 : 0) << 7) |
        (configuration->gyro_high_pass_filter << 4)
    );
    fd_lsm6dsl_timestamp_and_steps_enabled = configuration->timestamp_and_steps_enable;
    if (fd_lsm6dsl_timestamp_and_steps_enabled) {
        // enable timestamp, pedometer, enable functions
        fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL10_C, 0b00110100);
    } else {
        fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL10_C, 0b00000000);
    }
    fd_lsm6dsl_clear_step_count(device);

    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_FIFO_CTRL4, fd_lsm6dsl_timestamp_and_steps_enabled ? 0b00001000 : 0b00000000); // no timestamp decimation
    fd_lsm6dsl_write16(device, FD_LSM6DSL_REGISTER_FIFO_CTRL1,
        (fd_lsm6dsl_timestamp_and_steps_enabled ? 0x8000 /* enable timestamp in fifo */ : 0x0000) | (configuration->fifo_threshold & 0x07ff)
    );
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_FIFO_CTRL3,
        (configuration->gyro_enable ? 0b00001000 /* no decimation */ : 0b00000000) |
        (configuration->accelerometer_enable ? 0b00000001 /* no decimation */ : 0b00000000)
    );
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_FIFO_CTRL5, 0x00);
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_TIMESTAMP2_REG, 0xaa); // reset timestamp
    uint32_t axis_count = fd_lsm6dsl_get_axis_count();
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_FIFO_CTRL5,
        (axis_count != 0) ? ((configuration->fifo_output_data_rate << 3) | 0b110 /* continuous  */ ) : 0
    );
    fd_lsm6dsl_fifo_flush(device);
}

void fd_lsm6dsl_reset_step_counter(const fd_spim_device_t *device) {
    if (fd_lsm6dsl_timestamp_and_steps_enabled) {
        fd_lsm6dsl_clear_step_count(device);
    }
}

typedef struct {
    uint8_t location;
    uint8_t value;
} fd_lsm6dsl_access_t;

void fd_lsm6dsl_get_stats(
    const fd_spim_device_t *device,
    fd_lsm6dsl_accelerometer_sample_t *avg,
    fd_lsm6dsl_accelerometer_sample_t *min,
    fd_lsm6dsl_accelerometer_sample_t *max
) {
    fd_delay_ms(100);
    fd_lsm6dsl_accelerometer_sample_t samples[6];
    for (int i = 0; i < 6; ++i) {
        uint8_t status_reg;
        do {
            status_reg = fd_lsm6dsl_read(device, FD_LSM6DSL_REGISTER_STATUS_REG);
        } while ((status_reg & FD_LSM6DSLSTATUS_XLDA) == 0);
        uint8_t tx_bytes[] = { FD_LSM6DSL_READ | FD_LSM6DSL_REGISTER_OUTX_L_XL };
        uint8_t rx_bytes[6];
        fd_spim_device_sequence_txn_rxn(device, tx_bytes, sizeof(tx_bytes), rx_bytes, sizeof(rx_bytes));
        samples[i] = (fd_lsm6dsl_accelerometer_sample_t){
            .x = (rx_bytes[1] << 8) | rx_bytes[0],
            .y = (rx_bytes[3] << 8) | rx_bytes[2],
            .z = (rx_bytes[5] << 8) | rx_bytes[4],
        };
    }
    int32_t x = samples[1].x;
    int32_t y = samples[1].y;
    int32_t z = samples[1].z;
    int16_t x_min = x;
    int16_t y_min = y;
    int16_t z_min = z;
    int16_t x_max = x;
    int16_t y_max = y;
    int16_t z_max = z;
    for (int i = 2; i < 6; ++i) {
        fd_lsm6dsl_accelerometer_sample_t s = samples[i];
        x += s.x;
        y += s.y;
        z += s.z;
        if (s.x < x_min) {
            x_min = s.x;
        }
        if (s.y < y_min) {
            y_min = s.y;
        }
        if (s.z < z_min) {
            z_min = s.z;
        }
        if (s.x > x_max) {
            x_max = s.x;
        }
        if (s.y > y_max) {
            y_max = s.y;
        }
        if (s.z > z_max) {
            z_max = s.z;
        }
    }
    if (avg) {
        *avg = (fd_lsm6dsl_accelerometer_sample_t){
            .x = x / 5,
            .y = y / 5,
            .z = z / 5,
        };
    }
    if (min) {
        *min = (fd_lsm6dsl_accelerometer_sample_t){
            .x = x_min,
            .y = y_min,
            .z = z_min,
        };
    }
    if (max) {
        *max = (fd_lsm6dsl_accelerometer_sample_t){
            .x = x_max,
            .y = y_max,
            .z = z_max,
        };
    }
}

bool fd_lsm6dsl_self_test(const fd_spim_device_t *device) {
    fd_lsm6dsl_access_t setup_accesses[] = {
        { .location = FD_LSM6DSL_REGISTER_CTRL1_XL, .value = 0x38 },
        { .location = FD_LSM6DSL_REGISTER_CTRL2_G,  .value = 0x00 },
        { .location = FD_LSM6DSL_REGISTER_CTRL3_C,  .value = 0x44 },
        { .location = FD_LSM6DSL_REGISTER_CTRL4_C,  .value = 0x00 },
        { .location = FD_LSM6DSL_REGISTER_CTRL5_C,  .value = 0x00 },
        { .location = FD_LSM6DSL_REGISTER_CTRL6_C,  .value = 0x00 },
        { .location = FD_LSM6DSL_REGISTER_CTRL7_G,  .value = 0x00 },
        { .location = FD_LSM6DSL_REGISTER_CTRL8_XL, .value = 0x00 },
        { .location = FD_LSM6DSL_REGISTER_CTRL9_XL, .value = 0x00 },
        { .location = FD_LSM6DSL_REGISTER_CTRL10_C, .value = 0x00 },
    };
    uint32_t setup_count = sizeof(setup_accesses) / sizeof(setup_accesses[0]);
    for (int i = 0; i < setup_count; ++i) {
        fd_lsm6dsl_access_t *access = &setup_accesses[i];
        fd_lsm6dsl_write(device, access->location, access->value);
    }

    fd_lsm6dsl_accelerometer_sample_t nost;
    fd_lsm6dsl_accelerometer_sample_t nost_min;
    fd_lsm6dsl_accelerometer_sample_t nost_max;
    fd_lsm6dsl_get_stats(device, &nost, &nost_min, &nost_max);
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL5_C, 0x01);
    fd_lsm6dsl_accelerometer_sample_t st;
    fd_lsm6dsl_accelerometer_sample_t st_min;
    fd_lsm6dsl_accelerometer_sample_t st_max;
    fd_lsm6dsl_get_stats(device, &st, &st_min, &st_max);

    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL1_XL, 0x00);
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL5_C, 0x00);

    bool tx = (abs(st_min.x) <= abs(st.x - nost.x)) && (abs(st.x - nost.x) <= abs(st_max.x));
    bool ty = (abs(st_min.y) <= abs(st.y - nost.y)) && (abs(st.y - nost.y) <= abs(st_max.y));
    bool tz = (abs(st_min.z) <= abs(st.z - nost.z)) && (abs(st.z - nost.z) <= abs(st_max.z));
    return tx && ty && tz;
}
