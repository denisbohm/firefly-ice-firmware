#include "fd_bq25120.h"
#include "fd_spi_flash.h"
#include "fd_i2cm.h"
#include "fd_lsm6dsl.h"
#include "fd_pwm.h"
#include "fd_quiescent_test.h"
#include "fd_spim.h"

#include "system_nrf52840.h"
#include "fd_nrf5.h"

static
void halt(void) {
    __asm("BKPT   #0");
}

bool fd_test_suite_crystal_test(uint32_t source, uint32_t timeout) {
    if (source == 0) {
        NRF_CLOCK->LFCLKSRC = 1; // 1 == external crystal
        NRF_CLOCK->TASKS_LFCLKSTART = 1;
        for (uint32_t i = 0; i < timeout; ++i) {
            if (NRF_CLOCK->EVENTS_LFCLKSTARTED != 0) {
                return true;
            }
        }
    }

    if (source == 1) {
        NRF_CLOCK->TASKS_HFCLKSTART = 1;
        for (uint32_t i = 0; i < timeout; ++i) {
            if (NRF_CLOCK->EVENTS_HFCLKSTARTED != 0) {
                return true;
            }
        }
    }

    return false;
}

void main(void) {
    void* tasks[] = {
        halt,

        SystemInit,

        fd_gpio_configure_default,
        fd_gpio_configure_output,
        fd_gpio_configure_output_open_drain,
        fd_gpio_configure_output_open_drain_pull_up,
        fd_gpio_configure_output_open_source_pull_down,
        fd_gpio_configure_input,
        fd_gpio_configure_input_pull_up,
        fd_gpio_configure_input_pull_down,
        fd_gpio_set,
        fd_gpio_get,

        fd_i2cm_initialize,
        fd_i2cm_bus_enable,
        fd_i2cm_bus_disable,
        fd_i2cm_bus_is_enabled,
        fd_i2cm_device_io,
        fd_i2cm_bus_wait,

        fd_spim_initialize,
        fd_spim_bus_enable,
        fd_spim_bus_disable,
        fd_spim_bus_is_enabled,
        fd_spim_device_select,
        fd_spim_device_deselect,
        fd_spim_device_is_selected,
        fd_spim_bus_io,
        fd_spim_bus_wait,

        fd_bq25120_read,
        fd_bq25120_write,
        fd_bq25120_set_system_voltage,
        fd_bq25120_read_battery_voltage,

        fd_lsm6dsl_read,
        fd_lsm6dsl_write,
        fd_lsm6ds3_configure,
        fd_lsm6dsl_fifo_flush,
        fd_lsm6dsl_read_fifo_word_count,
        fd_lsm6dsl_read_fifo_samples,

        fd_spi_flash_wake,
        fd_spi_flash_sleep,
        fd_spi_flash_get_information,
        fd_spi_flash_enable_write,
        fd_spi_flash_wait_while_busy,
        fd_spi_flash_chip_erase,
        fd_spi_flash_erase_sector,
        fd_spi_flash_write_page,
        fd_spi_flash_read,

        fd_pwm_initialize,
        fd_pwm_module_enable,
        fd_pwm_module_disable,
        fd_pwm_channel_start,
        fd_pwm_channel_is_running,
        fd_pwm_channel_stop,

        fd_quiescent_test,

        fd_test_suite_crystal_test,
    };

    fd_quiescent_test();
}