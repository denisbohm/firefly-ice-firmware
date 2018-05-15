#ifndef FD_BQ25120_H
#define FD_BQ25120_H

#include "fd_i2cm.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Registers */
#define FD_BQ25120_STATUS_SHIPMODE_REG    0x00
#define FD_BQ25120_FAULTS_FAULTMASKS_REG  0x01
#define FD_BQ25120_TSCONTROL_STATUS_REG   0x02
#define FD_BQ25120_FASTCHARGE_CTL_REG     0x03
#define FD_BQ25120_CHARGETERM_I2CADDR_REG 0x04
#define FD_BQ25120_BATT_VOLTAGE_CTL_REG   0x05
#define FD_BQ25120_SYSTEM_VOUT_CTL_REG    0x06
#define FD_BQ25120_LOADSW_LDO_CTL_REG     0x07
#define FD_BQ25120_PUSH_BTN_CTL_REG       0x08
#define FD_BQ25120_ILIMIT_UVLO_CTL_REG    0x09
#define FD_BQ25120_BATT_VOLT_MONITOR_REG  0x0A
#define FD_BQ25120_VIN_DPM_TIMER_REG      0x0B

/* Status and ShipMode Control Register (0x0) */
#define FD_BQ25120_STAT        (BIT(7) | BIT(6))
#define FD_BQ25120_STAT_SHIFT    0x06
#define FD_BQ25120_STAT_MASK    0x03
#define FD_BQ25120_EN_SHIPMODE    BIT(5)
#define FD_BQ25120_RESET        BIT(4)
#define FD_BQ25120_TIMER        BIT(3)
#define FD_BQ25120_VINDPM_STAT    BIT(2)
#define FD_BQ25120_NOT_CD_STAT    BIT(1)
#define FD_BQ25120_SYS_EN_STAT    BIT(0)

typedef union {
    struct {
        uint8_t sys_en_stat:1;
        uint8_t not_cd_stat:1;
        uint8_t vindpm_stat:1;
        uint8_t timer:1;
        uint8_t reset:1;
        uint8_t en_shipmode:1;
        uint8_t stat:2;
    } field;
    uint8_t byte;
} fd_bq25120_status_t;

/* Faults and Fault Masks Register (Ox1) */
#define FD_BQ25120_VIN_OV        BIT(7)
#define FD_BQ25120_VIN_UV        BIT(6)
#define FD_BQ25120_BAT_UVLO    BIT(5)
#define FD_BQ25120_BAT_OCP        BIT(4)
#define FD_BQ25120_FAULTS        (BIT(7) | BIT(6) | BIT(5) | BIT(4))
#define FD_BQ25120_FAULTS_SHIFT    0x04
#define FD_BQ25120_FAULTS_MASK    0x0F
#define FD_BQ25120_VIN_OV_M    BIT(3)
#define FD_BQ25120_VIN_OV_M_SHIFT    0x03
#define FD_BQ25120_VIN_OV_M_MASK    0x01
#define FD_BQ25120_VIN_UV_M    BIT(2)
#define FD_BQ25120_VIN_UV_M_SHIFT    0x02
#define FD_BQ25120_VIN_UV_M_MASK    0x01
#define FD_BQ25120_BAT_UVLO_M    BIT(1)
#define FD_BQ25120_BAT_UVLO_M_SHIFT    0x01
#define FD_BQ25120_BAT_UVLO_M_MASK    0x01
#define FD_BQ25120_BAT_OCP_M    BIT(0)
#define FD_BQ25120_BAT_OCP_M_SHIFT    0x0
#define FD_BQ25120_BAT_OCP_M_MASK    0x01

typedef union {
    struct {
        uint8_t bat_ocp_m:1;
        uint8_t bat_uvlo_m:1;
        uint8_t vin_uv_m:1;
        uint8_t vin_ov_m:1;
        uint8_t fault_bat_ocp:1;
        uint8_t fault_bat_uvlo:1;
        uint8_t fault_vin_uv:1;
        uint8_t fault_vin_ov:1;
    } field;
    uint8_t byte;
} fd_bq25120_faults_t;

