#ifndef FD_LSM6DSL_H
#define FD_LSM6DSL_H

#include "fd_spim.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} fd_lsm6dsl_accelerometer_sample_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} fd_lsm6dsl_gyro_sample_t;

typedef struct {
    fd_lsm6dsl_accelerometer_sample_t accelerometer;
    fd_lsm6dsl_gyro_sample_t gyro;
    uint32_t steps;
    uint32_t timestamp;
} fd_lsm6dsl_sample_t;

#define FD_LSM6DSL_REGISTER_FUNC_CFG_ACCESS 0x01
#define FD_LSM6DSL_REGISTER_SENSOR_SYNC_TIME_FRAME 0x04
#define FD_LSM6DSL_REGISTER_SENSOR_SYNC_RES_RATIO 0x05
#define FD_LSM6DSL_REGISTER_FIFO_CTRL1 0x06
#define FD_LSM6DSL_REGISTER_FIFO_CTRL2 0x07
#define FD_LSM6DSL_REGISTER_FIFO_CTRL3 0x08
#define FD_LSM6DSL_REGISTER_FIFO_CTRL4 0x09
#define FD_LSM6DSL_REGISTER_FIFO_CTRL5 0x0a
#define FD_LSM6DSL_REGISTER_DRDY_PULSE_CFG_G 0x0b
#define FD_LSM6DSL_REGISTER_INT1_CTRL 0x0d
#define FD_LSM6DSL_REGISTER_INT2_CTRL 0x0e
#define FD_LSM6DSL_REGISTER_WHO_AM_I 0x0f
#define FD_LSM6DSL_REGISTER_CTRL1_XL 0x10
#define FD_LSM6DSL_REGISTER_CTRL2_G 0x11
#define FD_LSM6DSL_REGISTER_CTRL3_C 0x12
#define FD_LSM6DSL_REGISTER_CTRL4_C 0x13
#define FD_LSM6DSL_REGISTER_CTRL5_C 0x14
#define FD_LSM6DSL_REGISTER_CTRL6_C 0x15
#define FD_LSM6DSL_REGISTER_CTRL7_G 0x16
#define FD_LSM6DSL_REGISTER_CTRL8_XL 0x17
#define FD_LSM6DSL_REGISTER_CTRL9_XL 0x18
#define FD_LSM6DSL_REGISTER_CTRL10_C 0x19
#define FD_LSM6DSL_REGISTER_MASTER_CONFIG 0x1a
#define FD_LSM6DSL_REGISTER_WAKE_UP_SRC 0x1b
#define FD_LSM6DSL_REGISTER_TAP_SRC 0x1c
#define FD_LSM6DSL_REGISTER_D6D_SRC 0x1d
#define FD_LSM6DSL_REGISTER_STATUS_REG 0x1e
#define FD_LSM6DSL_REGISTER_OUT_TEMP_L 0x20
#define FD_LSM6DSL_REGISTER_OUT_TEMP_H 0x21
#define FD_LSM6DSL_REGISTER_OUTX_L_G 0x22
#define FD_LSM6DSL_REGISTER_OUTX_H_G 0x23
#define FD_LSM6DSL_REGISTER_OUTY_L_G 0x24
#define FD_LSM6DSL_REGISTER_OUTY_H_G 0x25
#define FD_LSM6DSL_REGISTER_OUTZ_L_G 0x26
#define FD_LSM6DSL_REGISTER_OUTZ_H_G 0x27
#define FD_LSM6DSL_REGISTER_OUTX_L_XL 0x28
#define FD_LSM6DSL_REGISTER_OUTX_H_XL 0x29
#define FD_LSM6DSL_REGISTER_OUTY_L_XL 0x2a
#define FD_LSM6DSL_REGISTER_OUTY_H_XL 0x2b
#define FD_LSM6DSL_REGISTER_OUTZ_L_XL 0x2c
#define FD_LSM6DSL_REGISTER_OUTZ_H_XL 0x2d
#define FD_LSM6DSL_REGISTER_SENSORHUB1_REG 0x2e
#define FD_LSM6DSL_REGISTER_SENSORHUB2_REG 0x2f
#define FD_LSM6DSL_REGISTER_SENSORHUB3_REG 0x30
#define FD_LSM6DSL_REGISTER_SENSORHUB4_REG 0x31
#define FD_LSM6DSL_REGISTER_SENSORHUB5_REG 0x32
#define FD_LSM6DSL_REGISTER_SENSORHUB6_REG 0x33
#define FD_LSM6DSL_REGISTER_SENSORHUB7_REG 0x34
#define FD_LSM6DSL_REGISTER_SENSORHUB8_REG 0x35
#define FD_LSM6DSL_REGISTER_SENSORHUB9_REG 0x36
#define FD_LSM6DSL_REGISTER_SENSORHUB10_REG 0x37
#define FD_LSM6DSL_REGISTER_SENSORHUB11_REG 0x38
#define FD_LSM6DSL_REGISTER_SENSORHUB12_REG 0x39
#define FD_LSM6DSL_REGISTER_FIFO_STATUS1 0x3a
#define FD_LSM6DSL_REGISTER_FIFO_STATUS2 0x3b
#define FD_LSM6DSL_REGISTER_FIFO_STATUS3 0x3c
#define FD_LSM6DSL_REGISTER_FIFO_STATUS4 0x3d
#define FD_LSM6DSL_REGISTER_FIFO_DATA_OUT_L 0x3e
#define FD_LSM6DSL_REGISTER_FIFO_DATA_OUT_H 0x3f
#define FD_LSM6DSL_REGISTER_TIMESTAMP0_REG 0x40
#define FD_LSM6DSL_REGISTER_TIMESTAMP1_REG 0x41
#define FD_LSM6DSL_REGISTER_TIMESTAMP2_REG 0x42
#define FD_LSM6DSL_REGISTER_STEP_TIMESTAMP_L 0x49
#define FD_LSM6DSL_REGISTER_STEP_TIMESTAMP_H 0x4a
#define FD_LSM6DSL_REGISTER_STEP_COUNTER_L 0x4b
#define FD_LSM6DSL_REGISTER_STEP_COUNTER_H 0x4c
#define FD_LSM6DSL_REGISTER_SENSORHUB13_REG 0x4d
#define FD_LSM6DSL_REGISTER_SENSORHUB14_REG 0x4e
#define FD_LSM6DSL_REGISTER_SENSORHUB15_REG 0x4f
#define FD_LSM6DSL_REGISTER_SENSORHUB16_REG 0x50
#define FD_LSM6DSL_REGISTER_SENSORHUB17_REG 0x51
#define FD_LSM6DSL_REGISTER_SENSORHUB18_REG 0x52
#define FD_LSM6DSL_REGISTER_FUNC_SRC1 0x53
#define FD_LSM6DSL_REGISTER_FUNC_SRC2 0x54
#define FD_LSM6DSL_REGISTER_WRIST_TILT_IA 0x55
#define FD_LSM6DSL_REGISTER_TAP_CFG 0x58
#define FD_LSM6DSL_REGISTER_TAP_THS_6D 0x59
#define FD_LSM6DSL_REGISTER_INT_DUR2 0x5a
#define FD_LSM6DSL_REGISTER_WAKE_UP_THS 0x5b
#define FD_LSM6DSL_REGISTER_WAKE_UP_DUR 0x5c
#define FD_LSM6DSL_REGISTER_FREE_FALL 0x5d
#define FD_LSM6DSL_REGISTER_MD1_CFG 0x5e
#define FD_LSM6DSL_REGISTER_MD2_CFG 0x5f
#define FD_LSM6DSL_REGISTER_MASTER_CMD_CODE 0x60
#define FD_LSM6DSL_REGISTER_SENS_SYNC_SPI_ERROR_CODE 0x61
#define FD_LSM6DSL_REGISTER_OUT_MAG_RAW_X_L 0x66
#define FD_LSM6DSL_REGISTER_OUT_MAG_RAW_X_H 0x67
#define FD_LSM6DSL_REGISTER_OUT_MAG_RAW_Y_L 0x68
#define FD_LSM6DSL_REGISTER_OUT_MAG_RAW_Y_H 0x69
#define FD_LSM6DSL_REGISTER_OUT_MAG_RAW_Z_L 0x6a
#define FD_LSM6DSL_REGISTER_OUT_MAG_RAW_Z_H 0x6b
#define FD_LSM6DSL_REGISTER_X_OFS_USR 0x73
#define FD_LSM6DSL_REGISTER_Y_OFS_USR 0x74
#define FD_LSM6DSL_REGISTER_Z_OFS_USR 0x75

