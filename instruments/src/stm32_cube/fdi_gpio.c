#include "fdi_gpio.h"

#include "fdi_stm32.h"

#include "fd_log.h"

typedef enum {
    fdi_gpio_status_valid,
    fdi_gpio_status_invalid,
    fdi_gpio_status_noop,
} fdi_gpio_status_t;

typedef struct {
    fdi_gpio_status_t status;
    void (*enable)(void);
    GPIO_TypeDef *port;
    uint8_t pin;
    uint32_t mode;
    uint32_t pull;
    uint32_t speed;
    uint32_t alternate;
    bool initially;
} fdi_gpio_t;

static void fdi_gpio_clock_enable_a(void) {
    __GPIOA_CLK_ENABLE();
}

static void fdi_gpio_clock_enable_b(void) {
    __GPIOB_CLK_ENABLE();
}

static void fdi_gpio_clock_enable_c(void) {
    __GPIOC_CLK_ENABLE();
}

static void fdi_gpio_clock_enable_d(void) {
    __GPIOD_CLK_ENABLE();
}

static void fdi_gpio_clock_enable_e(void) {
    __GPIOE_CLK_ENABLE();
}

static void fdi_gpio_clock_enable_f(void) {
    __GPIOF_CLK_ENABLE();
}

static void fdi_gpio_clock_enable_g(void) {
    __GPIOG_CLK_ENABLE();
}

static void fdi_gpio_clock_enable_h(void) {
    __GPIOH_CLK_ENABLE();
}

#define FDI_INSTRUMENT_ALL_IN_ONE 0
#define FDI_INSTRUMENT_SERIAL_WIRE 1
#define FDI_INSTRUMENT_POWER 0
#define FDI_INSTRUMENT_INPUT_OUTPUT 0

