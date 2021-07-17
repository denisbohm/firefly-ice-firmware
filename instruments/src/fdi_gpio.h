#ifndef FDI_GPIO_H
#define FDI_GPIO_H

#include <stdbool.h>
#include <stdint.h>

#define FDI_GPIO_ATE_USB_CS_EN        0
#define FDI_GPIO_ATE_BS_EN            1

#define FDI_GPIO_ATE_USB_5V_EN        2
#define FDI_GPIO_ATE_USB_D_EN         3
#define FDI_GPIO_ATE_BATTERY_SENSE    4
#define FDI_GPIO_ATE_BUTTON_EN        5
#define FDI_GPIO_ATE_MCU_VCC_SENSE    6

#define FDI_GPIO_ATE_SWD1_DIR_TO_DUT  7
#define FDI_GPIO_ATE_SWD1_NRESET      8
#define FDI_GPIO_ATE_SWD1_SWDCLK      9
#define FDI_GPIO_ATE_SWD1_PWR        10
#define FDI_GPIO_ATE_SWD1_SWDIO      11

#define FDI_GPIO_ATE_SWD2_DIR_TO_DUT 12
#define FDI_GPIO_ATE_SWD2_NRESET     13
#define FDI_GPIO_ATE_SWD2_SWDCLK     14
#define FDI_GPIO_ATE_SWD2_PWR        15
#define FDI_GPIO_ATE_SWD2_SWDIO      16

#define FDI_GPIO_LED_R               17
#define FDI_GPIO_LED_G               18
#define FDI_GPIO_LED_B               19

#define FDI_GPIO_SPI_SCLK            20
#define FDI_GPIO_SPI_MOSI            21
#define FDI_GPIO_SPI_MISO            22

#define FDI_GPIO_S25FL116K_CSN       23

#define FDI_GPIO_ATE_SWD1_SENSE_NRESET 24
#define FDI_GPIO_ATE_SWD2_SENSE_NRESET 25

#define FDI_GPIO_ATE_FILL_EN    26
#define FDI_GPIO_ATE_DRAIN_EN   27
#define FDI_GPIO_ATE_BAT_CAP_EN 28
#define FDI_GPIO_ATE_BAT_ADJ_EN 29

#define FDI_GPIO_IOA0 30
#define FDI_GPIO_IOA1 31
#define FDI_GPIO_IOA2 32
#define FDI_GPIO_IOA3 33
#define FDI_GPIO_IOA4 34
#define FDI_GPIO_IOA5 35
#define FDI_GPIO_IOA6 36
#define FDI_GPIO_IOA7 37

#define FDI_GPIO_IOR0 38
#define FDI_GPIO_IOR1 39
#define FDI_GPIO_IOR2 40
#define FDI_GPIO_IOR3 41

#define FDI_GPIO_DIO0 42
#define FDI_GPIO_DIO1 43
#define FDI_GPIO_DIO2 44
#define FDI_GPIO_DIO3 45
#define FDI_GPIO_DIO4 46
#define FDI_GPIO_DIO5 47
#define FDI_GPIO_DIO6 48
#define FDI_GPIO_DIO7 49
#define FDI_GPIO_DIO8 50
#define FDI_GPIO_DIO9 51
#define FDI_GPIO_DIO10 52
#define FDI_GPIO_DIO11 53
#define FDI_GPIO_DIO12 54
#define FDI_GPIO_DIO13 55
#define FDI_GPIO_DIO14 56
#define FDI_GPIO_DIO15 57

#define FDI_GPIO_RIO0 58
#define FDI_GPIO_RIO1 59
#define FDI_GPIO_RIO2 60
#define FDI_GPIO_RIO3 61

#define FDI_GPIO_AIO0 62
#define FDI_GPIO_AIO1 63
#define FDI_GPIO_AIO2 64
#define FDI_GPIO_AIO3 65

typedef enum {
    fdi_gpio_mode_input,
    fdi_gpio_mode_output,
} fdi_gpio_mode_t;

void fdi_gpio_initialize(void);

void fdi_gpio_set(uint32_t identifier, bool value);
void fdi_gpio_on(uint32_t identifier);
void fdi_gpio_off(uint32_t identifier);

bool fdi_gpio_get(uint32_t identifier);

void fdi_gpio_set_mode(uint32_t identifier, fdi_gpio_mode_t mode);
void fdi_gpio_set_mode_input(uint32_t identifier);
void fdi_gpio_set_mode_output(uint32_t identifier);

// optimized spi bit-bang transfers -denis
void fdi_gpio_spi_out(uint32_t clock, uint32_t mosi, uint32_t miso, uint8_t *out, uint32_t length);
void fdi_gpio_spi_in(uint32_t clock, uint32_t mosi, uint32_t miso, uint8_t *in, uint32_t length);

// optimized swd bit-bang transfers -denis
void fdi_gpio_serial_wire_debug_out(uint32_t clock, uint32_t data, uint8_t *out, uint32_t length);
void fdi_gpio_serial_wire_debug_in(uint32_t clock, uint32_t data, uint8_t *in, uint32_t length);

#endif
