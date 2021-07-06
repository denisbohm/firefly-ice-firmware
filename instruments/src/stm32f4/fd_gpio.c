#include "fd_gpio.h"

#include "fd_log.h"

#include <stm32f4xx.h>

void fd_gpio_initialize(void) {
    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOA;
    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOB;
    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOC;
    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOD;
    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOE;
    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOF;
    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOG;
    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOH;
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

static void fd_gpio_configure(fd_gpio_t gpio, uint32_t mode, uint32_t type) {
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin = 1 << gpio.pin;
    GPIO_InitStruct.GPIO_Mode = mode;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = type;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(fd_gpio_port(gpio.port), &GPIO_InitStruct);
}

void fd_gpio_configure_output(fd_gpio_t gpio) {
    fd_gpio_configure(gpio, GPIO_Mode_OUT, GPIO_OType_PP);
}

void fd_gpio_configure_output_open_drain(fd_gpio_t gpio) {
    fd_gpio_configure(gpio, GPIO_Mode_OUT, GPIO_OType_OD);
}

void fd_gpio_configure_input(fd_gpio_t gpio) {
    fd_gpio_configure(gpio, GPIO_Mode_IN, GPIO_OType_PP);
}

void fd_gpio_set(fd_gpio_t gpio, bool value) {
    if (value) {
        fd_gpio_port(gpio.port)->BSRRL = 1 << gpio.pin;
    } else {
        fd_gpio_port(gpio.port)->BSRRH = 1 << gpio.pin;
    }
}

bool fd_gpio_get(fd_gpio_t gpio) {
    return (fd_gpio_port(gpio.port)->IDR & (1 << gpio.pin)) != 0;
}
