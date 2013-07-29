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
#include "fd_mag3110.h"
#include "fd_nrf8001.h"
#include "fd_power.h"
#include "fd_processor.h"
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

int main(void) {
    fd_processor_initialize();

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

    fd_log_initialize();
    fd_event_initialize();
    fd_timer_initialize();
    fd_control_initialize();

//    GPIO_PinOutClear(LED0_PORT_PIN);
//    GPIO_PinOutClear(LED4_PORT_PIN);
//    GPIO_PinOutClear(LED5_PORT_PIN);
//    GPIO_PinOutClear(LED6_PORT_PIN);

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
//    fd_lis3dh_wake();
    //
    fd_nrf8001_initialize();
    fd_nrf8001_reset();
    fd_bluetooth_initialize();

    // initialize devices on spi0 powered bus
    fd_spi_on(FD_SPI_BUS_0);
    fd_spi_wake(FD_SPI_BUS_0);
    //
    fd_w25q16dw_initialize();

    fd_storage_initialize();
    fd_storage_buffer_collection_initialize();
    fd_log_enable_storage(true);

    fd_power_initialize();

    fd_indicator_initialize();
    fd_indicator_wake();

    fd_ui_initialize();
    fd_sync_initialize();
    fd_activity_initialize();
    fd_sensing_initialize();
    fd_sensing_wake();

    fd_usb_wake();

#if 0
    // low power testing
    fd_usb_sleep();
    fd_adc_sleep();
    fd_rtc_sleep();
    fd_lis3dh_sleep();
    fd_lp55231_sleep();
    fd_mag3110_sleep();
//    fd_w25q16dw_sleep();

//    fd_lis3dh_set_sample_callback(acc_sample);
    bool led_state = 0;
    bool entering_sleep = false;
    bool entered_sleep = false;
    while (true) {
        WDOG_Feed();
        if (fd_bluetooth_is_asleep()) {
            if (!entered_sleep) {
                GPIO_PinOutSet(LED0_PORT_PIN);
                fd_spi_sleep(FD_SPI_BUS_0);
                fd_spi_off(FD_SPI_BUS_0);
                fd_spi_sleep(FD_SPI_BUS_1);
                fd_i2c1_sleep();
                fd_i2c1_power_off();
                entered_sleep = true;
            } else {
#if 1
                if (led_state) {
                    GPIO_PinOutClear(LED4_PORT_PIN);
                } else {
                    GPIO_PinOutSet(LED4_PORT_PIN);
                }
                led_state = !led_state;
#endif
//                fd_processor_sleep();
                uint32_t time = fd_rtc_get_seconds();
                while (fd_rtc_get_seconds() == time) {
                    EMU_EnterEM2(false);
                }
//                fd_processor_wake();
            }
        } else {
            if (fd_nrf8001_did_setup && !entering_sleep) {
                fd_bluetooth_sleep();
                entering_sleep = true;
            }
            fd_bluetooth_step();
            fd_nrf8001_transfer();
        }
    }
#endif

    while (true) {
        fd_event_process();
    }

    return 0;
}