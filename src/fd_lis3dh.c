#include "fd_lis3dh.h"
#include "fd_processor.h"
#include "fd_spi0.h"

#include "em_gpio.h"

#include <stdint.h>

#define LIS3DH_READ 0x80
#define LIS3DH_INCREMENT 0x40

#define LIS3DH_WHO_AM_I 0x0f

static
void lis3dh_chip_select(void) {
    GPIO_PinOutClear(ACC_CSN_PORT_PIN);
}

static
void lis3dh_chip_deselect(void) {
    GPIO_PinOutSet(ACC_CSN_PORT_PIN);
}

void fd_lis3dh_initialize(void) {
    lis3dh_chip_select();
    uint8_t who_am_i = fd_spi0_read(LIS3DH_WHO_AM_I);
    lis3dh_chip_deselect();
    if (who_am_i != 0x33) {
        // log diagnostic
        return;
    }
}