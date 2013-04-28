#include "fd_lis3dh.h"
#include "fd_log.h"
#include "fd_processor.h"
#include "fd_spi0.h"

#include <em_gpio.h>

#include <stdint.h>

#define LIS3DH_WHO_AM_I 0x0f

#define LIS3DH_CTRL_REG1 0x20
#define LIS3DH_CTRL_REG1_ODR_POWER_DOWN 0x00
#define LIS3DH_CTRL_REG1_ODR_1HZ 0x10
#define LIS3DH_CTRL_REG1_ODR_10HZ 0x20
#define LIS3DH_CTRL_REG1_ODR_25HZ 0x30
#define LIS3DH_CTRL_REG1_ODR_50HZ 0x40
#define LIS3DH_CTRL_REG1_ODR_100HZ 0x50
#define LIS3DH_CTRL_REG1_ODR_200HZ 0x60
#define LIS3DH_CTRL_REG1_ODR_400HZ 0x70
#define LIS3DH_CTRL_REG1_ODR_1600HZ 0x80
#define LIS3DH_CTRL_REG1_ODR_5000HZ 0x90
#define LIS3DH_CTRL_REG1_LPEN 0x08
#define LIS3DH_CTRL_REG1_ZEN 0x04
#define LIS3DH_CTRL_REG1_YEN 0x02
#define LIS3DH_CTRL_REG1_XEN 0x01

#define LIS3DH_CTRL_REG3 0x22
#define LIS3DH_CTRL_REG3_I1_CLICK 0x80
#define LIS3DH_CTRL_REG3_I1_AOI1 0x40
#define LIS3DH_CTRL_REG3_I1_AOI2 0x20
#define LIS3DH_CTRL_REG3_I1_DRDY1 0x10
#define LIS3DH_CTRL_REG3_I1_DRDY2 0x08
#define LIS3DH_CTRL_REG3_I1_WTM 0x04
#define LIS3DH_CTRL_REG3_I1_OVERRUN 0x02

#define LIS3DH_CTRL_REG4 0x23
#define LIS3DH_CTRL_REG4_BDU 0x80
#define LIS3DH_CTRL_REG4_BLE 0x40
#define LIS3DH_CTRL_REG4_FS_2G 0x00
#define LIS3DH_CTRL_REG4_FS_4G 0x10
#define LIS3DH_CTRL_REG4_FS_8G 0x20
#define LIS3DH_CTRL_REG4_FS_16G 0x30
#define LIS3DH_CTRL_REG4_HR 0x08
#define LIS3DH_CTRL_REG4_ST_NORMAL 0x00
#define LIS3DH_CTRL_REG4_ST_0 0x02
#define LIS3DH_CTRL_REG4_ST_1 0x06
#define LIS3DH_CTRL_REG4_SIM_3W 0x01

#define LIS3DH_CTRL_REG5 0x24
#define LIS3DH_CTRL_REG5_BOOT 0x80
#define LIS3DH_CTRL_REG5_FIFO_EN 0x40
#define LIS3DH_CTRL_REG5_LIR_INT1 0x08
#define LIS3DH_CTRL_REG5_D4D_INT1 0x04

#define LIS3DH_STATUS_REG 0x27
#define LIS3DH_STATUS_REG_ZYXOR 0x80
#define LIS3DH_STATUS_REG_ZOR 0x40
#define LIS3DH_STATUS_REG_YOR 0x20
#define LIS3DH_STATUS_REG_XOR 0x10
#define LIS3DH_STATUS_REG_ZYXDA 0x08
#define LIS3DH_STATUS_REG_ZDA 0x04
#define LIS3DH_STATUS_REG_YDA 0x02
#define LIS3DH_STATUS_REG_XDA 0x01

#define LIS3DH_FIFO_CTRL_REG 0x2e
#define LIS3DH_FIFO_CTRL_REG_FM_BYPASS 0x00
#define LIS3DH_FIFO_CTRL_REG_FM_FIFO 0x40
#define LIS3DH_FIFO_CTRL_REG_FM_STREAM 0x80
#define LIS3DH_FIFO_CTRL_REG_FM_TRIGGER 0xc0
#define LIS3DH_FIFO_CTRL_REG_TR_INT2 0x20

#define LIS3DH_OUT_X_L 0x28
#define LIS3DH_OUT_X_H 0x29
#define LIS3DH_OUT_Y_L 0x2a
#define LIS3DH_OUT_Y_H 0x2b
#define LIS3DH_OUT_Z_L 0x2c
#define LIS3DH_OUT_Z_H 0x2d

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
    uint8_t who_am_i = fd_spi0_sync_read(LIS3DH_WHO_AM_I);
    lis3dh_chip_deselect();
    if (who_am_i != 0x33) {
        fd_log_assert_fail("");
        return;
    }

    lis3dh_chip_select();
    fd_spi0_sync_write(
        LIS3DH_CTRL_REG1,
        LIS3DH_CTRL_REG1_ODR_100HZ |
        LIS3DH_CTRL_REG1_ZEN |
        LIS3DH_CTRL_REG1_YEN |
        LIS3DH_CTRL_REG1_XEN
    );
    lis3dh_chip_deselect();
    lis3dh_chip_select();
    fd_spi0_sync_write(
        LIS3DH_CTRL_REG4,
        LIS3DH_CTRL_REG4_BDU |
        LIS3DH_CTRL_REG4_FS_2G |
        LIS3DH_CTRL_REG4_HR
    );
    lis3dh_chip_deselect();
}

typedef union {
    uint8_t bytes[6];
    struct {
        int16_t x;
        int16_t y;
        int16_t z;
    };
} fd_lis3dh_out_t;

void fd_lis3dh_read(float *x, float *y, float *z) {
    fd_lis3dh_out_t out;
    lis3dh_chip_select();
    fd_spi0_sync_read_bytes(LIS3DH_OUT_X_L, out.bytes, sizeof(out));
    lis3dh_chip_deselect();
    *x = out.x / 16384.0f;
    *y = out.y / 16384.0f;
    *z = out.z / 16384.0f;
}