#define FD_LSM6DSL_REGISTER_FUNC_CONFIG_PEDO_THS_MIN 0x0f

#define FD_LSM6DSL_WHO_AM_I_VALUE 0x6A

#define FD_LSM6DSLSTATUS_XLDA 0x01
#define FD_LSM6DSLSTATUS_GDA 0x02
#define FD_LSM6DSLSTATUS_TDA 0x04

#define FD_LSM6DSL_ODR_POWER_DOWN 0b0000
#define FD_LSM6DSL_ODR_13_HZ      0b0001
#define FD_LSM6DSL_ODR_26_HZ      0b0010
#define FD_LSM6DSL_ODR_52_HZ      0b0011
#define FD_LSM6DSL_ODR_104_HZ     0b0100
#define FD_LSM6DSL_ODR_208_HZ     0b0101
#define FD_LSM6DSL_ODR_416_HZ     0b0110
#define FD_LSM6DSL_ODR_833_HZ     0b0111
#define FD_LSM6DSL_ODR_1660_HZ    0b1000
#define FD_LSM6DSL_ODR_3330_HZ    0b1001
#define FD_LSM6DSL_ODR_6660_HZ    0b1010

#define FD_LSM6DSL_XFS_2_G  0b00
#define FD_LSM6DSL_XFS_4_G  0b10
#define FD_LSM6DSL_XFS_8_G  0b11
#define FD_LSM6DSL_XFS_16_G 0b01

