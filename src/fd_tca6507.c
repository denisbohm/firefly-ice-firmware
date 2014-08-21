#include "fd_hal_processor.h"
#include "fd_i2c1.h"
#include "fd_log.h"
#include "fd_pins.h"

#include <em_gpio.h>

#define FD_TCA6507_SELECT0 0x00
#define FD_TCA6507_SELECT1 0x01
#define FD_TCA6507_SELECT2 0x02
#define FD_TCA6507_FADE_ON_TIME 0x03
#define FD_TCA6507_FULLY_ON_TIME 0x04
#define FD_TCA6507_FADE_OFF_TIME 0x05
#define FD_TCA6507_FIRST_FULLY_OFF_TIME 0x06
#define FD_TCA6507_SECOND_FULLY_OFF_TIME 0x07
#define FD_TCA6507_MAXIMUM_INTENSITY 0x08
#define FD_TCA6507_ONE_SHOT_MASTER_INTENSITY 0x09
#define FD_TCA6507_INITIALIZATION 0x0a

#define FD_TCA6507_ADDRESS 0x8a

void fd_tca6507_initialize(void) {
}

void fd_tca6507_wake(void) {
    GPIO_PinOutSet(LED_EN_PORT_PIN);
}

void fd_tca6507_sleep(void) {
    GPIO_PinOutClear(LED_EN_PORT_PIN);
}

// P1 D9.B
// P2 D9.G
// P3 D9.R
// P4 D0.B
// P5 D0.G
// P6 D0.R

void fd_tca6507_set_color(bool r, bool g, bool b) {
    uint8_t value = 0;
    if (r) {
        value |= 0b01001000;
    }
    if (g) {
        value |= 0b00100100;
    }
    if (b) {
        value |= 0b00010010;
    }
    bool result = fd_i2c1_register_write(FD_TCA6507_ADDRESS, FD_TCA6507_SELECT2, value);
    if (!result) {
        fd_log_assert_fail("");
    }
}

void fd_tca6507_test(void) {
    uint8_t fade_on_time;
    bool result = fd_i2c1_register_read(FD_TCA6507_ADDRESS, FD_TCA6507_FADE_ON_TIME, &fade_on_time);
    if (!result) {
        fd_log_assert_fail("");
        return;
    }
    if (fade_on_time != 0x44) {
        fd_log_assert_fail("");
        return;
    }
}