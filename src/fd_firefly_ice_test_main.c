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

#include <em_cmu.h>
#include <em_gpio.h>
#include <em_rtc.h>

void halt(void) {
    *(uint32_t *)0xE000EDF0 = 0xA05F0003; // DHCSR = DBGKEY | C_HALT | C_DEBUGEN;
}

extern uint32_t fd_log_get_count(void);
extern char *fd_log_get_message(void);

void fd_test_hfxo_initialize(void) {
    CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
    CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
    CMU_ClockEnable(cmuClock_HFPER, true);
}

void fd_test_rtc_initialize(void) {
    CMU_ClockEnable(cmuClock_CORELE, true);
    CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
    CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
    CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO);
    CMU_ClockEnable(cmuClock_RTC, true);
    CMU_ClockEnable(cmuClock_CORELE, true);
    // output 32kHz clock (for nRF8001)
    CMU->CTRL = (CMU->CTRL & ~_CMU_CTRL_CLKOUTSEL1_MASK) | CMU_CTRL_CLKOUTSEL1_LFXO;
    CMU->ROUTE = CMU_ROUTE_LOCATION_LOC0 | CMU_ROUTE_CLKOUT1PEN;

    RTC_CompareSet(0, 65535); // 2 s
    RTC_CounterReset();
    RTC_Init_TypeDef init = RTC_INIT_DEFAULT;
    RTC_Init(&init);
}

uint32_t fd_test_rtc(void) {
    RTC_CounterReset();
    fd_delay_ms(100);
    uint32_t delta = RTC_CounterGet();
    return delta;
}

int main(void) {
    fd_processor_initialize();
    fd_processor_wake();

    fd_test_hfxo_initialize();

    fd_test_rtc_initialize();
    fd_test_rtc();

    GPIO_PinOutClear(NRF_RESETN_PORT_PIN);
    fd_delay_ms(100);
    GPIO_PinOutSet(NRF_RESETN_PORT_PIN);
    fd_delay_ms(100); // wait for nRF8001 to come out of reset (62ms)
    GPIO_PinInGet(NRF_RDYN_PORT_PIN);

    GPIO_PinOutClear(LED0_PORT_PIN);
    GPIO_PinOutSet(LED0_PORT_PIN);

    fd_log_initialize();
    char *message __attribute__((unused)) = fd_log_get_message();
    fd_event_initialize();

    bool is_charging __attribute__ ((unused)) = !GPIO_PinInGet(CHG_STAT_PORT_PIN);

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
    fd_lp55231_set_led_pwm(1, 255);
    float voltage __attribute__ ((unused)) = fd_lp55231_test_led(1);
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
    int16_t ax, ay, az;
    fd_lis3dh_read(&ax, &ay, &az);

    fd_log_get_count();

    halt();
    return 0;
}