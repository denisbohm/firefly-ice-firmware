#include "fd_adc.h"
#include "fd_i2c1.h"
#include "fd_lis3dh.h"
#include "fd_lp55231.h"
#include "fd_mag3110.h"
#include "fd_processor.h"
#include "fd_spi.h"
#include "fd_w25q16dw.h"

#include "fd_event.h"
#include "fd_log.h"

#include <em_gpio.h>

void halt(void) {
    *(uint32_t *)0xE000EDF0 = 0xA05F0003; // DHCSR = DBGKEY | C_HALT | C_DEBUGEN;
}

extern uint32_t fd_log_get_count(void);
extern char *fd_log_get_message(void);

int main(void) {
    fd_processor_initialize();

    GPIO_PinOutClear(LED0_PORT_PIN);
    GPIO_PinOutSet(LED0_PORT_PIN);

    fd_log_initialize();
    char *message __attribute__((unused)) = fd_log_get_message();
    fd_event_initialize();

    fd_adc_initialize();
    fd_adc_start(fd_adc_channel_temperature, true);
    float temperature __attribute__ ((unused)) = fd_adc_get_temperature();
    fd_adc_start(fd_adc_channel_battery_voltage, true);
    float battery_voltage __attribute__ ((unused)) = fd_adc_get_battery_voltage();
    fd_adc_start(fd_adc_channel_charge_current, true);
    float charge_current __attribute__ ((unused)) = fd_adc_get_charge_current();

    fd_i2c1_initialize();
    fd_i2c1_power_on();
    //
    fd_lp55231_initialize();
    fd_lp55231_power_on();
    fd_lp55231_wake();
    fd_lp55231_set_led_pwm(0, 255);
    //
    fd_mag3110_initialize();
    fd_mag3110_wake();
    fd_delay_ms(250);
    float mx, my, mz;
    fd_mag3110_read(&mx, &my, &mz);

    fd_spi_initialize();
    //
    // initialize devices on spi1 bus
    fd_spi_on(FD_SPI_BUS_1);
    fd_spi_wake(FD_SPI_BUS_1);
    // initialize devices on spi0 powered bus
//    fd_spi_on(FD_SPI_BUS_0);
//    fd_spi_wake(FD_SPI_BUS_0);
    //
    fd_w25q16dw_initialize();
    fd_w25q16dw_wake();
    uint32_t address = 0;
    fd_w25q16dw_enable_write();
    fd_w25q16dw_erase_sector(address);
    uint8_t write_data[2] = {0x01, 0x02};
    fd_w25q16dw_enable_write();
    fd_w25q16dw_write_page(address, write_data, sizeof(write_data));
    uint8_t read_data[2] = {0x00, 0x00};
    fd_w25q16dw_read(address, read_data, sizeof(read_data));

    fd_w25q16dw_enable_write();
    fd_w25q16dw_chip_erase();
    fd_w25q16dw_wait_while_busy();

    fd_lis3dh_initialize();
    fd_lis3dh_wake();
    float ax, ay, az;
    fd_lis3dh_read(&ax, &ay, &az);

    fd_log_get_count();

    halt();
    return 0;
}