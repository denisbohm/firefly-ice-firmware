#include "fd_activity.h"
#include "fd_adc.h"
#include "fd_binary.h"
#include "fd_bluetooth.h"
#include "fd_detour.h"
#include "fd_event.h"
#include "fd_i2c1.h"
#include "fd_lis3dh.h"
#include "fd_log.h"
#include "fd_lp55231.h"
#include "fd_mag3110.h"
#include "fd_nrf8001.h"
#include "fd_processor.h"
#include "fd_rtc.h"
#include "fd_sensing.h"
#include "fd_spi.h"
#include "fd_storage_buffer.h"
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

/*
static
void acc_sample(int16_t ix, int16_t iy, int16_t iz) {
    float x = ix / 16384.0f;
    float y = iy / 16384.0f;
    float z = iz / 16384.0f;
    fd_activity_accumulate(x, y, z);
}
*/

int main(void) {
    fd_processor_initialize();

#ifdef DEBUG
#warning debug is defined - watchdog is not enabled
#else
    CMU_ClockEnable(cmuClock_CORELE, true);
    WDOG_Init_TypeDef wdog_init = WDOG_INIT_DEFAULT;
    wdog_init.perSel = wdogPeriod_8k;
    wdog_init.swoscBlock = true;
    wdog_init.lock = true;
    WDOG_Init(&wdog_init);
#endif

    fd_rtc_initialize();

    GPIO_PinOutClear(LED0_PORT_PIN);
    GPIO_PinOutClear(LED4_PORT_PIN);
//    GPIO_PinOutClear(LED5_PORT_PIN);
    GPIO_PinOutClear(LED6_PORT_PIN);

    fd_event_initialize();
    fd_log_initialize();
    fd_usb_initialize();
    fd_adc_initialize();

    fd_i2c1_initialize();

    // initialize devices on i2c1 powered bus
    fd_i2c1_power_on();
    //
    fd_lp55231_initialize();
    fd_lp55231_wake();
    /*
    for (int i = 0; i < 9; ++i) {
        fd_lp55231_set_led_pwm(i, 127);
    }
    */
    //
    fd_mag3110_initialize();

    fd_spi_initialize();

    // initialize devices on spi1 bus
    fd_spi_on(FD_SPI_BUS_1);
    fd_spi_wake(FD_SPI_BUS_1);
    //
    fd_lis3dh_initialize();
    //
    fd_nrf8001_initialize();
    fd_nrf8001_reset();
    fd_bluetooth_initialize();

    // initialize devices on spi0 powered bus
    fd_spi_on(FD_SPI_BUS_0);
    fd_spi_wake(FD_SPI_BUS_0);
    //
    fd_w25q16dw_initialize();
    fd_w25q16dw_test();

    fd_storage_buffer_collection_initialize();

    fd_ui_initialize();
    fd_sensing_initialize();
    fd_activity_initialize();
    {
        float ax, ay, az;
        fd_lis3dh_read(&ax, &ay, &az);
        fd_activity_prime(ax, ay, az);
    }
    fd_storage_buffer_t activity_storage_buffer;
    fd_storage_buffer_initialize(&activity_storage_buffer, FD_STORAGE_TYPE('F', 'D', 'V', 'M'));
    fd_storage_buffer_collection_push(&activity_storage_buffer);

    // low power testing
    fd_usb_sleep();
    fd_adc_sleep();
//    fd_rtc_sleep();
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
                EMU_EnterEM2(false);
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

#if 0
    fd_time_t time = rtc_get_time();
    double now = time.seconds + time.microseconds * 1e-6;
    double last_sensing_time = now;
    double last_activity_time = now;
    while (true) {
        WDOG_Feed();
        fd_bluetooth_step();
        fd_nrf8001_transfer();
        fd_usb_transfer();

        time = rtc_get_time();
        double now = time.seconds + time.microseconds * 1e-6;
        if ((now - last_sensing_time) < 0.020) {
            continue;
        }
        last_sensing_time = now;

        float ax, ay, az;
        fd_lis3dh_read(&ax, &ay, &az);
        float mx, my, mz;
        fd_mag3110_read(&mx, &my, &mz);
        fd_sensing_push(&fd_bluetooth_detour_source_collection, ax, ay, az, mx, my, mz);

        if ((now - last_activity_time) >= 10.0) {
            float activity = fd_activity_value(10.0);
            fd_storage_buffer_add_time_series(&activity_storage_buffer, time.seconds, 10, activity);
            fd_activity_start();
            last_activity_time = now;
        }
        fd_activity_accumulate(ax, ay, az);

        fd_ui_update(ax);
    }
#endif

    return 0;
}