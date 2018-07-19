#include "fdi_gpio.h"

#include "fdi_serial_wire.h"

#include "fd_delay.h"
#include "fd_gpio.h"

#include "fd_nrf5.h"

typedef struct {
    bool valid;
    NRF_GPIO_Type *port;
    uint8_t pin;
    bool output;
    bool initial_value;
} fdi_gpio_t;

#define fdi_gpio_count (sizeof(fdi_gpios) / sizeof(fdi_gpios[0]))

static fdi_gpio_t fdi_gpios[] = {
    { .valid = false }, // FDI_GPIO_ATE_USB_CS_EN        0
    { .valid = false }, // FDI_GPIO_ATE_BS_EN            1

    { .valid = false }, // FDI_GPIO_ATE_USB_5V_EN        2
    { .valid = false }, // FDI_GPIO_ATE_USB_D_EN         3
    { .valid = false }, // FDI_GPIO_ATE_BATTERY_SENSE    4
    { .valid = false }, // FDI_GPIO_ATE_BUTTON_EN        5
    { .valid = false }, // FDI_GPIO_ATE_MCU_VCC_SENSE    6

    { // FDI_GPIO_ATE_SWD1_DIR_TO_DUT  7
        .valid = true,
        .port = NRF_P0,
        .pin = 0,
        .output = false,
        .initial_value = false,
    },
    { // FDI_GPIO_ATE_SWD1_NRESET      8
        .valid = true,
        .port = NRF_P0,
        .pin = 0,
        .output = false,
        .initial_value = false,
    },
    { // FDI_GPIO_ATE_SWD1_SWDCLK      9
        .valid = true,
        .port = NRF_P0,
        .pin = 0,
        .output = false,
        .initial_value = false,
    },
    { // FDI_GPIO_ATE_SWD1_PWR        10
        .valid = true,
        .port = NRF_P0,
        .pin = 0,
        .output = false,
        .initial_value = false,
    },
    { // FDI_GPIO_ATE_SWD1_SWDIO      11
        .valid = true,
        .port = NRF_P0,
        .pin = 0,
        .output = false,
        .initial_value = false,
    },

    { .valid = false }, // FDI_GPIO_ATE_SWD2_DIR_TO_DUT 12
    { .valid = false }, // FDI_GPIO_ATE_SWD2_NRESET     13
    { .valid = false }, // FDI_GPIO_ATE_SWD2_SWDCLK     14
    { .valid = false }, // FDI_GPIO_ATE_SWD2_PWR        15
    { .valid = false }, // FDI_GPIO_ATE_SWD2_SWDIO      16

    { .valid = false }, // FDI_GPIO_LED_R               17
    { .valid = false }, // FDI_GPIO_LED_G               18
    { .valid = false }, // FDI_GPIO_LED_B               19

    { .valid = false }, // FDI_GPIO_SPI_SCLK            20
    { .valid = false }, // FDI_GPIO_SPI_MOSI            21
    { .valid = false }, // FDI_GPIO_SPI_MISO            22

    { .valid = false }, // FDI_GPIO_S25FL116K_CSN       23

    { .valid = false }, // FDI_GPIO_ATE_SWD1_SENSE_NRESET 24
    { .valid = false }, // FDI_GPIO_ATE_SWD2_SENSE_NRESET 25

    { .valid = false }, // FDI_GPIO_ATE_FILL_EN    26
    { .valid = false }, // FDI_GPIO_ATE_DRAIN_EN   27
    { .valid = false }, // FDI_GPIO_ATE_BAT_CAP_EN 28
    { .valid = false }, // FDI_GPIO_ATE_BAT_ADJ_EN 29
};

static inline
NRF_GPIO_Type *fdi_gpio_get_nrf_gpio(uint32_t port) {
    return (NRF_GPIO_Type *)(NRF_P0_BASE + port * 0x300UL);
}

