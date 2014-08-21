#include "fd_event.h"
#include "fd_hal_processor.h"
#include "fd_lis3dh.h"
#include "fd_log.h"
#include "fd_spi.h"
#include "fd_timer.h"

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

#define LIS3DH_FIFO_SRC_REG 0x2f
#define LIS3DH_FIFO_SRC_REG_FSS 0x1f
#define LIS3DH_FIFO_SRC_REG_WTM 0x20
#define LIS3DH_FIFO_SRC_REG_OVRN_FIFO 0x40
#define LIS3DH_FIFO_SRC_REG_EMPTY 0x80

#define LIS3DH_OUT_X_L 0x28
#define LIS3DH_OUT_X_H 0x29
#define LIS3DH_OUT_Y_L 0x2a
#define LIS3DH_OUT_Y_H 0x2b
#define LIS3DH_OUT_Z_L 0x2c
#define LIS3DH_OUT_Z_H 0x2d

#define FIFO_THRESHOLD 16

typedef union {
    uint8_t bytes[6];
    struct __attribute__ ((packed)) {
        int16_t x;
        int16_t y;
        int16_t z;
    };
} fd_lis3dh_out_t;

#define SPI_READ 0x80
#define SPI_ADDRESS_INCREMENT 0x40

static fd_hal_accelerometer_sample_callback_t sample_callback;
static fd_timer_t fifo_timer;

static
void fd_lis3dh_schedule(void) {
    // Early board revisions use pin 4 (same as radio) so we can't use interrupts for fifo
    // and a timer is used instead.  At 25 samples per second and 32 deep fifo we check
    // the fifo every 1s at which point it should be about 75% full. -denis
    fd_time_t duration;
    duration.seconds = 1;
    duration.microseconds = 0;
    fd_timer_start(&fifo_timer, duration);
}

void fd_lis3dh_read_fifo(void) {
    uint8_t src = fd_spi_sync_tx1_rx1(FD_SPI_BUS_1_SLAVE_LIS3DH, SPI_READ | LIS3DH_FIFO_SRC_REG);
    uint8_t count = src & LIS3DH_FIFO_SRC_REG_FSS;
    while (count--) {
        uint8_t tx_bytes[] = {SPI_READ | SPI_ADDRESS_INCREMENT | LIS3DH_OUT_X_L};
        fd_lis3dh_out_t out;
        fd_spi_sync_txn_rxn(
            FD_SPI_BUS_1_SLAVE_LIS3DH,
            tx_bytes, sizeof(tx_bytes),
            out.bytes, sizeof(out.bytes)
        );
        if (sample_callback) {
            (*sample_callback)(out.x, out.y, out.z);
        }
    }

    fd_lis3dh_schedule();
}

void fd_lis3dh_initialize(void) {
    sample_callback = 0;

    uint8_t who_am_i = fd_spi_sync_tx1_rx1(FD_SPI_BUS_1_SLAVE_LIS3DH, SPI_READ | LIS3DH_WHO_AM_I);
    if (who_am_i != 0x33) {
        fd_log_assert_fail("");
        return;
    }

    fd_spi_sync_tx2(
        FD_SPI_BUS_1_SLAVE_LIS3DH,
        LIS3DH_CTRL_REG4,
        LIS3DH_CTRL_REG4_BDU |
        LIS3DH_CTRL_REG4_FS_8G |
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

    fd_event_add_callback(FD_EVENT_ACC_INT, fd_lis3dh_read_fifo);

    fd_timer_add(&fifo_timer, fd_lis3dh_read_fifo);
}

void fd_lis3dh_set_sample_callback(fd_hal_accelerometer_sample_callback_t callback) {
    sample_callback = callback;
}

void fd_lis3dh_sleep(void) {
    fd_timer_stop(&fifo_timer);

    fd_spi_sync_tx2(
        FD_SPI_BUS_1_SLAVE_LIS3DH,
        LIS3DH_CTRL_REG1,
        LIS3DH_CTRL_REG1_ODR_POWER_DOWN
    );
}

void fd_lis3dh_wake(void) {
    fd_lis3dh_schedule();

    fd_spi_sync_tx2(
        FD_SPI_BUS_1_SLAVE_LIS3DH,
        LIS3DH_CTRL_REG1,
        LIS3DH_CTRL_REG1_ODR_25HZ |
//        LIS3DH_CTRL_REG1_LPEN |
        LIS3DH_CTRL_REG1_ZEN |
        LIS3DH_CTRL_REG1_YEN |
        LIS3DH_CTRL_REG1_XEN
    );
}

void fd_lis3dh_read(int16_t *x, int16_t *y, int16_t *z) {
    uint8_t tx_bytes[] = {SPI_READ | SPI_ADDRESS_INCREMENT | LIS3DH_OUT_X_L};
    fd_lis3dh_out_t out;
    fd_spi_sync_txn_rxn(
        FD_SPI_BUS_1_SLAVE_LIS3DH,
        tx_bytes, sizeof(tx_bytes),
        out.bytes, sizeof(out.bytes)
    );
    *x = out.x;
    *y = out.y;
    *z = out.z;
}