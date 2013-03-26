#include "fd_adc.h"
#include "fd_processor.h"
#include "fd_spi0.h"
#include "fd_spi1.h"

void main(void) {
    fd_processor_initialize();

    fd_adc_initialize();
    fd_i2c1_initialize();
    fd_spi0_initialize();
    fd_spi1_initialize();
}