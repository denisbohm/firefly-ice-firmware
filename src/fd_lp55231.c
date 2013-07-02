#include "fd_i2c1.h"
#include "fd_lp55231.h"
#include "fd_processor.h"

#include <em_gpio.h>

#define WRITE_ADDRESS 0x64
#define READ_ADDRESS 0x65

#define D1_PWM 0x16
#define D2_PWM 0x17
#define D3_PWM 0x18
#define D4_PWM 0x19
#define D5_PWM 0x1a
#define D6_PWM 0x1b
#define D7_PWM 0x1c
#define D8_PWM 0x1d
#define D9_PWM 0x1e

void fd_lp55231_initialize(void) {
}

void fd_lp55231_wake(void) {
    GPIO_PinOutSet(LED_EN_PORT_PIN);
}

void fd_lp55231_sleep(void) {
    GPIO_PinOutClear(LED_EN_PORT_PIN);
}

void fd_lp55231_set_led_pwm(uint8_t led, uint8_t pwm) {
    fd_i2c1_register_write(WRITE_ADDRESS, D1_PWM + led, pwm);
}