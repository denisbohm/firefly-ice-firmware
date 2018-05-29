#include "fd_quiescent_test.h"

#include "fd_bq25120.h"
#include "fd_delay.h"
#include "fd_gpio.h"
#include "fd_i2cm.h"
#include "fd_lsm6dsl.h"
#include "fd_spi_flash.h"
#include "fd_spim.h"

#include "fd_nrf5.h"

#include <stdbool.h>
#include <stdint.h>

void fd_quiescent_test_set_system_voltage(void) {
    fd_i2cm_bus_t buses[] = {
        {
            .instance = (uint32_t)NRF_TWIM0,
            .scl = { .port = 1, .pin = 12 },
            .sda = { .port = 1, .pin = 13 },
            .frequency = 100000
        },
    };
    fd_i2cm_bus_t *bus = &buses[0];
    
    fd_i2cm_device_t devices[] = {
        { .bus = bus, .address = 0x6a /* bq25120 7-bit address */ },
    };
    fd_i2cm_device_t *device = &devices[0];
    
    fd_i2cm_initialize(buses, 1, devices, 1);
    fd_i2cm_bus_enable(bus);
    float voltage = 3.2f;
    fd_bq25120_set_system_voltage(device, voltage);
    fd_i2cm_bus_disable(bus);
}

void fd_quiescent_test_spi_initialize(void) {
    fd_spim_bus_t buses[] = {
        {
            .instance = (uint32_t)NRF_SPIM1,
            .sclk = { .port = 0, .pin = 5 },
            .mosi = { .port = 0, .pin = 4 },
            .miso = { .port = 0, .pin = 7 },
            .frequency = 8000000,
            .mode = 3
        },
        {
            .instance = (uint32_t)NRF_SPIM2,
            .sclk = { .port = 1, .pin = 3 },
            .mosi = { .port = 1, .pin = 2 },
            .miso = { .port = 1, .pin = 1 },
            .frequency = 8000000,
            .mode = 3
        },
    };
    fd_spim_bus_t *flash_bus = &buses[0];
    fd_spim_bus_t *lsm6dsl_bus = &buses[1];
    uint32_t bus_count = 2;
    
    fd_spim_device_t devices[] = {
        { .bus = flash_bus, .csn = { .port = 0, .pin = 6 } },
        { .bus = lsm6dsl_bus, .csn = { .port = 1, .pin = 4 } },
    };
    fd_spim_device_t *flash_device = &devices[0];
    fd_spim_device_t *lsm6dsl_device = &devices[1];
    uint32_t device_count = 2;
    
    fd_spim_initialize(buses, bus_count, devices, device_count);

    fd_spim_bus_enable(flash_bus);
    uint32_t wake_delay_us = 30;
    fd_spi_flash_wake(flash_device, 30);
    fd_spi_flash_sleep(flash_device);
    fd_spim_bus_disable(flash_bus);

    fd_spim_bus_enable(lsm6dsl_bus);
    fd_lsm6dsl_configuration_t configuration = {
        .fifo_threshold = 32,
        .fifo_output_data_rate = FD_LSM6DSL_ODR_13_HZ,
        .accelerometer_output_data_rate = FD_LSM6DSL_ODR_13_HZ,
        .accelerometer_low_power = true,
        .accelerometer_full_scale_range = FD_LSM6DSL_XFS_2_G,
        .accelerometer_bandwidth_filter = FD_LSM6DSL_XBWF_50_HZ,
        .accelerometer_enable = false,
        .gyro_output_data_rate = FD_LSM6DSL_ODR_13_HZ,
        .gyro_low_power = true,
        .gyro_full_scale_range = FD_LSM6DSL_GFS_125_DPS,
        .gyro_high_pass_filter = FD_LSM6DSL_GHPF_DISABLED_HZ,
        .gyro_enable = false
    };
    fd_lsm6ds3_configure(lsm6dsl_device, &configuration);
    uint32_t disable_delay = 100;
    fd_delay_us(disable_delay);
    fd_spim_bus_disable(lsm6dsl_bus);
}

void fd_quiescent_test(void) {
    fd_quiescent_test_set_system_voltage();
    fd_quiescent_test_spi_initialize();

    while (true) {
        __WFI();
    }
}
