#include "fd_quiescent_test.h"

#include "fd_bq25120.h"
#include "fd_delay.h"
#include "fd_gpio.h"
#include "fd_i2cm.h"
#include "fd_lsm6dsl.h"
#include "fd_pwm.h"
#include "fd_spi_flash.h"
#include "fd_spim.h"

#include "fd_nrf5.h"

#include <stdbool.h>
#include <stdint.h>

void fd_quiescent_test_button_initialize(void) {
    fd_gpio_t buttons[] = {
        { .port = 1, .pin = 11 },
        { .port = 1, .pin =  0 },
        { .port = 0, .pin = 26 },
        { .port = 1, .pin =  8 },
        { .port = 0, .pin = 27 },
    };
    fd_gpio_configure_input_pull_up(buttons[0]);
    const int count = sizeof(buttons) / sizeof(buttons[0]);
    for (int i = 1; i < count; ++i) {
        fd_gpio_configure_input_pull_up(buttons[i]);
    }
}

float battery_voltage = 0.0f;

void fd_quiescent_test_set_system_voltage(void) {
    // wait for BQ25120A startup
    fd_delay_ms(1000); // !!! 70 works, 65 does not.  TI says wait for twait1 min/0.08s, max/1.0s. -denis

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

    fd_gpio_t cdn = { .port = 1, .pin = 15 };
    fd_gpio_configure_output(cdn);
    fd_gpio_set(cdn, true);
    fd_delay_us(100);

    float voltage = 3.2f;
    bool result = fd_bq25120_set_system_voltage(device, voltage);
    fd_delay_us(100);

    result = fd_bq25120_read_battery_voltage(device, &battery_voltage);

    fd_gpio_set(cdn, false);

    fd_i2cm_bus_disable(bus);
}

fd_spi_flash_information_t information;
uint8_t who_am_i;
uint8_t ctrl1_xl;

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

#if 1
    fd_spim_bus_enable(flash_bus);
    uint32_t wake_delay_us = 30;
    fd_spi_flash_wake(flash_device, 30);
    //
    fd_spi_flash_get_information(flash_device, &information);
    //
    fd_spi_flash_sleep(flash_device);
    fd_spim_bus_disable(flash_bus);
#endif

    fd_spim_bus_enable(lsm6dsl_bus);
#if 1
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
#else
    fd_lsm6dsl_write(lsm6dsl_device, FD_LSM6DSL_REGISTER_CTRL3_C, 0b01010100); // block data update, int1/2 open drain, address automatically incremented
#endif

    //
    who_am_i = fd_lsm6dsl_read(lsm6dsl_device, FD_LSM6DSL_REGISTER_WHO_AM_I);
    ctrl1_xl = fd_lsm6dsl_read(lsm6dsl_device, FD_LSM6DSL_REGISTER_CTRL1_XL);
    //
    uint32_t disable_delay = 100;
    fd_delay_us(disable_delay);
    fd_spim_bus_disable(lsm6dsl_bus);
}

void fd_quiescent_vibrate(void) {
    const fd_pwm_module_t aw_pwm_modules[] = {
        {
            .instance = 0x4001C000, // NRF_PWM0
            .frequency = 32000.0f
        },
    };
    const fd_pwm_channel_t aw_pwm_channels[] = {
        {
            .module = &aw_pwm_modules[0],
            .instance = 0,
            .gpio = {
                .port = 0,
                .pin = 20
            }
        },
    };
    fd_pwm_initialize(aw_pwm_modules, 1);
    fd_pwm_module_enable(&aw_pwm_modules[0]);
    fd_pwm_channel_start(&aw_pwm_channels[0], 0.5f);
}

uint32_t loop_count = 0;

void fd_quiescent_test(void) {
    fd_gpio_t nreset = { .port = 0, .pin = 18 };
    fd_gpio_configure_input_pull_up(nreset);
    fd_gpio_t cdn = { .port = 1, .pin = 15 };
    fd_gpio_configure_output(cdn);
    fd_gpio_set(cdn, false);
    fd_gpio_t bq_npg = { .port = 1, .pin = 10 };
    fd_gpio_configure_input_pull_up(bq_npg);
    fd_gpio_t bq_int = { .port = 0, .pin = 3 };
    fd_gpio_configure_input(bq_int);
    fd_gpio_t bq_lsctrl = { .port = 1, .pin = 14 };
    fd_gpio_configure_output(bq_lsctrl);
    fd_gpio_set(bq_lsctrl, true);
    fd_gpio_t v5_en = { .port = 0, .pin = 8 };
    fd_gpio_configure_output(v5_en);
    fd_gpio_set(v5_en, false);

    fd_gpio_t motor_en = { .port = 0, .pin = 20 };
    fd_gpio_configure_output(motor_en);
    fd_gpio_set(motor_en, false);

    fd_gpio_t imu_int1 = { .port = 1, .pin = 6 };
    fd_gpio_configure_input_pull_up(imu_int1);
    fd_gpio_t imu_int2 = { .port = 1, .pin = 5 };
    fd_gpio_configure_input_pull_up(imu_int2);

    fd_quiescent_test_button_initialize();
    fd_quiescent_test_set_system_voltage();
    fd_quiescent_test_spi_initialize();

    fd_quiescent_vibrate();

    while (true) {
        ++loop_count;
        // Wait for an event.
        __WFE();
        // Clear the internal event register.
        __SEV();
        __WFE();
    }
}
