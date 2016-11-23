#include "fdi_gpio.h"

#include "fd_log.h"

#include <stm32f4xx.h>

typedef struct {
    uint32_t enable;
    GPIO_TypeDef *port;
    uint8_t pin;
    GPIOMode_TypeDef mode;
    GPIOSpeed_TypeDef speed;
    GPIOOType_TypeDef type;
    GPIOPuPd_TypeDef pull;
    bool initially;
} fdi_gpio_t;

fdi_gpio_t fdi_gpios[] = {
    { // ATE_USB_CS_EN
        .enable = RCC_AHB1Periph_GPIOB,
        .port = GPIOB,
        .pin  = 14,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = false,
    },
    { // FDI_ATE_BS_EN
        .enable = RCC_AHB1Periph_GPIOC,
        .port = GPIOC,
        .pin  = 11,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = false,
    },
    { // ATE_USB_5V_EN
        .enable = RCC_AHB1Periph_GPIOB,
        .port = GPIOB,
        .pin  = 13,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = false,
    },
    { // ATE_USB_D_EN
        .enable = RCC_AHB1Periph_GPIOB,
        .port = GPIOB,
        .pin  = 15,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = false,
    },
    { // ATE_BATTERY_SENSE
        .enable = RCC_AHB1Periph_GPIOC,
        .port = GPIOC,
        .pin  = 10,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = false,
    },
    { // ATE_BUTTON_EN
        .enable = RCC_AHB1Periph_GPIOC,
        .port = GPIOC,
        .pin  = 12,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = false,
    },
    { // ATE_MCU_VCC_SENSE
        .enable = RCC_AHB1Periph_GPIOD,
        .port = GPIOD,
        .pin  = 2,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = false,
    },

    { // ATE_SWD1_DIR_TO_DUT
        .enable = RCC_AHB1Periph_GPIOB,
        .port = GPIOB,
        .pin  = 0,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = false,
    },
    { // ATE_SWD1_NRESET
        .enable = RCC_AHB1Periph_GPIOB,
        .port = GPIOB,
        .pin  = 1,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = false,
    },
    { // ATE_SWD1_SWDCLK
        .enable = RCC_AHB1Periph_GPIOB,
        .port = GPIOB,
        .pin  = 2,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = false,
    },
    { // ATE_SWD1_PWR
        .enable = RCC_AHB1Periph_GPIOB,
        .port = GPIOB,
        .pin  = 10,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = false,
    },
    { // ATE_SWD1_SWDIO
        .enable = RCC_AHB1Periph_GPIOB,
        .port = GPIOB,
        .pin  = 12,
        .mode = GPIO_Mode_IN,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = true,
    },

    { // ATE_SWD2_DIR_TO_DUT
        .enable = RCC_AHB1Periph_GPIOC,
        .port = GPIOC,
        .pin  = 6,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = false,
    },
    { // ATE_SWD2_NRESET
        .enable = RCC_AHB1Periph_GPIOC,
        .port = GPIOC,
        .pin  = 7,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = false,
    },
    { // ATE_SWD2_SWDCLK
        .enable = RCC_AHB1Periph_GPIOC,
        .port = GPIOC,
        .pin  = 8,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = false,
    },
    { // ATE_SWD2_PWR
        .enable = RCC_AHB1Periph_GPIOC,
        .port = GPIOC,
        .pin  = 9,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = false,
    },
    { // ATE_SWD2_SWDIO
        .enable = RCC_AHB1Periph_GPIOA,
        .port = GPIOA,
        .pin  = 8,
        .mode = GPIO_Mode_IN,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = true,
    },

    { // LED R
        .enable = RCC_AHB1Periph_GPIOC,
        .port = GPIOC,
        .pin  = 15,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = true,
    },
    { // LED G
        .enable = RCC_AHB1Periph_GPIOC,
        .port = GPIOC,
        .pin  = 14,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = true,
    },
    { // LED B
        .enable = RCC_AHB1Periph_GPIOC,
        .port = GPIOC,
        .pin  = 13,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = true,
    },
    { // SPI SCLK
        .enable = RCC_AHB1Periph_GPIOA,
        .port = GPIOA,
        .pin  = 0,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = true,
    },
    { // SPI MOSI
        .enable = RCC_AHB1Periph_GPIOA,
        .port = GPIOA,
        .pin  = 1,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = true,
    },
    { // SPI MISO
        .enable = RCC_AHB1Periph_GPIOA,
        .port = GPIOA,
        .pin  = 2,
        .mode = GPIO_Mode_IN,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_DOWN,
        .initially = true,
    },
    { // S25FL116K CSN
        .enable = RCC_AHB1Periph_GPIOB,
        .port = GPIOB,
        .pin  = 15,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = true,
    },
    { // FDI_GPIO_ATE_SWD1_SENSE_NRESET
        .enable = RCC_AHB1Periph_GPIOB,
        .port = GPIOB,
        .pin  = 6,
        .mode = GPIO_Mode_IN,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = true,
    },
    { // FDI_GPIO_ATE_SWD2_SENSE_NRESET
        .enable = RCC_AHB1Periph_GPIOB,
        .port = GPIOB,
        .pin  = 9,
        .mode = GPIO_Mode_IN,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = true,
    },
    { // FDI_GPIO_ATE_FILL_EN
        .enable = RCC_AHB1Periph_GPIOB,
        .port = GPIOB,
        .pin  = 4,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = true,
    },
    { // FDI_GPIO_ATE_DRAIN_EN
        .enable = RCC_AHB1Periph_GPIOB,
        .port = GPIOB,
        .pin  = 5,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = true,
    },
    { // FDI_GPIO_ATE_BAT_CAP_EN
        .enable = RCC_AHB1Periph_GPIOB,
        .port = GPIOB,
        .pin  = 3,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = true,
    },
    { // FDI_GPIO_ATE_BAT_ADJ_EN
        .enable = RCC_AHB1Periph_GPIOC,
        .port = GPIOC,
        .pin  = 10,
        .mode = GPIO_Mode_OUT,
        .speed = GPIO_Speed_50MHz,
        .type = GPIO_OType_PP,
        .pull = GPIO_PuPd_NOPULL,
        .initially = true,
    },
};

