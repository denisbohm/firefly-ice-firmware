#include "fd_activity.h"
#include "fd_adc.h"
#include "fd_at24c512c.h"
#include "fd_binary.h"
#include "fd_bluetooth.h"
#include "fd_detour.h"
#include "fd_event.h"
#include "fd_i2c1.h"
#include "fd_lis3dh.h"
#include "fd_log.h"
#include "fd_mag3110.h"
#include "fd_nrf8001.h"
#include "fd_processor.h"
#include "fd_rtc.h"
#include "fd_sensing.h"
#include "fd_spi0.h"
#include "fd_spi1.h"
#include "fd_storage_buffer.h"
#include "fd_tca6507.h"
#include "fd_ui.h"
#include "fd_usb.h"

#include <em_cmu.h>
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
    wdog_init.lock = true;
    WDOG_Init(&wdog_init);
#endif

    fd_spi1_power_on();
    fd_i2c1_power_on();

    fd_event_initialize();
    fd_log_initialize();
    fd_rtc_initialize();
    fd_usb_initialize();
    fd_adc_initialize();

    fd_spi0_initialize();
    fd_lis3dh_initialize();

    fd_i2c1_initialize();
//    fd_i2c1_power_on();
    //
    fd_tca6507_initialize();
    fd_tca6507_enable();
    fd_tca6507_test();
    fd_tca6507_set_color(false, false, false);
    //
    fd_at24c512c_initialize();
    fd_at24c512c_test();
    //
    fd_mag3110_initialize();

//    fd_spi1_power_on();
    fd_spi1_initialize();
    //
    fd_nrf8001_initialize();
    fd_nrf8001_reset();
    fd_bluetooth_initialize();

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
//        fd_sensing_push(&fd_bluetooth_detour_source_collection, ax, ay, az, mx, my, mz);

        if ((now - last_activity_time) >= 10.0) {
            float activity = fd_activity_value(10.0);
            fd_storage_buffer_add_time_series(&activity_storage_buffer, time.seconds, 10, activity);
            fd_activity_start();
            last_activity_time = now;
        }
        fd_activity_accumulate(ax, ay, az);

        fd_ui_update(ax);
    }

    return 0;
}