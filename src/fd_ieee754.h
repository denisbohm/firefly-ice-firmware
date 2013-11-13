#ifndef FD_IEEE754_H
#define FD_IEEE754_H

#include <stdint.h>

uint16_t fd_ieee754_float_to_uint16(float value);
float fd_ieee754_uint16_to_float(uint16_t value);

#endif