const uint32_t fdi_gpio_count = (sizeof(fdi_gpios) / sizeof(fdi_gpio_t));

void fdi_gpio_initialize(void) {
    for (uint32_t i = 0; i < fdi_gpio_count; ++i) {
        fdi_gpio_t *gpio = &fdi_gpios[i];

        // enable gpio port clock
        RCC->AHB1ENR |= gpio->enable;

        // initial output port value
        if (gpio->initially) {
            gpio->port->BSRRL = 1 << gpio->pin;
        } else {
            gpio->port->BSRRH = 1 << gpio->pin;
        }

        // initial port configuration
        GPIO_InitTypeDef GPIO_InitStruct;
        GPIO_StructInit(&GPIO_InitStruct);
        GPIO_InitStruct.GPIO_Pin = 1 << gpio->pin;
        GPIO_InitStruct.GPIO_Mode = gpio->mode;
        GPIO_InitStruct.GPIO_Speed = gpio->speed;
        GPIO_InitStruct.GPIO_OType = gpio->type;
        GPIO_InitStruct.GPIO_PuPd = gpio->pull;
        GPIO_Init(gpio->port, &GPIO_InitStruct);
    }
}

void fdi_gpio_set(uint32_t identifier, bool value) {
    if (value) {
        fdi_gpio_on(identifier);
    } else {
        fdi_gpio_off(identifier);
    }
}

void fdi_gpio_on(uint32_t identifier) {
    fd_log_assert(identifier < fdi_gpio_count);

    fdi_gpio_t *gpio = &fdi_gpios[identifier];
    gpio->port->BSRRL = 1 << gpio->pin;
}

void fdi_gpio_off(uint32_t identifier) {
    fd_log_assert(identifier < fdi_gpio_count);

    fdi_gpio_t *gpio = &fdi_gpios[identifier];
    gpio->port->BSRRH = 1 << gpio->pin;
}

