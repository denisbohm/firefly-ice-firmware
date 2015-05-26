#include "fd_event.h"
#include "fd_hal_processor.h"
#include "fd_hal_reset.h"
#include "fd_hal_rtc.h"
#include "fd_hal_system.h"
#include "fd_hal_usb.h"
#include "fd_power.h"
#include "fd_timer.h"

static fd_timer_t fd_power_update_timer;
static uint32_t fd_power_high_start;
static uint32_t fd_power_low_start;
static fd_power_callback_t fd_power_low_battery_level_callback;

#define UPDATE_INTERVAL 60

#define FD_POWER_HIGH_DURATION (1 * 60)
#define FD_POWER_LOW_DURATION (5 * 60)

void fd_power_update_callback(void) {
    fd_hal_system_start_conversions();

    fd_timer_start_next(&fd_power_update_timer, UPDATE_INTERVAL);
}

static
void fd_power_sanity_check(void) {
    bool is_usb_powered = fd_hal_usb_is_powered();
    bool is_charging = fd_hal_system_is_charging();
    float battery_voltage = fd_hal_system_get_battery_voltage();

    // sanity check for fully charged battery
    if (is_usb_powered && !is_charging && (battery_voltage > 4.0f)) {
        if (fd_power_high_start == 0) {
            fd_power_high_start = fd_hal_rtc_get_seconds();
        }
        uint32_t duration = fd_hal_rtc_get_seconds() - fd_power_high_start;
        if (duration >= FD_POWER_HIGH_DURATION) {
            // The battery appears to be fully charged.
            if (RETAINED->power_battery_level < 1.0) {
                RETAINED->power_battery_level = 1.0;
            }
        }
    } else {
        fd_power_high_start = 0;
    }

    // sanity check for almost discharged battery
    if (battery_voltage < 3.5f) {
        if (fd_power_low_start == 0) {
            fd_power_low_start = fd_hal_rtc_get_seconds();
        }
        uint32_t duration = fd_hal_rtc_get_seconds() - fd_power_low_start;
        if (duration >= FD_POWER_LOW_DURATION) {
            // The battery appears to be almost discharged.
            if (RETAINED->power_battery_level > 0.05) {
                RETAINED->power_battery_level = 0.05;
            }
        }
    } else {
        fd_power_low_start = 0;
    }
}

void fd_power_charge_current_callback(void) {
    bool is_usb_powered = fd_hal_usb_is_powered();
    bool is_charging = fd_hal_system_is_charging();
    if (is_usb_powered) {
        if (is_charging) {
            RETAINED->power_battery_level += fd_hal_system_get_charge_level_change_per_minute();
            if (RETAINED->power_battery_level > 1.0) {
                RETAINED->power_battery_level = 1.0;
            }
        } else {
            RETAINED->power_battery_level = 1.0;
        }
    } else {
        RETAINED->power_battery_level -= fd_hal_system_get_discharge_level_change_per_minute();
        if (RETAINED->power_battery_level < 0.0) {
            RETAINED->power_battery_level = 0.0;
        }
    }

    fd_power_sanity_check();

    if (RETAINED->power_battery_level < 0.05) {
        if (fd_power_low_battery_level_callback) {
            fd_power_low_battery_level_callback();
        }
    }
}

double fd_power_estimate_battery_level(void) {
    float battery_voltage = fd_hal_system_get_battery_voltage();
    bool is_usb_powered = fd_hal_usb_is_powered();
    bool is_charging = fd_hal_system_is_charging();
    if (is_usb_powered && !is_charging && (battery_voltage > 4.0f)) {
        // The battery appears to be fully charged.
        return 1.0;
    }
    if (battery_voltage < 3.5f) {
        // The battery appears to be almost discharged.
        return 0.05;
    }
    if (is_usb_powered && is_charging) {
        if (battery_voltage > 4.0f) {
            // The battery is charging and near or in the constant voltage phase.
            return 0.5;
        } else {
            // The battery is charging and in the constant current phase.
            return 0.15;
        }
    }
    // The battery isn't low or high and isn't charging.
    return 0.15;
}

void fd_power_set_low_battery_level_callback(fd_power_callback_t callback) {
    fd_power_low_battery_level_callback = callback;
}

fd_power_callback_t fd_power_get_low_battery_level_callback(void) {
    return fd_power_low_battery_level_callback;
}

void fd_power_initialize(void) {
    fd_hal_reset_retained_t *retained = RETAINED;
    if ((retained->power_battery_level <= 0) || (retained->power_battery_level > 1.0)) {
        // battery level is uninitialized/unknown/bogus, so make a guess... -denis
        retained->power_battery_level = fd_power_estimate_battery_level();
    }

    fd_power_high_start = 0;
    fd_power_low_start = 0;
    fd_power_low_battery_level_callback = 0;

    fd_event_add_callback(FD_EVENT_ADC_CHARGE_CURRENT, fd_power_charge_current_callback);

    fd_timer_add(&fd_power_update_timer, fd_power_update_callback);
    fd_timer_start_next(&fd_power_update_timer, UPDATE_INTERVAL);
}

void fd_power_get(fd_power_t *power) {
    power->battery_level = RETAINED->power_battery_level;
    power->battery_voltage = fd_hal_system_get_battery_voltage();
    power->is_usb_powered = fd_hal_usb_is_powered();
    power->is_charging = power->is_usb_powered && fd_hal_system_is_charging();
    power->charge_current = fd_hal_system_get_charge_current();
    power->temperature = fd_hal_system_get_temperature();
}

void fd_power_set(float battery_level) {
    RETAINED->power_battery_level = battery_level;
}