static
void fdi_gpio_configure(
    uint32_t identifier,
    nrf_gpio_pin_dir_t dir,
    nrf_gpio_pin_input_t input,
    nrf_gpio_pin_pull_t pull,
    nrf_gpio_pin_drive_t drive,
    nrf_gpio_pin_sense_t sense
) {
    fdi_gpio_t gpio = fdi_gpios[identifier];
    NRF_GPIO_Type *nrf_gpio = gpio.port;
    nrf_gpio->PIN_CNF[gpio.pin] = ((uint32_t)dir << GPIO_PIN_CNF_DIR_Pos)
                                | ((uint32_t)input << GPIO_PIN_CNF_INPUT_Pos)
                                | ((uint32_t)pull << GPIO_PIN_CNF_PULL_Pos)
                                | ((uint32_t)drive << GPIO_PIN_CNF_DRIVE_Pos)
                                | ((uint32_t)sense << GPIO_PIN_CNF_SENSE_Pos);
}

void fdi_gpio_initialize(void) {
    for (int i = 0; i < fdi_gpio_count; ++i) {
        fdi_gpio_t *gpio = &fdi_gpios[i];
        if (!gpio->valid) {
            continue;
        }
        if (gpio->output) {
            fdi_gpio_set_mode_out(i);
            fdi_gpio_set(i, gpio->initial_value);
        } else {
            fdi_gpio_set_mode_in(i);
        }
    }
}

void fdi_gpio_set(uint32_t identifier, bool value) {
    fdi_gpio_t gpio = fdi_gpios[identifier];
    NRF_GPIO_Type *nrf_gpio = gpio.port;
    if (value) {
        nrf_gpio->OUTSET = 1UL << gpio.pin;
    } else {
        nrf_gpio->OUTCLR = 1UL << gpio.pin;
    }
}

void fdi_gpio_on(uint32_t identifier) {
    fdi_gpio_set(identifier, true);
}

void fdi_gpio_off(uint32_t identifier) {
    fdi_gpio_set(identifier, false);
}

bool fdi_gpio_get(uint32_t identifier) {
    fdi_gpio_t gpio = fdi_gpios[identifier];
    NRF_GPIO_Type *nrf_gpio = gpio.port;
    return ((nrf_gpio->IN >> gpio.pin) & 1UL) != 0;
}

void fdi_gpio_set_mode_in(uint32_t identifier) {
    fdi_gpio_configure(identifier,
        NRF_GPIO_PIN_DIR_INPUT,
        NRF_GPIO_PIN_INPUT_CONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE
    );
}

void fdi_gpio_set_mode_out(uint32_t identifier) {
    fdi_gpio_configure(identifier,
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_DISCONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE
    );
}

#define fdi_gpio_serial_wire_half_bit_delay() fd_delay_ns(100)

void fdi_gpio_serial_wire_shift_out(uint32_t clock, uint32_t data, uint8_t byte, int bit_count) {
    while (bit_count-- > 0) {
        fdi_gpio_serial_wire_half_bit_delay();
        fdi_gpio_on(clock);
        fdi_gpio_serial_wire_half_bit_delay();

        if (byte & 1) {
            fdi_gpio_on(data);
        } else {
            fdi_gpio_off(data);
        }
        byte >>= 1;

        fdi_gpio_off(clock);
    }
}

void fdi_gpio_serial_wire_debug_out(uint32_t clock, uint32_t data, uint8_t *out, uint32_t length) {
    for (uint32_t i = 0; i < length; ++i) {
        fdi_gpio_serial_wire_shift_out(clock, data, out[i], 8);
    }
}

uint8_t fdi_gpio_serial_wire_shift_in(uint32_t clock, uint32_t data, int bit_count) {
    uint8_t byte = 0;
    while (bit_count-- > 0) {
        fdi_gpio_serial_wire_half_bit_delay();
        fdi_gpio_on(clock);
        fdi_gpio_serial_wire_half_bit_delay();

        byte >>= 1;
        if (fdi_gpio_get(data)) {
            byte |= 0b10000000;
        }

        fdi_gpio_off(clock);
    }
    return byte;
}

void fdi_gpio_serial_wire_debug_in(uint32_t clock, uint32_t data, uint8_t *in, uint32_t length) {
    for (uint32_t i = 0; i < length; ++i) {
        in[i] = fdi_gpio_serial_wire_shift_in(clock, data, 8);
    }
}
