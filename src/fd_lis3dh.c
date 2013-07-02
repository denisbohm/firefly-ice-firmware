#include "fd_event.h"
#include "fd_lis3dh.h"
#include "fd_log.h"
#include "fd_processor.h"
#include "fd_spi.h"

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

#define FIFO_THRESHOLD 5

typedef union {
    uint8_t bytes[6];
    struct {
        int16_t x;
        int16_t y;
        int16_t z;
    };
} fd_lis3dh_out_t;

/*

static
fd_lis3dh_out_t transfer_out;

#define SPI_READ 0x80
#define SPI_ADDRESS_INCREMENT 0x40

static
uint8_t transfer_in = SPI_READ | SPI_ADDRESS_INCREMENT | LIS3DH_OUT_X_L;

static
fd_spi_transfer transfers[] = {
    {
        .op = fd_spi_op_write,
        .length = 1,
        .tx_buffer = &transfer_in,
        .rx_buffer = 0
    },
    {
        .op = fd_spi_op_read,
        .length = sizeof(transfer_out),
        .tx_buffer = 0,
        .rx_buffer = (uint8_t *)&transfer_out
    }
};

static
uint32_t fd_lis3dh_transfer_count;

static
fd_lish3dh_sample_callback_t fd_lish3dh_sample_callback;

void fd_lis3dh_set_sample_callback(fd_lish3dh_sample_callback_t callback) {
    fd_lish3dh_sample_callback = callback;
}

static
void fd_lis3dh_transfer_complete(void) {
    lis3dh_chip_deselect();

    int16_t x = transfer_out.x;
    int16_t y = transfer_out.y;
    int16_t z = transfer_out.z;

    if (fd_lis3dh_transfer_count > 0) {
        --fd_lis3dh_transfer_count;
        fd_spi1_io(ACC_CSN_PORT_PIN, transfers, sizeof(transfers) / sizeof(fd_spi_transfer), fd_lis3dh_transfer_complete);
    }

    if (fd_lish3dh_sample_callback) {
        (*fd_lish3dh_sample_callback)(x, y, z);
    }
}

static
void fd_lis3dh_interrupt(void) {
    fd_lis3dh_transfer_count = FIFO_THRESHOLD;
    fd_spi1_io(ACC_CSN_PORT_PIN, transfers, sizeof(transfers) / sizeof(fd_spi_transfer), fd_lis3dh_transfer_complete);
}

*/

#define SPI_READ 0x80
#define SPI_ADDRESS_INCREMENT 0x40

void fd_lis3dh_initialize(void) {
    uint8_t who_am_i = fd_spi_sync_tx1_rx1(FD_SPI_BUS_1_SLAVE_LIS3DH, SPI_READ | LIS3DH_WHO_AM_I);
    if (who_am_i != 0x33) {
        fd_log_assert_fail("");
        return;
    }

    fd_spi_sync_tx2(
        FD_SPI_BUS_1_SLAVE_LIS3DH,
        LIS3DH_CTRL_REG4,
        LIS3DH_CTRL_REG4_BDU |
        LIS3DH_CTRL_REG4_FS_2G |
        LIS3DH_CTRL_REG4_HR
    );

    fd_spi_sync_tx2(
        FD_SPI_BUS_1_SLAVE_LIS3DH,
        LIS3DH_FIFO_CTRL_REG,
        LIS3DH_FIFO_CTRL_REG_FM_BYPASS // clear FIFO
    );

    fd_spi_sync_tx2(
        FD_SPI_BUS_1_SLAVE_LIS3DH,
        LIS3DH_FIFO_CTRL_REG,
        LIS3DH_FIFO_CTRL_REG_FM_STREAM |
        FIFO_THRESHOLD
    );

    fd_spi_sync_tx2(
        FD_SPI_BUS_1_SLAVE_LIS3DH,
        LIS3DH_CTRL_REG3,
        LIS3DH_CTRL_REG3_I1_WTM |
        LIS3DH_CTRL_REG3_I1_OVERRUN
    );

    fd_spi_sync_tx2(
        FD_SPI_BUS_1_SLAVE_LIS3DH,
        LIS3DH_CTRL_REG5,
        LIS3DH_CTRL_REG5_FIFO_EN
    );

    fd_lis3dh_wake();

//    fd_event_set_callback(FD_EVENT_ACC_INT, fd_lis3dh_interrupt);
}

void fd_lis3dh_sleep(void) {
    fd_spi_sync_tx2(
        FD_SPI_BUS_1_SLAVE_LIS3DH,
        LIS3DH_CTRL_REG1,
        LIS3DH_CTRL_REG1_ODR_POWER_DOWN
    );
}

void fd_lis3dh_wake(void) {
    fd_spi_sync_tx2(
        FD_SPI_BUS_1_SLAVE_LIS3DH,
        LIS3DH_CTRL_REG1,
        LIS3DH_CTRL_REG1_ODR_25HZ |
        LIS3DH_CTRL_REG1_LPEN |
        LIS3DH_CTRL_REG1_ZEN |
        LIS3DH_CTRL_REG1_YEN |
        LIS3DH_CTRL_REG1_XEN
    );
}

void fd_lis3dh_read(float *x, float *y, float *z) {
    uint8_t tx_bytes[] = {SPI_READ | SPI_ADDRESS_INCREMENT | LIS3DH_OUT_X_L};
    fd_lis3dh_out_t out;
    fd_spi_sync_txn_rxn(
        FD_SPI_BUS_1_SLAVE_LIS3DH,
        tx_bytes, sizeof(tx_bytes),
        out.bytes, sizeof(out.bytes)
    );
    *x = out.x / 16384.0f;
    *y = out.y / 16384.0f;
    *z = out.z / 16384.0f;
}