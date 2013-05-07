#include "fd_bluetooth.h"
#include "fd_processor.h"
#include "fd_tca6507.h"
#include "fd_ui.h"

#include <em_gpio.h>

#include <float.h>
#include <math.h>
#include <stdint.h>

static int this_n;
static float last_v;
static uint32_t led_n;

void fd_ui_initialize(void) {
    this_n = 0;
    last_v = FLT_MAX;
    led_n = 0;
}

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

void fd_ui_update(float ax) {
    float v = 8.0f * (ax - 0.0f);
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