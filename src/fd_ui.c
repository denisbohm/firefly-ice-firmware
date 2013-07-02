#include "fd_bluetooth.h"
#include "fd_processor.h"
#include "fd_lp55231.h"
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
        case 1: GPIO_PinOutSet(LED0_PORT_PIN); break;
        case 2: GPIO_PinOutSet(LED4_PORT_PIN); break;
    }
    led_n = n;
    switch (led_n) {
        case 1: GPIO_PinOutClear(LED0_PORT_PIN); break;
        case 2: GPIO_PinOutClear(LED4_PORT_PIN); break;
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
        if (n > 2) {
            n = 2;
        }
        this_n = n;
        last_v = v;
    }
    set_led(n);

    if (fd_nrf8001_did_connect) {
        fd_lp55231_set_led_pwm(0, 0xff);
    } else {
        fd_lp55231_set_led_pwm(0, 0xff);
    }
}