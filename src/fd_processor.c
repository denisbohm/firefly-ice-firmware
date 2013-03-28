#include "fd_processor.h"

#include <em_i2c.h>
#include <em_gpio.h>
#include <em_usart.h>

void fd_processor_initialize(void) {
    GPIO_PinModeSet(gpioPortA, 0, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortA, 1, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortA, 2, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortA, 3, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortA, 4, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortA, 5, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortA, 6, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortA, 15, gpioModeDisabled, 0); // unused port pin
    //
    GPIO_PinModeSet(gpioPortC, 4, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortC, 5, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortC, 8, gpioModeDisabled, 0); // unused port pin
    //
    GPIO_PinModeSet(gpioPortE, 8, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortE, 10, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortE, 11, gpioModeDisabled, 0); // unused port pin

    GPIO_PinModeSet(BAT_VDIV2EN_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(BAT_VDIV2_PORT_PIN, gpioModeDisabled, 0); // analog input

    GPIO_PinModeSet(AUX_PWR_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(MAG_INT_PORT_PIN, gpioModeInputPull, 0);
    GPIO_IntConfig(MAG_INT_PORT_PIN, true /* rising */, false /* falling */, true);
    GPIO_PinModeSet(I2C1_SDA_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(I2C1_SCL_PORT_PIN, gpioModePushPull, 0);
    I2C1->ROUTE = I2C_ROUTE_SDAPEN | I2C_ROUTE_SCLPEN | I2C1_LOCATION;

    GPIO_PinModeSet(ACC_CSN_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(US0_CLK_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(US0_MISO_PORT_PIN, gpioModeInputPull, 0);
    GPIO_PinModeSet(US0_MOSI_PORT_PIN, gpioModePushPull, 0);
    USART0->ROUTE = USART_ROUTE_TXPEN | USART_ROUTE_RXPEN | USART_ROUTE_CLKPEN | US0_LOCATION;
    GPIO_PinModeSet(ACC_INT_PORT_PIN, gpioModeInput, 0);
    GPIO_IntConfig(ACC_INT_PORT_PIN, true /* rising */, true /* falling */, true);

    GPIO_PinModeSet(LED_EN_PORT_PIN, gpioModePushPull, 0);

    GPIO_PinModeSet(LED1_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(LED2_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(LED3_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(LED4_PORT_PIN, gpioModePushPull, 1);

    GPIO_PinModeSet(LED5_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(LED6_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(LED7_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(LED8_PORT_PIN, gpioModePushPull, 1);

    GPIO_PinModeSet(SWD_CLK_PORT_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(SWD_IO_PORT_PIN, gpioModeDisabled, 0);

    GPIO_PinModeSet(CHG_STAT_PORT_PIN, gpioModeInput, 0);
    GPIO_IntConfig(CHG_STAT_PORT_PIN, true /* rising */, true /* falling */, true);
    GPIO_PinModeSet(CHG_RATE_PORT_PIN, gpioModeDisabled, 0); // analog input

    GPIO_PinModeSet(PWR_HIGH_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(PWR_MODE_PORT_PIN, gpioModePushPull, 1);

    GPIO_PinModeSet(USB_DM_PORT_PIN, gpioModeInput, 0);
    GPIO_PinModeSet(USB_DP_PORT_PIN, gpioModeInput, 0);

    GPIO_PinModeSet(NRF_PWR_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(US1_CLK_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(US1_MISO_PORT_PIN, gpioModeInputPull, 0);
    GPIO_PinModeSet(US1_MOSI_PORT_PIN, gpioModePushPull, 0);
    USART1->ROUTE = USART_ROUTE_TXPEN | USART_ROUTE_RXPEN | USART_ROUTE_CLKPEN | US1_LOCATION;
    GPIO_PinModeSet(NRF_RESETN_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(NRF_REQN_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(NRF_RDYN_PORT_PIN, gpioModeInput, 0);
    GPIO_IntConfig(NRF_RDYN_PORT_PIN, true /* rising */, true /* falling */, true);

    GPIO_PinModeSet(LFXTAL_P_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(LFXTAL_N_PORT_PIN, gpioModePushPull, 0);

    GPIO_PinModeSet(HFXTAL_P_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(HFXTAL_P_PORT_PIN, gpioModePushPull, 0);
}

void __attribute((naked)) fd_delay_3x_cycles(uint32_t cycles) {
    __asm(
        "    subs r0, #1\n"
        "    bne fd_delay_3x_cycles\n"
        "    bx lr"
    );
}

#define CYCLES_PER_SECOND 48000000

void fd_delay_ms(uint32_t ms) {
    while (ms--) {
        fd_delay_3x_cycles(CYCLES_PER_SECOND / 3000);
    }
}