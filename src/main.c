#include "fd_activity.h"
#include "fd_adc.h"
#include "fd_binary.h"
#include "fd_bluetooth.h"
#include "fd_control.h"
#include "fd_detour.h"
#include "fd_event.h"
#include "fd_i2c1.h"
#include "fd_lis3dh.h"
#include "fd_indicator.h"
#include "fd_log.h"
#include "fd_lp55231.h"
#include "fd_main.h"
#include "fd_mag3110.h"
#include "fd_nrf8001.h"
#include "fd_power.h"
#include "fd_processor.h"
#include "fd_reset.h"
#include "fd_rtc.h"
#include "fd_sensing.h"
#include "fd_spi.h"
#include "fd_storage_buffer.h"
#include "fd_sync.h"
#include "fd_timer.h"
#include "fd_ui.h"
#include "fd_usb.h"
#include "fd_w25q16dw.h"

#include <em_cmu.h>
#include <em_emu.h>
#include <em_gpio.h>
#include <em_wdog.h>

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

static fd_main_mode_t main_mode;
static bool main_sleep_when_bluetooth_is_asleep;

static
void main_sleep(void) {
    fd_sensing_sleep();
    fd_indicator_sleep();

    fd_usb_sleep();
    fd_adc_sleep();
    fd_rtc_sleep();
    fd_lis3dh_sleep();
    fd_lp55231_sleep();
    fd_mag3110_sleep();
    fd_w25q16dw_sleep();

//    fd_spi_sleep(FD_SPI_BUS_0);
//    fd_spi_off(FD_SPI_BUS_0);
    fd_spi_sleep(FD_SPI_BUS_1);
    fd_i2c1_sleep();
    fd_i2c1_power_off();

    USB->IFC = USB_IFC_VREGOSH | USB_IFC_VREGOSL;
}

static
void main_wake(void) {
    fd_i2c1_power_on();
    fd_i2c1_wake();
    fd_spi_wake(FD_SPI_BUS_1);
//    fd_spi_on(FD_SPI_BUS_0);
//    fd_spi_wake(FD_SPI_BUS_0);

    fd_usb_wake();
    fd_adc_wake();
    fd_rtc_wake();
    fd_lis3dh_wake();
    fd_lp55231_wake();
    fd_mag3110_wake();
    fd_w25q16dw_wake();

    fd_indicator_wake();
    fd_sensing_wake();
}

static
void main_enter_storage_mode(void) {
    main_mode = fd_main_mode_storage;
    main_sleep_when_bluetooth_is_asleep = true;

    fd_bluetooth_sleep();
}

static
void main_enter_run_mode(void) {
    main_mode = fd_main_mode_run;
    main_sleep_when_bluetooth_is_asleep = false;

    main_wake();

    fd_bluetooth_wake();
}

void fd_main_set_mode(fd_main_mode_t mode) {
    if (main_mode == mode) {
        return;
    }
    if (mode == fd_main_mode_storage) {
        main_enter_storage_mode();
    } else
    if (mode == fd_main_mode_run) {
        main_enter_run_mode();
    }
}

fd_main_mode_t fd_main_get_mode(void) {
    return main_mode;
}

/*
void fd_main_usb_state_change(void) {
    if ((main_mode != fd_main_mode_run) && fd_usb_is_powered()) {
        fd_main_set_mode(fd_main_mode_run);
    }
}
*/

