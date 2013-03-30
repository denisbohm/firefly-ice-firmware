#include "fd_i2c1.h"
#include "fd_processor.h"

#include <em_gpio.h>

#define SELECT0 0x00
#define SELECT1 0x01
#define SELECT2 0x02
#define FADE_ON_TIME 0x03
#define ADDRESS 0x8a

void fd_tca6507_initialize(void) {
}

void fd_tca6507_enable(void) {
    GPIO_PinOutSet(LED_EN_PORT_PIN);
}

void fd_tca6507_disable(void) {
    GPIO_PinOutClear(LED_EN_PORT_PIN);
}

void fd_tca6507_test(void) {
    uint8_t fade_on_time;
    bool result = fd_i2c1_read(ADDRESS, FADE_ON_TIME, &fade_on_time);
    if (!result) {
        return;
    }
    if (fade_on_time != 0x44) {
        // log diagnostic
        return;
    }
    result = fd_i2c1_write(ADDRESS, SELECT0, 0xff);
    result = fd_i2c1_write(ADDRESS, SELECT1, 0xff);
    result = fd_i2c1_write(ADDRESS, SELECT2, 0xff);
}