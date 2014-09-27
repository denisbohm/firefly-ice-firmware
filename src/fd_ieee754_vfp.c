#include "fd_ieee754.h"

float fd_ieee754_uint16_to_float(uint16_t hfp) {
    float sfp;
    __asm(
    "fmsr s13,%1\n"
    "vcvtb.f32.f16 %0, s13\n"
    : "=w" (sfp)
    : "r" (hfp)
    );
    return sfp;
}

uint16_t fd_ieee754_float_to_uint16(float sfp) {
    uint32_t hfp;
    __asm(
    "vcvtb.f16.f32 s13, %1\n"
    "fmrs %0, s13\n"
    : "=r" (hfp)
    : "w" (sfp)
    );
    return (uint16_t)hfp;
}

