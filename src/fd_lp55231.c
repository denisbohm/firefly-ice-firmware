#include "fd_i2c1.h"
#include "fd_hal_processor.h"
#include "fd_log.h"
#include "fd_lp55231.h"
#include "fd_pins.h"

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
#define MISC_CLK_DET_EN 0x02
#define MISC_PWM_PS_EN 0x04
#define MISC_CP_MODE_AUTOMATIC 0x18
#define MISC_POWERSAVE_EN 0x20
#define MISC_EN_AUTO_INCR 0x40
#define MISC_VARIABLE_D_SEL 0x80

#define RESET_REGISTER 0x3d
#define RESET_VALUE 0xff

void fd_lp55231_initialize(void) {
}

void fd_lp55231_power_on(void) {
    GPIO_PinOutSet(LED_EN_PORT_PIN);

#define RC 40
#define GC 10
#define BC 80
    const uint8_t current_controls[] = {
        BC /* D3.B */, GC /* D3.G */, BC /* D1.B */,
        GC /* D2.G */, BC /* D1.B */, GC /* D1.G */,
        RC /* D3.R */, RC /* D2.R */, RC /* D1.R */,
    };
    bool result = fd_i2c1_register_write_bytes(ADDRESS, D1_CURRENT_CONTROL, (uint8_t *)current_controls, sizeof(current_controls));
    if (!result) {
        fd_log_assert_fail("");
    }

//    fd_delay_us(500); // power on delay for analog blocks (charge pump, etc)
}

void fd_lp55231_wake(void) {
    bool result = fd_i2c1_register_write(ADDRESS, ENGINE_CNTRL1, ENGINE_CNTRL1_CHIP_EN);
    if (!result) {
        fd_log_assert_fail("");
    }
    result = fd_i2c1_register_write(ADDRESS, MISC, MISC_INT_CLK_EN | MISC_CP_MODE_AUTOMATIC | MISC_EN_AUTO_INCR);
    if (!result) {
        fd_log_assert_fail("");
    }
}

void fd_lp55231_sleep(void) {
    for (int i = 1; i <= 9; ++i) {
        fd_lp55231_set_led_pwm(i, 0);
    }
    bool result = fd_i2c1_register_write(ADDRESS, MISC, MISC_EN_AUTO_INCR);
    if (!result) {
        fd_log_assert_fail("");
    }
    result = fd_i2c1_register_write(ADDRESS, ENGINE_CNTRL1, 0x00);
    if (!result) {
        fd_log_assert_fail("");
    }
}

void fd_lp55231_power_off(void) {
    /*
    // !!! reset before power off, just in case we power back on before the voltage has dropped low enough for a power on reset -denis
    bool result = fd_i2c1_register_write(ADDRESS, RESET_REGISTER, RESET_VALUE);
    if (!result) {
        fd_log_assert_fail("");
    }
    */

    GPIO_PinOutClear(LED_EN_PORT_PIN);
}

void fd_lp55231_set_led_pwm(uint8_t led, uint8_t pwm) {
    bool result = fd_i2c1_register_write(ADDRESS, D1_PWM + led - 1, pwm);
    if (!result) {
        fd_log_assert_fail("");
    }
}

#define LED_TEST_CONTROL 0x41
#define EN_LEDTEST_ADC 0x80
#define EN_LEDTEST_INT 0x40
#define CONTINUOUS_CONV 0x20

#define LED_TEST_ADC 0x42

float fd_lp55231_test_led(uint8_t led) {
    bool result = fd_i2c1_register_write(ADDRESS, LED_TEST_CONTROL, EN_LEDTEST_ADC | (led - 1));
    if (!result) {
        fd_log_assert_fail("");
    }
    fd_hal_processor_delay_ms(5);
    uint8_t adc;
    result = fd_i2c1_register_read(ADDRESS, LED_TEST_ADC, &adc);
    if (!result) {
        fd_log_assert_fail("");
    }

    float v = adc * 0.03f - 1.478f;
    return v;
}