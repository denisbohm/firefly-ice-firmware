#ifndef FD_ADCM_H
#define FD_ADCM_H

#include "fd_gpio.h"

void fd_adcm_initialize(void);
float fd_adcm_convert(fd_gpio_t gpio, float max_voltage);

#endif