#if FDI_INSTRUMENT_SERIAL_WIRE
fdi_gpio_t fdi_gpios[] = {
    { // ATE_USB_CS_EN
        .status = fdi_gpio_status_invalid,
    },
    { // FDI_ATE_BS_EN
        .status = fdi_gpio_status_invalid,
    },
    { // ATE_USB_5V_EN
        .status = fdi_gpio_status_invalid,
    },
    { // ATE_USB_D_EN
        .status = fdi_gpio_status_invalid,
    },
    { // ATE_BATTERY_SENSE
        .status = fdi_gpio_status_invalid,
    },
    { // ATE_BUTTON_EN
        .status = fdi_gpio_status_invalid,
    },
    { // ATE_MCU_VCC_SENSE
        .status = fdi_gpio_status_invalid,
    },

    { // ATE_SWD1_DIR_TO_DUT
        .status = fdi_gpio_status_valid,
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 2,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_SWD1_NRESET
        .status = fdi_gpio_status_valid,
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 1,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_SWD1_SWDCLK
        .status = fdi_gpio_status_valid,
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 4,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_SWD1_PWR
        .status = fdi_gpio_status_valid,
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 0,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_SWD1_SWDIO
        .status = fdi_gpio_status_valid,
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 3,
        .mode = GPIO_MODE_INPUT,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },

    { // ATE_SWD2_DIR_TO_DUT
        .status = fdi_gpio_status_valid,
        .enable = fdi_gpio_clock_enable_c,
        .port = GPIOC,
        .pin  = 2,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_SWD2_NRESET
        .status = fdi_gpio_status_valid,
        .enable = fdi_gpio_clock_enable_c,
        .port = GPIOC,
        .pin  = 1,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_SWD2_SWDCLK
        .status = fdi_gpio_status_valid,
        .enable = fdi_gpio_clock_enable_c,
        .port = GPIOC,
        .pin  = 4,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_SWD2_PWR
        .status = fdi_gpio_status_valid,
        .enable = fdi_gpio_clock_enable_c,
        .port = GPIOC,
        .pin  = 0,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_SWD2_SWDIO
        .status = fdi_gpio_status_valid,
        .enable = fdi_gpio_clock_enable_c,
        .port = GPIOC,
        .pin  = 3,
        .mode = GPIO_MODE_INPUT,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },

    { // LED R
        .status = fdi_gpio_status_invalid,
    },
    { // LED G
        .status = fdi_gpio_status_invalid,
    },
    { // LED B
        .status = fdi_gpio_status_invalid,
    },
    { // SPI SCLK
        .status = fdi_gpio_status_valid,
        .enable = fdi_gpio_clock_enable_a,
        .port = GPIOA,
        .pin  = 5,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // SPI MOSI
        .status = fdi_gpio_status_valid,
        .enable = fdi_gpio_clock_enable_a,
        .port = GPIOA,
        .pin  = 7,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // SPI MISO
        .status = fdi_gpio_status_valid,
        .enable = fdi_gpio_clock_enable_a,
        .port = GPIOA,
        .pin  = 6,
        .mode = GPIO_MODE_INPUT,
        .pull = GPIO_PULLDOWN,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // S25FL116K CSN
        .status = fdi_gpio_status_valid,
        .enable = fdi_gpio_clock_enable_a,
        .port = GPIOA,
        .pin  = 4,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // FDI_GPIO_ATE_SWD1_SENSE_NRESET
        .status = fdi_gpio_status_valid,
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 5,
        .mode = GPIO_MODE_INPUT,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // FDI_GPIO_ATE_SWD2_SENSE_NRESET
        .status = fdi_gpio_status_valid,
        .enable = fdi_gpio_clock_enable_c,
        .port = GPIOC,
        .pin  = 5,
        .mode = GPIO_MODE_INPUT,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // FDI_GPIO_ATE_FILL_EN
        .status = fdi_gpio_status_invalid,
    },
    { // FDI_GPIO_ATE_DRAIN_EN
        .status = fdi_gpio_status_invalid,
    },
    { // FDI_GPIO_ATE_BAT_CAP_EN
        .status = fdi_gpio_status_invalid,
    },
    { // FDI_GPIO_ATE_BAT_ADJ_EN
        .status = fdi_gpio_status_invalid,
    },
};

#endif

