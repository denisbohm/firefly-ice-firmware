#include "fd_adc.h"
#include "fd_i2c1.h"
#include "fd_lis3dh.h"
#include "fd_mag3110.h"
#include "fd_nrf8001.h"
#include "fd_processor.h"
#include "fd_rtc.h"
#include "fd_spi0.h"
#include "fd_spi1.h"
#include "fd_tca6507.h"

#include <em_gpio.h>

#include <stdbool.h>

void main(void) {
    fd_processor_initialize();

    fd_rtc_initialize();

    fd_adc_initialize();
    fd_i2c1_initialize();
    fd_spi0_initialize();
    fd_spi1_initialize();

    GPIO_PinOutClear(LED1_PORT_PIN);
    fd_delay_ms(250);
    GPIO_PinOutSet(LED1_PORT_PIN);

    GPIO_PinOutClear(LED2_PORT_PIN);
    fd_delay_ms(250);
    GPIO_PinOutSet(LED2_PORT_PIN);

    GPIO_PinOutClear(LED3_PORT_PIN);
    fd_delay_ms(250);
    GPIO_PinOutSet(LED3_PORT_PIN);

    GPIO_PinOutClear(LED4_PORT_PIN);
    fd_delay_ms(250);
    GPIO_PinOutSet(LED4_PORT_PIN);

    GPIO_PinOutClear(LED5_PORT_PIN);
    fd_delay_ms(250);
    GPIO_PinOutSet(LED5_PORT_PIN);

    GPIO_PinOutClear(LED6_PORT_PIN);
    fd_delay_ms(250);
    GPIO_PinOutSet(LED6_PORT_PIN);

    GPIO_PinOutClear(LED7_PORT_PIN);
    fd_delay_ms(250);
    GPIO_PinOutSet(LED7_PORT_PIN);

    GPIO_PinOutClear(LED8_PORT_PIN);
    fd_delay_ms(250);
    GPIO_PinOutSet(LED8_PORT_PIN);

    fd_lis3dh_initialize();

    fd_i2c1_power_on();

    fd_tca6507_initialize();
    fd_tca6507_enable();
    fd_tca6507_test();

    fd_mag3110_initialize();
    fd_mag3110_test();

    fd_spi1_power_on();

    fd_nrf8001_initialize();
    fd_nrf8001_reset();
    while (true) {
        fd_nrf8001_transfer();
    }
}