int main(void) {
    fd_reset_initialize();
    fd_processor_initialize();

/*
#ifdef DEBUG
#warning debug is defined - watchdog is not enabled
#else
    CMU_ClockEnable(cmuClock_CORELE, true);
    WDOG_Init_TypeDef wdog_init = WDOG_INIT_DEFAULT;
    wdog_init.perSel = wdogPeriod_8k;
    wdog_init.swoscBlock = true;
//    wdog_init.lock = true;
    WDOG_Init(&wdog_init);
#endif
*/

    main_mode = fd_main_mode_run;
    main_sleep_when_bluetooth_is_asleep = false;

    fd_log_initialize();
    fd_event_initialize();
    fd_timer_initialize();
    fd_control_initialize();

    GPIO_PinOutClear(LED0_PORT_PIN);
    GPIO_PinOutClear(LED4_PORT_PIN);
    GPIO_PinOutClear(LED5_PORT_PIN);
    GPIO_PinOutClear(LED6_PORT_PIN);

    fd_rtc_initialize();
    fd_adc_initialize();
    fd_usb_initialize();

    fd_i2c1_initialize();
    // initialize devices on i2c1 powered bus
    fd_i2c1_power_on();
    //
    fd_lp55231_initialize();
    fd_lp55231_wake();

    //
    fd_mag3110_initialize();

    fd_spi_initialize();

    // initialize devices on spi1 bus
    fd_spi_on(FD_SPI_BUS_1);
    fd_spi_wake(FD_SPI_BUS_1);
    //
    fd_lis3dh_initialize();
    fd_lis3dh_wake();
    //
    fd_nrf8001_initialize();
    fd_bluetooth_reset();
    fd_bluetooth_initialize();

    // initialize devices on spi0 powered bus
//    fd_spi_on(FD_SPI_BUS_0);
//    fd_spi_wake(FD_SPI_BUS_0);
    //
    fd_w25q16dw_initialize();

    fd_storage_initialize();
    fd_storage_buffer_collection_initialize();
//    fd_log_enable_storage(true);

    fd_power_initialize();

    fd_indicator_initialize();
    fd_indicator_wake();

    fd_indicator_set_usb(0xff, 0x00);
    fd_indicator_set_d0(0x00);
    fd_indicator_set_d1(0x00, 0x00, 0x00);
    fd_indicator_set_d2(0x00, 0x00, 0x00);
    fd_indicator_set_d3(0x00, 0x00, 0x00);
    fd_indicator_set_d4(0x00);
    fd_delay_ms(250);
    fd_indicator_set_usb(0xff, 0x00);
    fd_delay_ms(250);
    fd_indicator_set_usb(0x00, 0xff);
    fd_delay_ms(250);
    fd_indicator_set_usb(0x00, 0x00);
    fd_indicator_set_d0(0xff);
    fd_delay_ms(250);
    fd_indicator_set_d0(0x00);
    fd_indicator_set_d1(0xff, 0x00, 0x00);
    fd_delay_ms(250);
    fd_indicator_set_d1(0x00, 0xff, 0x00);
    fd_delay_ms(250);
    fd_indicator_set_d1(0x00, 0x00, 0xff);
    fd_delay_ms(250);
    fd_indicator_set_d1(0x00, 0x00, 0x00);
    fd_indicator_set_d2(0xff, 0x00, 0x00);
    fd_delay_ms(250);
    fd_indicator_set_d2(0x00, 0xff, 0x00);
    fd_delay_ms(250);
    fd_indicator_set_d2(0x00, 0x00, 0xff);
    fd_delay_ms(250);
    fd_indicator_set_d2(0x00, 0x00, 0x00);
    fd_indicator_set_d3(0xff, 0x00, 0x00);
    fd_delay_ms(250);
    fd_indicator_set_d3(0x00, 0xff, 0x00);
    fd_delay_ms(250);
    fd_indicator_set_d3(0x00, 0x00, 0xff);
    fd_delay_ms(250);
    fd_indicator_set_d3(0x00, 0x00, 0x00);
    fd_indicator_set_d4(0xff);
    fd_delay_ms(250);
    fd_indicator_set_d4(0x00);
    fd_delay_ms(250);

    fd_ui_initialize();
    fd_sync_initialize();
    fd_activity_initialize();
    fd_sensing_initialize();
    fd_sensing_wake();

    fd_usb_wake();

//    fd_event_add_callback(FD_EVENT_USB_STATE, fd_main_usb_state_change);

    while (true) {
        fd_event_process();

        if (main_sleep_when_bluetooth_is_asleep && fd_bluetooth_is_asleep()) {
            main_sleep_when_bluetooth_is_asleep = false;
            main_sleep();
        }
        if ((main_mode == fd_main_mode_storage) && (USB->IF & (USB_IF_VREGOSH | USB_IF_VREGOSL))) {
            fd_main_set_mode(fd_main_mode_run);
        }
    }

    return 0;
}