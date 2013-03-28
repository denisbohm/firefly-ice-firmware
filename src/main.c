#include "fd_adc.h"
#include "fd_i2c1.h"
#include "fd_lis3dh.h"
#include "fd_nrf8001.h"
#include "fd_processor.h"
#include "fd_spi0.h"
#include "fd_spi1.h"

#include <stdbool.h>

void main(void) {
    fd_processor_initialize();

    fd_adc_initialize();
    fd_i2c1_initialize();
    fd_spi0_initialize();
    fd_spi1_initialize();

    fd_lis3dh_initialize();
    fd_nrf8001_initialize();

    fd_nrf8001_power_on();
    while (true) {
        fd_nrf8001_transfer();
    }
}