/* TS Control and Status Mask Register (0x2) */
#define FD_BQ25120_TS_EN        BIT(7)
#define FD_BQ25120_TS_FAULT    (BIT(6) | BIT(5))
#define FD_BQ25120_TS_FAULT_SHIFT  0x05
#define FD_BQ25120_TS_FAULT_MASK    0X03
#define FD_BQ25120_TS_FAULT_OPEN    BIT(4)
#define FD_BQ25120_EN_INT        BIT(3)
#define FD_BQ25120_WAKE_M        BIT(2)
#define FD_BQ25120_WAKE_M_SHIFT    0x02
#define FD_BQ25120_WAKE_M_MASK    0x01
#define FD_BQ25120_RESET_M        BIT(1)
#define FD_BQ25120_RESET_SHIFT    0x01
#define FD_BQ25120_RESET_MASK    0x01
#define FD_BQ25120_TIMER_M        BIT(0)
#define FD_BQ25120_TIMER_M_SHIFT    0
#define FD_BQ25120_TIMER_M_MASK    0x01

typedef union {
    struct {
        uint8_t timer_m:1;
        uint8_t reset_m:1;
        uint8_t wake_m:1;
        uint8_t en_int:1;
        uint8_t ts_fault:2;
        uint8_t ts_en:1;
    } field;
    uint8_t byte;
} fd_bq25120_ts_control_t;

/* Fast Charge Control Register (0x03) */
#define FD_BQ25120_ICHRG_RANGE    BIT(7)
#define FD_BQ25120_ICHRG        (BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(2))
#define FD_BQ25120_ICHRG_SHIFT    0x02
#define FD_BQ25120_ICHRG_MASK    0x1F
#define FD_BQ25120_CE        BIT(1)
#define FD_BQ25120_HZ_MODE        BIT(0)

typedef union {
    struct {
        uint8_t hz_mode:1;
        uint8_t not_ce:1;
        uint8_t ichrg:5;
        uint8_t ichrg_range:1;
    } field;
    uint8_t byte;
} fd_bq25120_fast_charge_control_t;

/* Termination/Pre-Charge and I2C Address Register (0x4) */
#define FD_BQ25120_IPRETERM_RANGE    BIT(7)
#define FD_BQ25120_IPRETERM    (BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(2))
#define FD_BQ25120_IPRETERM_SHIFT  0x02
#define FD_BQ25120_IPRETERM_MASK   0x1F
#define FD_BQ25120_TE        BIT(1)
// Bit 0 Reserved

typedef union {
    struct {
        uint8_t reserved0:1;
        uint8_t ipreterm:5;
        uint8_t ipreterm_range:1;
    } field;
    uint8_t byte;
} fd_bq25120_termination_t;

/* Battery Voltage Control Register (0x05) */
#define FD_BQ25120_VBREG        (BIT(7) | BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1))
#define FD_BQ25120_VBREG_SHIFT    0x1
#define FD_BQ25120_VBREG_MASK    0x7F
// Bit 0 Reserved

typedef union {
    struct {
        uint8_t reserved0:1;
        uint8_t vbreg:7;
        uint8_t byte;
    } field;
    uint8_t byte;
} fd_bq25120_battery_voltage_control;

/* SYS VOUT Control Register (0x06) */
#define FD_BQ25120_EN_SYS_OUT    BIT(7)
#define FD_BQ25120_SYS_SEL        (BIT(6) | BIT(5))
#define FD_BQ25120_SYS_SEL_SHIFT    0x4
#define FD_BQ25120_SYS_MASK    0x3
#define FD_BQ25120_SYS_VOUT    (BIT(4) | BIT(3) | BIT(2) | BIT(1))
#define FD_BQ25120_SYS_VOUT_SHIFT    0x01
#define FD_BQ25120_SYS_VOUT_MASK    0x0F
//Bit 0 Reserved

typedef union {
    struct {
        uint8_t reserved0:1;
        uint8_t sys_vout:4;
        uint8_t sys_sel:2;
        uint8_t en_sys_out:1;
    } field;
    uint8_t byte;
} fd_bq25120_sys_t;

/* Load Switch and LDO Control Register (0x07) */
#define FD_BQ25120_EN_LS_LDO    BIT(7)
#define FD_BQ25120_LS_LDO        (BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(2))
#define FD_BQ25120_LS_LDO_SHIFT    0x2
#define FD_BQ25120_LS_LDO_MASK    0x1F
//Bit 1 Reserved
#define FD_BQ25120_MRRESET_VIN    BIT(0)

typedef union {
    struct {
        uint8_t mrreset_vin:1;
        uint8_t reserved1:1;
        uint8_t ls_ldo:5;
        uint8_t en_ls_ldo:1;
    } field;
    uint8_t byte;
} fd_bq25120_ldo_t;