#define FD_LSM6DSL_XBWF_50_HZ  0b11
#define FD_LSM6DSL_XBWF_100_HZ 0b10
#define FD_LSM6DSL_XBWF_200_HZ 0b01
#define FD_LSM6DSL_XBWF_400_HZ 0b00

#define FD_LSM6DSL_GFS_125_DPS  0b001
#define FD_LSM6DSL_GFS_245_DPS  0b000
#define FD_LSM6DSL_GFS_500_DPS  0b010
#define FD_LSM6DSL_GFS_1000_DPS 0b100
#define FD_LSM6DSL_GFS_2000_DPS 0b110

#define FD_LSM6DSL_GHPF_DISABLED_HZ 0b000
#define FD_LSM6DSL_GHPF_P0081_HZ    0b100
#define FD_LSM6DSL_GHPF_P0324_HZ    0b101
#define FD_LSM6DSL_GHPF_2P07_HZ     0b110
#define FD_LSM6DSL_GHPF_16P32_HZ    0b111

#define FD_LSM6DSL_INT1_STEP_DETECTOR 0b10000000
#define FD_LSM6DSL_INT1_SIGN_MOT      0b01000000
#define FD_LSM6DSL_INT1_FULL_FLAG     0b00100000
#define FD_LSM6DSL_INT1_FIFO_OVR      0b00010000
#define FD_LSM6DSL_INT1_FTH           0b00001000
#define FD_LSM6DSL_INT1_BOOT          0b00000100
#define FD_LSM6DSL_INT1_DRDY_G        0b00000010
#define FD_LSM6DSL_INT1_DRDY_XL       0b00000001

#define FD_LSM6DSL_FIFO_STATUS2_WATERM          0b10000000
#define FD_LSM6DSL_FIFO_STATUS2_OVER_RUN        0b01000000
#define FD_LSM6DSL_FIFO_STATUS2_FIFO_FULL_SMART 0b00100000
#define FD_LSM6DSL_FIFO_STATUS2_EMPTY           0b00010000

#define FD_LSM6DSL_READ 0x80

float fd_lsm6dsl_accelerometer_scale(uint32_t fs);
float fd_lsm6dsl_gyro_scale(uint32_t fs);

uint8_t fd_lsm6dsl_read(const fd_spim_device_t *device, uint8_t location);
void fd_lsm6dsl_write(const fd_spim_device_t *device, uint8_t location, uint8_t byte);

typedef struct {
    uint16_t fifo_threshold;
    uint8_t fifo_output_data_rate;
    uint8_t accelerometer_output_data_rate;
    bool accelerometer_low_power;
    uint8_t accelerometer_full_scale_range;
    uint8_t accelerometer_bandwidth_filter;
    bool accelerometer_enable;
    uint8_t gyro_output_data_rate;
    bool gyro_low_power;
    uint8_t gyro_full_scale_range;
    uint8_t gyro_high_pass_filter;
    bool gyro_enable;
    bool timestamp_and_steps_enable;
    uint8_t steps_threshold;
} fd_lsm6dsl_configuration_t;

void fd_lsm6ds3_configure(const fd_spim_device_t *device, const fd_lsm6dsl_configuration_t *configuration);

void fd_lsm6dsl_fifo_flush(const fd_spim_device_t *device);
uint32_t fd_lsm6dsl_read_fifo_word_count(const fd_spim_device_t *device);
uint32_t fd_lsm6dsl_read_fifo_samples(const fd_spim_device_t *device, fd_lsm6dsl_sample_t *samples, uint32_t sample_count);

uint32_t fd_lsm6dsl_get_step_count(const fd_spim_device_t *device);
void fd_lsm6dsl_reset_step_counter(const fd_spim_device_t *device);

bool fd_lsm6dsl_self_test(const fd_spim_device_t *device);

#endif