bool fdi_gpio_get(uint32_t identifier) {
    fd_log_assert(identifier < fdi_gpio_count);

    fdi_gpio_t *gpio = &fdi_gpios[identifier];
    return (gpio->port->IDR & (1 << gpio->pin)) != 0;
}

const int fdi_gpio_mode_input = 0b00;
const int fdi_gpio_mode_output = 0b01;
const int fdi_gpio_mode_alternate_function = 0b10;
const int fdi_gpio_mode_analog = 0b11;

void fdi_gpio_set_mode(GPIO_TypeDef *port, int pin_index, int mode) {
    port->MODER = (port->MODER & ~(0b11 << (pin_index * 2))) | (mode << (pin_index * 2));
}

void fdi_gpio_set_mode_in(uint32_t identifier) {
    fd_log_assert(identifier < fdi_gpio_count);

    fdi_gpio_t *gpio = &fdi_gpios[identifier];
    fdi_gpio_set_mode(gpio->port, gpio->pin, fdi_gpio_mode_input);
}

void fdi_gpio_set_mode_out(uint32_t identifier) {
    fd_log_assert(identifier < fdi_gpio_count);

    fdi_gpio_t *gpio = &fdi_gpios[identifier];
    fdi_gpio_set_mode(gpio->port, gpio->pin, fdi_gpio_mode_output);
}

// optimized spi bit-bang transfer out -denis
void fdi_gpio_spi_out(uint32_t clock, uint32_t mosi, uint32_t miso __attribute((unused)), uint8_t *out, uint32_t length) {
    fdi_gpio_t *clock_gpio = &fdi_gpios[clock];
    __IO uint16_t *clock_on_address = &clock_gpio->port->BSRRL;
    __IO uint16_t *clock_off_address = &clock_gpio->port->BSRRH;
    uint16_t clock_bit = 1 << clock_gpio->pin;

    fdi_gpio_t *mosi_gpio = &fdi_gpios[mosi];
    __IO uint16_t *mosi_on_address = &mosi_gpio->port->BSRRL;
    __IO uint16_t *mosi_off_address = &mosi_gpio->port->BSRRH;
    uint16_t mosi_bit = 1 << mosi_gpio->pin;

//    fdi_gpio_t *miso_gpio = &fdi_gpios[miso];
//    __IO uint32_t *miso_in_address = &miso_gpio->port->IDR;
//    uint32_t miso_shift = miso_gpio->pin;

    for (uint32_t i = 0; i < length; ++i) {
        uint8_t data = *out++;
//        uint8_t in = 0;
        for (int j = 0; j < 8; ++j) {
            *clock_off_address = clock_bit;
//            fdi_gpio_off(FDI_GPIO_SPI_SCLK);
            if (data & 0x80) {
                *mosi_on_address = mosi_bit;
//                fdi_gpio_on(FDI_GPIO_SPI_MOSI);
            } else {
                *mosi_off_address = mosi_bit;
//                fdi_gpio_off(FDI_GPIO_SPI_MOSI);
            }
            data = data << 1;
//            fdi_spi_delay();

            *clock_on_address = clock_bit;
            fdi_gpio_on(FDI_GPIO_SPI_SCLK);
//            fdi_spi_delay();
//            in = (in << 1) | ((*miso_in_address >> miso_shift) & 0b1);
//            in = (in << 1) | fdi_gpio_get(FDI_GPIO_SPI_MISO);
        }
    }
}

