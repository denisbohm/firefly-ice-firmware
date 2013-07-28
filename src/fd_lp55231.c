#include "fd_i2c1.h"
#include "fd_log.h"
#include "fd_lp55231.h"
#include "fd_processor.h"

#include <em_gpio.h>

#define ADDRESS 0x64

#define ENGINE_CNTRL1 0x00
#define ENGINE_CNTRL1_CHIP_EN 0x40

#define D1_PWM 0x16
#define D2_PWM 0x17
#define D3_PWM 0x18
#define D4_PWM 0x19
#define D5_PWM 0x1a
#define D6_PWM 0x1b
#define D7_PWM 0x1c
#define D8_PWM 0x1d
#define D9_PWM 0x1e

#define D1_CURRENT_CONTROL 0x26

#define MISC 0x36
#define MISC_INT_CLK_EN 0x01
#define MISC_CP_MODE_AUTOMATIC 0x18

void fd_lp55231_initialize(void) {
}

void fd_lp55231_wake(void) {
    GPIO_PinOutSet(LED_EN_PORT_PIN);
    fd_delay_us(500);
    bool result = fd_i2c1_register_write(ADDRESS, ENGINE_CNTRL1, ENGINE_CNTRL1_CHIP_EN);
    if (!result) {
        fd_log_assert_fail("");
    }
    result = fd_i2c1_register_write(ADDRESS, MISC, MISC_INT_CLK_EN | MISC_CP_MODE_AUTOMATIC);
    if (!result) {
        fd_log_assert_fail("");
    }
}

void fd_lp55231_sleep(void) {
    bool result = fd_i2c1_register_write(ADDRESS, MISC, 0x00);
    if (!result) {
        fd_log_assert_fail("");
    }
    result = fd_i2c1_register_write(ADDRESS, ENGINE_CNTRL1, 0x00);
    if (!result) {
        fd_log_assert_fail("");
    }
    GPIO_PinOutClear(LED_EN_PORT_PIN);
}

void fd_lp55231_set_led_pwm(uint8_t led, uint8_t pwm) {
    bool result = fd_i2c1_register_write(ADDRESS, D1_PWM + led, pwm);
    if (!result) {
        fd_log_assert_fail("");
    }
}