#if FDI_INSTRUMENT_ALL_IN_ONE
fdi_gpio_t fdi_gpios[] = {
    { // ATE_USB_CS_EN
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 14,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // FDI_ATE_BS_EN
        .enable = fdi_gpio_clock_enable_c,
        .port = GPIOC,
        .pin  = 11,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_USB_5V_EN
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 13,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_USB_D_EN
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 15,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_BATTERY_SENSE
        .enable = fdi_gpio_clock_enable_c,
        .port = GPIOC,
        .pin  = 10,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_BUTTON_EN
        .enable = fdi_gpio_clock_enable_c,
        .port = GPIOC,
        .pin  = 12,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_MCU_VCC_SENSE
        .enable = fdi_gpio_clock_enable_d,
        .port = GPIOD,
        .pin  = 2,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },

    { // ATE_SWD1_DIR_TO_DUT
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 0,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_SWD1_NRESET
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 1,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_SWD1_SWDCLK
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 2,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_SWD1_PWR
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 10,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_SWD1_SWDIO
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 12,
        .mode = GPIO_MODE_INPUT,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },

    { // ATE_SWD2_DIR_TO_DUT
        .enable = fdi_gpio_clock_enable_c,
        .port = GPIOC,
        .pin  = 6,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_SWD2_NRESET
        .enable = fdi_gpio_clock_enable_c,
        .port = GPIOC,
        .pin  = 7,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_SWD2_SWDCLK
        .enable = fdi_gpio_clock_enable_c,
        .port = GPIOC,
        .pin  = 8,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_SWD2_PWR
        .enable = fdi_gpio_clock_enable_c,
        .port = GPIOC,
        .pin  = 9,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = false,
    },
    { // ATE_SWD2_SWDIO
        .enable = fdi_gpio_clock_enable_a,
        .port = GPIOA,
        .pin  = 8,
        .mode = GPIO_MODE_INPUT,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },

    { // LED R
        .enable = fdi_gpio_clock_enable_c,
        .port = GPIOC,
        .pin  = 15,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // LED G
        .enable = fdi_gpio_clock_enable_c,
        .port = GPIOC,
        .pin  = 14,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // LED B
        .enable = fdi_gpio_clock_enable_c,
        .port = GPIOC,
        .pin  = 13,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // SPI SCLK
        .enable = fdi_gpio_clock_enable_a,
        .port = GPIOA,
        .pin  = 0,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // SPI MOSI
        .enable = fdi_gpio_clock_enable_a,
        .port = GPIOA,
        .pin  = 1,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // SPI MISO
        .enable = fdi_gpio_clock_enable_a,
        .port = GPIOA,
        .pin  = 2,
        .mode = GPIO_MODE_INPUT,
        .pull = GPIO_PULLDOWN,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // S25FL116K CSN
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 15,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // FDI_GPIO_ATE_SWD1_SENSE_NRESET
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 6,
        .mode = GPIO_MODE_INPUT,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // FDI_GPIO_ATE_SWD2_SENSE_NRESET
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 9,
        .mode = GPIO_MODE_INPUT,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // FDI_GPIO_ATE_FILL_EN
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 4,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // FDI_GPIO_ATE_DRAIN_EN
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 5,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // FDI_GPIO_ATE_BAT_CAP_EN
        .enable = fdi_gpio_clock_enable_b,
        .port = GPIOB,
        .pin  = 3,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
    { // FDI_GPIO_ATE_BAT_ADJ_EN
        .enable = fdi_gpio_clock_enable_c,
        .port = GPIOC,
        .pin  = 10,
        .mode = GPIO_MODE_OUTPUT_PP,
        .pull = GPIO_NOPULL,
        .speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .alternate = 0,
        .initially = true,
    },
};

#endif

const uint32_t fdi_gpio_count = (sizeof(fdi_gpios) / sizeof(fdi_gpio_t));

void fdi_gpio_initialize(void) {
    for (uint32_t i = 0; i < fdi_gpio_count; ++i) {
        fdi_gpio_t *gpio = &fdi_gpios[i];
        if (gpio->status != fdi_gpio_status_valid) {
            continue;
        }

        // enable gpio port clock
        gpio->enable();

        // initial output port value
        if (gpio->initially) {
            gpio->port->BSRR = 0x00000001 << gpio->pin;
        } else {
            gpio->port->BSRR = 0x00010000 << gpio->pin;
        }

        // initial port configuration
        GPIO_InitTypeDef GPIO_InitStruct;
        GPIO_InitStruct.Pin = 1 << gpio->pin;
        GPIO_InitStruct.Mode = gpio->mode;
        GPIO_InitStruct.Pull = gpio->pull;
        GPIO_InitStruct.Speed = gpio->speed;
        GPIO_InitStruct.Alternate = gpio->alternate;
        HAL_GPIO_Init(gpio->port, &GPIO_InitStruct);
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
    fd_log_assert(gpio->status != fdi_gpio_status_invalid);
    if (gpio->status != fdi_gpio_status_valid) {
        return;
    }
    gpio->port->BSRR = 0x00000001 << gpio->pin;
}

void fdi_gpio_off(uint32_t identifier) {
    fd_log_assert(identifier < fdi_gpio_count);

    fdi_gpio_t *gpio = &fdi_gpios[identifier];
    fd_log_assert(gpio->status != fdi_gpio_status_invalid);
    if (gpio->status != fdi_gpio_status_valid) {
        return;
    }
    gpio->port->BSRR = 0x00010000 << gpio->pin;
}

bool fdi_gpio_get(uint32_t identifier) {
    fd_log_assert(identifier < fdi_gpio_count);

    fdi_gpio_t *gpio = &fdi_gpios[identifier];
    fd_log_assert(gpio->status != fdi_gpio_status_invalid);
    if (gpio->status != fdi_gpio_status_valid) {
        return false;
    }
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
    fd_log_assert(gpio->status != fdi_gpio_status_invalid);
    if (gpio->status != fdi_gpio_status_valid) {
        return;
    }
    fdi_gpio_set_mode(gpio->port, gpio->pin, fdi_gpio_mode_input);
}

void fdi_gpio_set_mode_out(uint32_t identifier) {
    fd_log_assert(identifier < fdi_gpio_count);

    fdi_gpio_t *gpio = &fdi_gpios[identifier];
    fd_log_assert(gpio->status != fdi_gpio_status_invalid);
    if (gpio->status != fdi_gpio_status_valid) {
        return;
    }
    fdi_gpio_set_mode(gpio->port, gpio->pin, fdi_gpio_mode_output);
}

// optimized spi bit-bang transfer out -denis
void fdi_gpio_spi_out(uint32_t clock, uint32_t mosi, uint32_t miso __attribute((unused)), uint8_t *out, uint32_t length) {
    fdi_gpio_t *clock_gpio = &fdi_gpios[clock];
    fd_log_assert(clock_gpio->status == fdi_gpio_status_valid);
    __IO uint32_t *clock_address = &clock_gpio->port->BSRR;
    uint32_t clock_on_bit = 0x00000001 << clock_gpio->pin;
    uint32_t clock_off_bit = 0x00010000 << clock_gpio->pin;

    fdi_gpio_t *mosi_gpio = &fdi_gpios[mosi];
    fd_log_assert(mosi_gpio->status == fdi_gpio_status_valid);
    __IO uint32_t *mosi_address = &mosi_gpio->port->BSRR;
    uint32_t mosi_on_bit = 0x00000001 << mosi_gpio->pin;
    uint32_t mosi_off_bit = 0x00010000 << mosi_gpio->pin;

//    fdi_gpio_t *miso_gpio = &fdi_gpios[miso];
//    __IO uint32_t *miso_in_address = &miso_gpio->port->IDR;
//    uint32_t miso_shift = miso_gpio->pin;

    for (uint32_t i = 0; i < length; ++i) {
        uint8_t data = *out++;
//        uint8_t in = 0;
        for (int j = 0; j < 8; ++j) {
            *clock_address = clock_off_bit;
//            fdi_gpio_off(FDI_GPIO_SPI_SCLK);
            if (data & 0x80) {
                *mosi_address = mosi_on_bit;
//                fdi_gpio_on(FDI_GPIO_SPI_MOSI);
            } else {
                *mosi_address = mosi_off_bit;
//                fdi_gpio_off(FDI_GPIO_SPI_MOSI);
            }
            data = data << 1;
//            fdi_spi_delay();

            *clock_address = clock_on_bit;
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
    fd_log_assert(clock_gpio->status == fdi_gpio_status_valid);
    __IO uint32_t *clock_address = &clock_gpio->port->BSRR;
    uint32_t clock_on_bit = 0x00000001 << clock_gpio->pin;
    uint32_t clock_off_bit = 0x00010000 << clock_gpio->pin;

    fdi_gpio_t *mosi_gpio = &fdi_gpios[mosi];
    fd_log_assert(mosi_gpio->status == fdi_gpio_status_valid);
    __IO uint32_t *mosi_address = &mosi_gpio->port->BSRR;
    uint32_t mosi_on_bit = 0x00000001 << mosi_gpio->pin;
    uint32_t mosi_off_bit = 0x00010000 << mosi_gpio->pin;

    fdi_gpio_t *miso_gpio = &fdi_gpios[miso];
    fd_log_assert(miso_gpio->status == fdi_gpio_status_valid);
    __IO uint32_t *miso_address = &miso_gpio->port->IDR;
    uint32_t miso_shift = miso_gpio->pin;

    *mosi_address = mosi_off_bit;
    for (uint32_t i = 0; i < length; ++i) {
//        uint8_t data = *out++;
        uint8_t in_byte = 0;
        for (int j = 0; j < 8; ++j) {
            *clock_address = clock_off_bit;
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

            *clock_address = clock_on_bit;
            fdi_gpio_on(FDI_GPIO_SPI_SCLK);
//            fdi_spi_delay();
            in_byte = (in_byte << 1) | ((*miso_address >> miso_shift) & 0b1);
//            in = (in << 1) | fdi_gpio_get(FDI_GPIO_SPI_MISO);
        }
        *in++ = in_byte;
    }
}

#define fdi_serial_wire_half_bit_delay() fdi_delay_ns(1000)

void fdi_gpio_serial_wire_debug_out(uint32_t clock, uint32_t data, uint8_t *out, uint32_t length) {
    fdi_gpio_t *clock_gpio = &fdi_gpios[clock];
    fd_log_assert(clock_gpio->status == fdi_gpio_status_valid);
    __IO uint32_t *clock_address = &clock_gpio->port->BSRR;
    uint32_t clock_on_bit = 0x00000001 << clock_gpio->pin;
    uint32_t clock_off_bit = 0x00010000 << clock_gpio->pin;

    fdi_gpio_t *data_gpio = &fdi_gpios[data];
    fd_log_assert(data_gpio->status == fdi_gpio_status_valid);
    __IO uint32_t *data_address = &data_gpio->port->BSRR;
    uint32_t data_on_bit = 0x00000001 << data_gpio->pin;
    uint32_t data_off_bit = 0x00010000 << data_gpio->pin;

    for (uint32_t i = 0; i < length; ++i) {
        uint8_t byte = *out++;
        for (int j = 0; j < 8; ++j) {
//            fdi_serial_wire_half_bit_delay();
            *clock_address = clock_on_bit;
//            fdi_gpio_on(serial_wire->gpio_clock);
//            fdi_serial_wire_half_bit_delay();

            if (byte & 1) {
                *data_address = data_on_bit;
//                fdi_gpio_on(serial_wire->gpio_data);
            } else {
                *data_address = data_off_bit;
//                fdi_gpio_off(serial_wire->gpio_data);
            }
            byte >>= 1;

            *clock_address = clock_off_bit;
//            fdi_gpio_off(serial_wire->gpio_clock);
        }
    }
}

void fdi_gpio_serial_wire_debug_in(uint32_t clock, uint32_t data, uint8_t *in, uint32_t length) {
    fdi_gpio_t *clock_gpio = &fdi_gpios[clock];
    fd_log_assert(clock_gpio->status == fdi_gpio_status_valid);
    __IO uint32_t *clock_address = &clock_gpio->port->BSRR;
    uint32_t clock_on_bit = 0x00000001 << clock_gpio->pin;
    uint32_t clock_off_bit = 0x00010000 << clock_gpio->pin;

    fdi_gpio_t *data_gpio = &fdi_gpios[data];
    fd_log_assert(data_gpio->status == fdi_gpio_status_valid);
    __IO uint32_t *data_address = &data_gpio->port->IDR;
    uint32_t data_bit = 1 << data_gpio->pin;

    for (uint32_t i = 0; i < length; ++i) {
        uint8_t byte = 0;
        for (int j = 0; j < 8; ++j) {
//            fdi_serial_wire_half_bit_delay();
            *clock_address = clock_on_bit;
//            fdi_gpio_on(serial_wire->gpio_clock);
//            fdi_serial_wire_half_bit_delay();

            byte >>= 1;
            if (*data_address & data_bit) {
                byte |= 0b10000000;
            }

            *clock_address = clock_off_bit;
//            fdi_gpio_off(serial_wire->gpio_clock);
        }
        *in++ = byte;
    }
}