/* Pushbutton Control Register (0x08) */
#define FD_BQ25120_MRWAKE1            BIT(7)
#define FD_BQ25120_MRWAKE2        BIT(6)
#define FD_BQ25120_MRREC        BIT(5)
#define FD_BQ25120_MRRESET        (BIT(4) | BIT(3))
#define FD_BQ25120_MRRESET_SHIFT    0x03
#define FD_BQ25120_MRRESET_MASK    0x03
#define FD_BQ25120_PGB_MRS        BIT(2)
#define FD_BQ25120_WAKE1        BIT(1)
#define FD_BQ25120_WAKE2        BIT(0)

typedef union {
    struct {
        uint8_t wake2:1;
        uint8_t wake1:1;
        uint8_t pgb_mrs:1;
        uint8_t mrreset:2;
        uint8_t mrrec:1;
        uint8_t mrwake2:1;
        uint8_t mrwake1:1;
    } field;
    uint8_t byte;
} fd_bq25120_pushbutton_control_t;

/* ILIM and Battery UVLO Control Register (0x09) */
#define FD_BQ25120_RESET_REG    BIT(7)
//Bit 6 Reserved
#define FD_BQ25120_INLIM        (BIT(5) | BIT(4) | BIT(3))
#define FD_BQ25120_INLIM_SHIFT    0x03
#define FD_BQ25120_INLIM_MASK    0x07
#define FD_BQ25120_BUVLO        (BIT(2) | BIT(1) | BIT(0))
#define FD_BQ25120_BUVLO_SHIFT    0x0
#define FD_BQ25120_BUVLO_MASK    0x7

typedef union {
    struct {
        uint8_t buvlo:3;
        uint8_t inlim:3;
        uint8_t reserved6:1;
        uint8_t reset_reg:1;
    } field;
    uint8_t byte;
} fd_bq25120_ilim_t;

/* Voltage Based Battery Monitor Register (0x0A) */
#define FD_BQ25120_VBMON_READ    BIT(7)
#define FD_BQ25120_VBMON_RANGE    (BIT(6) | BIT(5))
#define FD_BQ25120_VBMON_RANGE_SHIFT 0x05
#define FD_BQ25120_VBMON_RANGE_MASK  0x03
#define FD_BQ25120_VBMON_TH    (BIT(4) | BIT(3) | BIT(2))
#define FD_BQ25120_VBMON_TH_SHIFT    0x02
#define FD_BQ25120_VBMON_TH_MASK    0x07
//Bit 1 and 0 Reserved

typedef union {
    struct {
        uint8_t reserved0:1;
        uint8_t reserved1:1;
        uint8_t vbmon_th:3;
        uint8_t vbmon_range:2;
        uint8_t vbmon_read:1;
    } field;
    uint8_t byte;
} fd_bq25120_vbmon_t;

/* VIN_DPM and Timers Register (0x0B) */
#define FD_BQ25120_VINDPM_OFF    BIT(7)
#define FD_BQ25120_VINDPM        (BIT(6) | BIT(5) | BIT(4))
#define FD_BQ25120_VINDPM_SHIFT    0x04
#define FD_BQ25120_VINDPM_MASK    0x07
#define FD_BQ25120_2XTMR_EN    BIT(3)
#define FD_BQ25120_TMR        (BIT(2) | BIT(1))
#define FD_BQ25120_TMR_SHIFT    0x01
#define FD_BQ25120_TMR_MASK    0x03
#define FD_BQ25120_TMR_DISABLE    0x03
//Bit 0 Reserved

typedef union {
    struct {
        uint8_t reserved0:1;
        uint8_t tmr:2;
        uint8_t twoxtmr_en:1;
        uint8_t vindpm:3;
        uint8_t vindpm_off:1;
    } field;
    uint8_t byte;
} fd_bq25120_vin_dpm_t;

#define FD_BQ25120_I2C_ADDRESS 0xD4

bool fd_bq25120_read(const fd_i2cm_device_t *device, uint8_t location, uint8_t *byte);
bool fd_bq25120_write(const fd_i2cm_device_t *device, uint8_t location, uint8_t byte);

bool fd_bq25120_set_system_voltage(const fd_i2cm_device_t *device, float voltage);
bool fd_bq25120_read_battery_voltage(const fd_i2cm_device_t *device, float *battery_voltage);

#endif
