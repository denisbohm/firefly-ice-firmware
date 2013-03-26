#include "fd_i2c1.h"

#include <em_i2c.h>

void fd_i2c1_initialize(void) {
    I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;
    I2C_Init(I2C0, &i2cInit);
}