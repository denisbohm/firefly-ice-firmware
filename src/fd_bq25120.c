#include "fd_bq25120.h"

struct fd_bq25120_reg_default {
    unsigned int reg;
    unsigned int def;
};

struct fd_bq25120_reg_default fd_bq25120_reg_defaults[] = {
    {FD_BQ25120_STATUS_SHIPMODE_REG, 0x01},
    {FD_BQ25120_FAULTS_FAULTMASKS_REG, 0x00},
    {FD_BQ25120_TSCONTROL_STATUS_REG, 0x88},
    {FD_BQ25120_FASTCHARGE_CTL_REG, 0x14},
    {FD_BQ25120_CHARGETERM_I2CADDR_REG, 0x0E},
    {FD_BQ25120_BATT_VOLTAGE_CTL_REG, 0x78},
    {FD_BQ25120_SYSTEM_VOUT_CTL_REG, 0xAA},
    {FD_BQ25120_LOADSW_LDO_CTL_REG, 0x7C},
    {FD_BQ25120_PUSH_BTN_CTL_REG, 0x68},
    {FD_BQ25120_ILIMIT_UVLO_CTL_REG, 0x0A},
    {FD_BQ25120_BATT_VOLT_MONITOR_REG, 0x00},
    {FD_BQ25120_VIN_DPM_TIMER_REG, 0x4A},
};

enum fd_bq25120_charge_fault {
    BQ25120_NORMAL = 0,
    BQ25120_CH_VIN_OV,
    BQ25120_CH_VIN_UV,
    BQ25120_BATTERY_UVLO,
    BQ25120_BATTERY_OCP,
};

enum fd_bq25120_temp_status {
    BQ25120_TEMP_NORMAL = 0,
    BQ25120_TEMP_TNTC,
    BQ25120_TEMP_TCOOL,
    BQ25120_TEMP_TWARM,
};

bool fd_bq25120_read(const fd_i2cm_device_t *device, uint8_t location, uint8_t *byte) {
    return fd_i2cm_device_sequence_tx1_rx1(device, location, byte);
}

bool fd_bq25120_write(const fd_i2cm_device_t *device, uint8_t location, uint8_t byte) {
    return fd_i2cm_device_sequence_tx1_tx1(device, location, byte);
}

bool fd_bq25120_set_system_voltage(const fd_i2cm_device_t *device, float voltage) {
    // If SYS_SEL = 11, then SYS = 1.80 V + SYS_VOUTCODE x 100 mV. (1.8 to 3.3 V)
    int vout = (int)((voltage * 10.0f) + 0.05f);
    if (vout < 18) {
        vout = 18;
    }
    if (vout > 33) {
        vout = 33;
    }
    fd_bq25120_sys_t sys = {
        .field = {
            .sys_vout = vout - 18,
            .sys_sel = 0b11,
            .en_sys_out = 1,
        }
    };
    return fd_bq25120_write(device, FD_BQ25120_SYSTEM_VOUT_CTL_REG, sys.byte);
}

bool fd_bq25120_read_battery_voltage(const fd_i2cm_device_t *device, float *battery_voltage) {
    *battery_voltage = 0.0f;
    fd_bq25120_battery_voltage_control bv;
    if (!fd_bq25120_read(device, FD_BQ25120_BATT_VOLTAGE_CTL_REG, &bv.byte)) {
        return false;
    }
    float battery_regulation_voltage = 3.6f + bv.field.vbreg / 100.0f;
    fd_bq25120_vbmon_t vbmon;
    if (!fd_bq25120_read(device, FD_BQ25120_BATT_VOLT_MONITOR_REG, &vbmon.byte)) {
        return false;
    }
    float amount = 0.6f + 0.1f * vbmon.field.vbmon_range;
    switch (vbmon.field.vbmon_th) {
        case 0b001:
            amount += 0.02f;
            break;
        case 0b010:
            amount += 0.04f;
            break;
        case 0b011:
            amount += 0.06f;
            break;
        case 0b110:
            amount += 0.08f;
            break;
        case 0b111:
            amount += 0.10f;
            break;
        default:
            break;
    }
    *battery_voltage = battery_regulation_voltage * amount;
    return true;
}
