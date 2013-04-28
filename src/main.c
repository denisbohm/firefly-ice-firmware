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
#include "fd_spi0.h"
#include "fd_spi1.h"
#include "fd_tca6507.h"
#include "fd_usb.h"

#include <em_cmu.h>
#include <em_gpio.h>
#include <em_wdog.h>

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

uint32_t led_n = 0;

void set_led(uint32_t n) {
    if (led_n == n) {
        return;
    }
    if (n < led_n) {
        n = led_n - 1;
    } else
    if (n > led_n) {
        n = led_n + 1;
    }
    switch (led_n) {
        case 1: GPIO_PinOutSet(LED1_PORT_PIN); break;
        case 2: GPIO_PinOutSet(LED2_PORT_PIN); break;
        case 3: GPIO_PinOutSet(LED3_PORT_PIN); break;
        case 4: GPIO_PinOutSet(LED4_PORT_PIN); break;
        case 5: GPIO_PinOutSet(LED5_PORT_PIN); break;
        case 6: GPIO_PinOutSet(LED6_PORT_PIN); break;
        case 7: GPIO_PinOutSet(LED7_PORT_PIN); break;
        case 8: GPIO_PinOutSet(LED8_PORT_PIN); break;
    }
    led_n = n;
    switch (led_n) {
        case 1: GPIO_PinOutClear(LED1_PORT_PIN); break;
        case 2: GPIO_PinOutClear(LED2_PORT_PIN); break;
        case 3: GPIO_PinOutClear(LED3_PORT_PIN); break;
        case 4: GPIO_PinOutClear(LED4_PORT_PIN); break;
        case 5: GPIO_PinOutClear(LED5_PORT_PIN); break;
        case 6: GPIO_PinOutClear(LED6_PORT_PIN); break;
        case 7: GPIO_PinOutClear(LED7_PORT_PIN); break;
        case 8: GPIO_PinOutClear(LED8_PORT_PIN); break;
    }
}

#define SENSING_SIZE 25

fd_detour_source_t fd_sensing_detour_source;
uint8_t fd_sensing_buffer[SENSING_SIZE];

static
void fd_sensing_detour_supplier(uint32_t offset, uint8_t *data, uint32_t length) {
    memcpy(data, &fd_sensing_buffer[offset], length);
}

void fd_sensing_initialize(void) {
    fd_detour_source_initialize(&fd_sensing_detour_source);
}

void fd_sensing_push(
    fd_detour_source_collection_t *detour_source_collection,
    float ax, float ay, float az,
    float mx, float my, float mz
) {
    if (fd_detour_source_is_transferring(&fd_sensing_detour_source)) {
        return;
    }

    fd_binary_t binary;
    fd_binary_initialize(&binary, fd_sensing_buffer, SENSING_SIZE);
    fd_binary_put_uint8(&binary, 0xff);
    fd_binary_put_float32(&binary, ax);
    fd_binary_put_float32(&binary, ay);
    fd_binary_put_float32(&binary, az);
    fd_binary_put_float32(&binary, mx);
    fd_binary_put_float32(&binary, my);
    fd_binary_put_float32(&binary, mz);

    fd_detour_source_set(&fd_sensing_detour_source, fd_sensing_detour_supplier, SENSING_SIZE);
    fd_detour_source_collection_push(detour_source_collection, &fd_sensing_detour_source);
}

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

    GPIO_PinOutClear(LED1_PORT_PIN);
    fd_delay_ms(100);
    GPIO_PinOutSet(LED1_PORT_PIN);

    GPIO_PinOutClear(LED2_PORT_PIN);
    fd_delay_ms(100);
    GPIO_PinOutSet(LED2_PORT_PIN);

    GPIO_PinOutClear(LED3_PORT_PIN);
    fd_delay_ms(100);
    GPIO_PinOutSet(LED3_PORT_PIN);

    GPIO_PinOutClear(LED4_PORT_PIN);
    fd_delay_ms(100);
    GPIO_PinOutSet(LED4_PORT_PIN);

    GPIO_PinOutClear(LED5_PORT_PIN);
    fd_delay_ms(100);
    GPIO_PinOutSet(LED5_PORT_PIN);

    GPIO_PinOutClear(LED6_PORT_PIN);
    fd_delay_ms(100);
    GPIO_PinOutSet(LED6_PORT_PIN);

    GPIO_PinOutClear(LED7_PORT_PIN);
    fd_delay_ms(100);
    GPIO_PinOutSet(LED7_PORT_PIN);

    GPIO_PinOutClear(LED8_PORT_PIN);
    fd_delay_ms(100);
    GPIO_PinOutSet(LED8_PORT_PIN);

    fd_spi0_initialize();
    fd_lis3dh_initialize();

    fd_i2c1_initialize();
//    fd_i2c1_power_on();

    fd_tca6507_initialize();
    fd_tca6507_enable();
    fd_tca6507_test();
    fd_tca6507_set_color(false, false, false);

    fd_at24c512c_initialize();
    fd_at24c512c_test();

    fd_mag3110_initialize();

    fd_adc_initialize();

//    fd_spi1_power_on();
    fd_spi1_initialize();

    fd_nrf8001_initialize();
    fd_nrf8001_reset();
    fd_bluetooth_initialize();
    fd_sensing_initialize();
    int this_n = 0;
    float last_v = FLT_MAX;
    double time = 0.0;
    while (true) {
        WDOG_Feed();
        fd_bluetooth_step();
        fd_nrf8001_transfer();
        fd_usb_transfer();

        fd_time_t t = rtc_get_time();
        double now = t.seconds + t.microseconds * 1e-6;
        if ((now - time) < 0.020) {
            continue;
        }
        time = now;

// 31 µT - strength of Earth's magnetic field at 0° latitude (on the equator)
// 5 mT - the strength of a typical refrigerator magnet
        float mx, my, mz;
        fd_mag3110_read(&mx, &my, &mz);
        float teslas = sqrt(mx * mx + my * my + mz * mz);
        float v = 8.0f * (teslas - 0.000031f) / (0.005f - 0.000031f);
        float ax, ay, az;
        fd_lis3dh_read(&ax, &ay, &az);
        v = 8.0f * (ax - 0.0f);
        fd_sensing_push(&fd_bluetooth_detour_source_collection, ax, ay, az, mx, my, mz);
        int n = this_n;
        if (fabs(v - last_v) > 0.75) {
            int n = 1 + (int)v;
            if (n < 1) {
                n = 1;
            } else
            if (n > 8) {
                n = 8;
            }
            this_n = n;
            last_v = v;
        }
        set_led(n);

        if (fd_nrf8001_did_connect) {
            fd_tca6507_set_color(false, false, true);
        } else {
            fd_tca6507_set_color(false, false, false);
        }
    }

    return 0;
}