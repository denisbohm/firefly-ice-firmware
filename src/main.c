#include "fd_activity.h"
#include "fd_adc.h"
#include "fd_binary.h"
#include "fd_bluetooth.h"
#include "fd_control.h"
#include "fd_detour.h"
#include "fd_event.h"
#include "fd_hal_processor.h"
#include "fd_hal_reset.h"
#include "fd_hal_rtc.h"
#include "fd_hal_system.h"
#include "fd_hal_ui.h"
#include "fd_i2c1.h"
#include "fd_lis3dh.h"
#include "fd_lock.h"
#include "fd_log.h"
#include "fd_lp55231.h"
#include "fd_main.h"
#include "fd_mag3110.h"
#include "fd_nrf8001.h"
#include "fd_pins.h"
#include "fd_power.h"
#include "fd_recognition.h"
#include "fd_sensing.h"
#include "fd_spi.h"
#include "fd_storage_buffer.h"
#include "fd_sync.h"
#include "fd_timer.h"
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

static fd_main_mode_t fd_main_mode;
static bool fd_main_sleep_when_bluetooth_is_asleep;
static bool fd_main_was_unplugged;

static
void main_sleep(void) {
    fd_sensing_sleep();
    fd_hal_ui_sleep();

    fd_usb_sleep();
    CMU->HFCORECLKEN0 |= (CMU_HFCORECLKEN0_USB | CMU_HFCORECLKEN0_USBC);

    fd_adc_sleep();
    fd_hal_rtc_sleep();
    fd_lis3dh_sleep();
//    fd_lp55231_sleep();
//    fd_lp55231_power_off();
    fd_mag3110_sleep();
//    fd_w25q16dw_sleep();

//    fd_spi_sleep(FD_SPI_BUS_0);
//    fd_spi_off(FD_SPI_BUS_0);
    fd_spi_sleep(FD_SPI_BUS_1);
    fd_i2c1_sleep();
//    fd_i2c1_power_off();

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
    fd_hal_rtc_wake();
    fd_lis3dh_wake();
//    fd_lp55231_power_on();
//    fd_lp55231_wake();
//    fd_mag3110_wake();
//    fd_w25q16dw_wake();

    fd_sensing_wake();
    fd_hal_ui_wake();
}

static
void main_enter_storage_mode(void) {
    fd_main_mode = fd_main_mode_storage;
    fd_main_sleep_when_bluetooth_is_asleep = true;
    fd_main_was_unplugged = false;

    fd_bluetooth_sleep();
}

static
void main_enter_run_mode(void) {
    fd_main_mode = fd_main_mode_run;
    fd_main_sleep_when_bluetooth_is_asleep = false;
    fd_main_was_unplugged = false;

    main_wake();

    fd_bluetooth_wake();
}

void fd_main_set_mode(fd_main_mode_t mode) {
    if (fd_main_mode == mode) {
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
    return fd_main_mode;
}

/*
void fd_main_usb_state_change(void) {
    if ((main_mode != fd_main_mode_run) && fd_usb_is_powered()) {
        fd_main_set_mode(fd_main_mode_run);
    }
}
*/

int main(void) {
    fd_hal_reset_initialize();
    fd_hal_processor_initialize();

    fd_hal_reset_start_watchdog();

    fd_main_mode = fd_main_mode_run;
    fd_main_sleep_when_bluetooth_is_asleep = false;
    fd_main_was_unplugged = false;

    fd_log_initialize();
    fd_event_initialize();
    fd_pins_events_initialize();
    fd_timer_initialize();
    fd_lock_initialize();
    fd_detour_startup_initialize();
    fd_control_initialize();

    GPIO_PinOutClear(LED0_PORT_PIN);
    GPIO_PinOutClear(LED4_PORT_PIN);
    GPIO_PinOutClear(LED5_PORT_PIN);
    GPIO_PinOutClear(LED6_PORT_PIN);

    fd_hal_reset_feed_watchdog();
    
    fd_hal_rtc_initialize();
    fd_adc_initialize();
    fd_usb_initialize();
    fd_hal_system_initialize();

    fd_hal_reset_feed_watchdog();

    fd_i2c1_initialize();
    // initialize devices on i2c1 powered bus
    fd_i2c1_power_on();
    //
    fd_lp55231_initialize();
    fd_lp55231_power_on();
    //
    fd_mag3110_initialize();

    fd_hal_reset_feed_watchdog();

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
    uint8_t *name;
    uint32_t length = fd_control_get_name(&name);
    if (length > 0) {
        fd_bluetooth_set_name(name, length);
    }

    // initialize devices on spi0 powered bus
//    fd_spi_on(FD_SPI_BUS_0);
//    fd_spi_wake(FD_SPI_BUS_0);
    //
    fd_w25q16dw_initialize();

    fd_storage_initialize();
    fd_storage_buffer_collection_initialize();
//    fd_log_set_storage(true);

    fd_hal_reset_feed_watchdog();

    fd_hal_ui_initialize();
    fd_sync_initialize();
    fd_activity_initialize();
    fd_sensing_initialize();
    fd_sensing_wake();
    fd_recognition_initialize();

    fd_hal_reset_feed_watchdog();

    fd_usb_wake();
    fd_power_initialize();

//    fd_event_add_callback(FD_EVENT_USB_STATE, fd_main_usb_state_change);

    while (true) {
        fd_event_process();

        // go to sleep when bluetooth is asleep and usb can be put to sleep
        if (fd_main_sleep_when_bluetooth_is_asleep && fd_bluetooth_is_asleep()) {
            fd_main_sleep_when_bluetooth_is_asleep = false;
            fd_main_was_unplugged = !fd_usb_is_powered();
            main_sleep();
        }
        // wake up when usb was unplugged and now has been plugged in
        if ((fd_main_mode == fd_main_mode_storage) && !fd_main_sleep_when_bluetooth_is_asleep) {
            if (!fd_main_was_unplugged && ((USB->IF & USB_IF_VREGOSL) != 0)) {
                fd_main_was_unplugged = true;
            }
            if (fd_main_was_unplugged && ((USB->IF & USB_IF_VREGOSH) != 0)) {
                fd_main_set_mode(fd_main_mode_run);
            }
        }
    }

    return 0;
}
