#include "fd_lsm6dsl.h"

#include "string.h"

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

float fd_lsm6dsl_gyro_scale(uint32_t fs) {
    return fd_lsm6dsl_gyro_scales[fs & 0x3];
}

uint8_t fd_lsm6dsl_read(fd_spim_device_t *device, uint8_t location) {
    return fd_spim_device_sequence_tx1_rx1(device, FD_LSM6DSL_READ | location);
}

void fd_lsm6dsl_write(fd_spim_device_t *device, uint8_t location, uint8_t byte) {
    fd_spim_device_tx2(device, location, byte);
}

void fd_lsm6dsl_write16(fd_spim_device_t *device, uint8_t location, uint16_t word) {
    uint8_t bytes[] = {location, word, word >> 8};
    fd_spim_device_txn(device, bytes, sizeof(bytes));
}

static inline
uint16_t fd_lsm6dsl_to_uint16(uint8_t *bytes, uint32_t offset) {
    uint8_t b0 = bytes[offset];
    uint8_t b1 = bytes[offset + 1];
    return (uint16_t)((b1 << 8) | b0);
}

uint32_t fd_lsm6dsl_read_fifo_word_count(fd_spim_device_t *device) {
    for (int retry = 0; retry < 10; ++retry) {
        fd_spim_device_select(device);
        uint8_t location = FD_LSM6DSL_REGISTER_FIFO_STATUS1;
        uint8_t buffer[4];
        fd_spim_bus_sequence_txn_rxn(device->bus, &location, 1, buffer, sizeof(buffer));
        fd_spim_bus_wait(device->bus);
        uint16_t unread_words = fd_lsm6dsl_to_uint16(buffer, 0) & 0x7ff;
        uint16_t pattern = fd_lsm6dsl_to_uint16(buffer, 2) & 0x3ff;

        // pattern will be 0 at the start of a sample set, otherwise try to align pattern by discarding a fifo word
        if (pattern != 0) {
            uint8_t word[2];
            fd_spim_bus_rxn(device->bus, word, sizeof(word));
            fd_spim_bus_wait(device->bus);
            fd_spim_device_deselect(device);
            continue;
        }

        fd_spim_device_deselect(device);
        return unread_words; // normal return
    }
    return 0; // failed to align pattern
}

uint32_t fd_lsm6dsl_read_fifo_samples(fd_spim_device_t *device, fd_lsm6dsl_sample_t *samples, uint32_t sample_count) {
    uint32_t word_count = fd_lsm6dsl_read_fifo_word_count(device);
    const uint32_t axis_count = 6;
    uint32_t count = word_count / axis_count;
    if (count > sample_count) {
        count = sample_count;
    }

    fd_spim_device_select(device);
    fd_spim_bus_tx1(device->bus, FD_LSM6DSL_READ | FD_LSM6DSL_REGISTER_FIFO_DATA_OUT_L);
    fd_spim_bus_wait(device->bus);
    for (int i = 0; i < count; ++i) {
        uint8_t bytes[12];
        fd_spim_bus_rxn(device->bus, bytes, sizeof(bytes));
        fd_spim_bus_wait(device->bus);
        fd_lsm6dsl_sample_t *sample = &samples[i];
        sample->gyro.x = fd_lsm6dsl_to_uint16(bytes, 0);
        sample->gyro.y = fd_lsm6dsl_to_uint16(bytes, 2);
        sample->gyro.z = fd_lsm6dsl_to_uint16(bytes, 4);
        sample->accelerometer.x = fd_lsm6dsl_to_uint16(bytes, 6);
        sample->accelerometer.y = fd_lsm6dsl_to_uint16(bytes, 8);
        sample->accelerometer.z = fd_lsm6dsl_to_uint16(bytes, 10);
    }
    fd_spim_device_deselect(device);
    return count;
}

void fd_lsm6dsl_fifo_flush(fd_spim_device_t *device) {
    uint32_t word_count = fd_lsm6dsl_read_fifo_word_count(device);

    fd_spim_device_select(device);
    fd_spim_bus_tx1(device->bus, FD_LSM6DSL_READ | FD_LSM6DSL_REGISTER_FIFO_DATA_OUT_L);
    fd_spim_bus_wait(device->bus);
    for (uint32_t i = 0; i < word_count; ++i) {
        uint8_t bytes[2];
        fd_spim_bus_rxn(device->bus, bytes, sizeof(bytes));
        fd_spim_bus_wait(device->bus);
    }
    fd_spim_device_deselect(device);
}

void fd_lsm6ds3_configure(fd_spim_device_t *device, fd_lsm6dsl_configuration_t *configuration) {
    uint8_t who_am_i = fd_lsm6dsl_read(device, FD_LSM6DSL_REGISTER_WHO_AM_I);

    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL4_C, 0b00000100); // disable I2C
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL3_C, 0b01010100); // block data update, int1/2 open drain, address automatically incremented

    uint32_t lsm6ds3_axis_count = 6;
    if (!configuration->accelerometer_enable) {
        configuration->accelerometer_output_data_rate = FD_LSM6DSL_ODR_POWER_DOWN;
        lsm6ds3_axis_count -= 3;
    }
    if (!configuration->gyro_enable) {
        configuration->gyro_output_data_rate = FD_LSM6DSL_ODR_POWER_DOWN;
        lsm6ds3_axis_count -= 3;
    }
    
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL1_XL,
        (configuration->accelerometer_output_data_rate << 4) |
        (configuration->accelerometer_full_scale_range << 2) |
        configuration->accelerometer_bandwidth_filter
    );
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL6_C,
        configuration->accelerometer_low_power ? 0b00010000 : 0b00000000
    );
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL9_XL,
        configuration->accelerometer_enable ? 0b00111000 : 0b00000000
    );

    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL2_G,
        (configuration->gyro_output_data_rate << 4) |
        (configuration->gyro_full_scale_range << 1)
    );
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL7_G,
        ((configuration->gyro_low_power ? 1 : 0) << 7) |
        (configuration->gyro_high_pass_filter << 4)
    );
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_CTRL10_C,
        configuration->gyro_enable ? 0b00111000 : 0b00000000
    );

    fd_lsm6dsl_write16(device, FD_LSM6DSL_REGISTER_FIFO_CTRL1,
        configuration->fifo_threshold & 0x0fff
    );
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_FIFO_CTRL3,
        (configuration->gyro_enable ? 0b00001000 /* no decimation */ : 0b00000000) |
        (configuration->accelerometer_enable ? 0b00000001 /* no decimation */ : 0b00000000)
    );
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_FIFO_CTRL5, 0x00);
    fd_lsm6dsl_fifo_flush(device);
    fd_lsm6dsl_write(device, FD_LSM6DSL_REGISTER_FIFO_CTRL5,
        (lsm6ds3_axis_count != 0) ? ((configuration->fifo_output_data_rate << 3) | 0b110 /* continuous  */ ) : 0
    );
}