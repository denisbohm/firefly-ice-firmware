#include "fd_gpio.h"

#include "fdi_stm32.h"

#include "fd_log.h"

void fd_gpio_initialize(void) {
    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();
    __GPIOD_CLK_ENABLE();
    __GPIOE_CLK_ENABLE();
    __GPIOF_CLK_ENABLE();
    __GPIOG_CLK_ENABLE();
    __GPIOH_CLK_ENABLE();
}

static GPIO_TypeDef *fd_gpio_port(uint32_t port) {
    switch (port) {
        case 0:
            return GPIOA;
        case 1:
            return GPIOB;
        case 2:
            return GPIOC;
        case 3:
            return GPIOD;
        case 4:
            return GPIOE;
        case 5:
            return GPIOF;
        case 6:
            return GPIOG;
        case 7:
            return GPIOH;
        default:
            break;
    }
    fd_log_assert_fail("unknown port");
    return 0;
}

static void fd_gpio_configure(fd_gpio_t gpio, uint32_t mode, uint32_t pull) {
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = 1 << gpio.pin;
    GPIO_InitStruct.Mode = mode;
    GPIO_InitStruct.Pull = pull;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(fd_gpio_port(gpio.port), &GPIO_InitStruct);

#if defined(GPIO_ASCR_ASC0)
    if (mode == GPIO_MODE_ANALOG) {
        GPIOA->ASCR |= GPIO_InitStruct.Pin;
    } else {
        GPIOA->ASCR &= ~GPIO_InitStruct.Pin;
    }
#endif
}

void fd_gpio_configure_output(fd_gpio_t gpio) {
    fd_gpio_configure(gpio, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
}

void fd_gpio_configure_output_open_drain(fd_gpio_t gpio) {
    fd_gpio_configure(gpio, GPIO_MODE_OUTPUT_OD, GPIO_NOPULL);
}

void fd_gpio_configure_output_open_drain_pull_up(fd_gpio_t gpio) {
    fd_gpio_configure(gpio, GPIO_MODE_OUTPUT_OD, GPIO_PULLUP);
}

void fd_gpio_configure_input(fd_gpio_t gpio) {
    fd_gpio_configure(gpio, GPIO_MODE_INPUT, GPIO_NOPULL);
}

void fd_gpio_configure_input_pull_up(fd_gpio_t gpio) {
    fd_gpio_configure(gpio, GPIO_MODE_INPUT, GPIO_PULLUP);
}

void fd_gpio_set(fd_gpio_t gpio, bool value) {
    if (value) {
        fd_gpio_port(gpio.port)->BSRR = 0x00000001 << gpio.pin;
    } else {
        fd_gpio_port(gpio.port)->BSRR = 0x00010000 << gpio.pin;
    }
}

bool fd_gpio_get(fd_gpio_t gpio) {
    return (fd_gpio_port(gpio.port)->IDR & (1 << gpio.pin)) != 0;
}