// optimized spi bit-bang transfer in -denis
void fdi_gpio_spi_in(uint32_t clock, uint32_t mosi, uint32_t miso, uint8_t *in, uint32_t length) {
    fdi_gpio_t *clock_gpio = &fdi_gpios[clock];
    __IO uint16_t *clock_on_address = &clock_gpio->port->BSRRL;
    __IO uint16_t *clock_off_address = &clock_gpio->port->BSRRH;
    uint16_t clock_bit = 1 << clock_gpio->pin;

    fdi_gpio_t *mosi_gpio = &fdi_gpios[mosi];
//    __IO uint16_t *mosi_on_address = &mosi_gpio->port->BSRRL;
    __IO uint16_t *mosi_off_address = &mosi_gpio->port->BSRRH;
    uint16_t mosi_bit = 1 << mosi_gpio->pin;

    fdi_gpio_t *miso_gpio = &fdi_gpios[miso];
    __IO uint32_t *miso_in_address = &miso_gpio->port->IDR;
    uint32_t miso_shift = miso_gpio->pin;

    *mosi_off_address = mosi_bit;
    for (uint32_t i = 0; i < length; ++i) {
//        uint8_t data = *out++;
        uint8_t in_byte = 0;
        for (int j = 0; j < 8; ++j) {
            *clock_off_address = clock_bit;
//            fdi_gpio_off(FDI_GPIO_SPI_SCLK);
//            if (data & 0x80) {
//                *mosi_on_address = mosi_bit;
//                fdi_gpio_on(FDI_GPIO_SPI_MOSI);
//            } else {
//                *mosi_off_address = mosi_bit;
//                fdi_gpio_off(FDI_GPIO_SPI_MOSI);
//            }
//            data = data << 1;
//            fdi_spi_delay();

            *clock_on_address = clock_bit;
            fdi_gpio_on(FDI_GPIO_SPI_SCLK);
//            fdi_spi_delay();
            in_byte = (in_byte << 1) | ((*miso_in_address >> miso_shift) & 0b1);
//            in = (in << 1) | fdi_gpio_get(FDI_GPIO_SPI_MISO);
        }
        *in++ = in_byte;
    }
}

void fdi_gpio_serial_wire_debug_out(uint32_t clock, uint32_t data, uint8_t *out, uint32_t length) {
    fdi_gpio_t *clock_gpio = &fdi_gpios[clock];
    __IO uint16_t *clock_on_address = &clock_gpio->port->BSRRL;
    __IO uint16_t *clock_off_address = &clock_gpio->port->BSRRH;
    uint16_t clock_bit = 1 << clock_gpio->pin;

    fdi_gpio_t *data_gpio = &fdi_gpios[data];
    __IO uint16_t *data_on_address = &data_gpio->port->BSRRL;
    __IO uint16_t *data_off_address = &data_gpio->port->BSRRH;
    uint16_t data_bit = 1 << data_gpio->pin;

    for (uint32_t i = 0; i < length; ++i) {
        uint8_t byte = *out++;
        for (int j = 0; j < 8; ++j) {
//            fdi_serial_wire_half_bit_delay();
            *clock_on_address = clock_bit;
//            fdi_gpio_on(serial_wire->gpio_clock);
//            fdi_serial_wire_half_bit_delay();

            if (byte & 1) {
                *data_on_address = data_bit;
//                fdi_gpio_on(serial_wire->gpio_data);
            } else {
                *data_off_address = data_bit;
//                fdi_gpio_off(serial_wire->gpio_data);
            }
            byte >>= 1;

            *clock_off_address = clock_bit;
//            fdi_gpio_off(serial_wire->gpio_clock);
        }
    }
}

void fdi_gpio_serial_wire_debug_in(uint32_t clock, uint32_t data, uint8_t *in, uint32_t length) {
    fdi_gpio_t *clock_gpio = &fdi_gpios[clock];
    __IO uint16_t *clock_on_address = &clock_gpio->port->BSRRL;
    __IO uint16_t *clock_off_address = &clock_gpio->port->BSRRH;
    uint16_t clock_bit = 1 << clock_gpio->pin;

    fdi_gpio_t *data_gpio = &fdi_gpios[data];
    __IO uint32_t *data_in_address = &data_gpio->port->IDR;
    uint32_t data_bit = 1 << data_gpio->pin;

    for (uint32_t i = 0; i < length; ++i) {
        uint8_t byte = 0;
        for (int j = 0; j < 8; ++j) {
//            fdi_serial_wire_half_bit_delay();
            *clock_on_address = clock_bit;
//            fdi_gpio_on(serial_wire->gpio_clock);
//            fdi_serial_wire_half_bit_delay();

            byte >>= 1;
            if (*data_in_address & data_bit) {
                byte |= 0b10000000;
            }

            *clock_off_address = clock_bit;
//            fdi_gpio_off(serial_wire->gpio_clock);
        }
        *in++ = byte;
    }
}
