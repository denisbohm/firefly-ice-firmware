#include "fd_event.h"
#include "fd_hal_processor.h"
#include "fd_hal_reset.h"
#include "fd_hal_system.h"
#include "fd_hal_usb.h"
#include "fd_power.h"
#include "fd_timer.h"

static fd_timer_t fd_power_update_timer;

#define UPDATE_INTERVAL 60

// fill up full charge over 2 hours
#define CHARGE_LEVEL_CHANGE_PER_INTERVAL 1.0 / (2.0 * 60.0);

// use up full charge over 2 weeks
#define DISCHARGE_LEVEL_CHANGE_PER_INTERVAL 1.0 / (14.0 * 24.0 * 60.0);

void fd_power_charge_status_callback(void) {
    float battery_voltage = fd_hal_system_get_battery_voltage();
    bool is_usb_powered = fd_hal_usb_is_powered();
    bool is_charging = fd_hal_system_is_charging();
    if (is_usb_powered && !is_charging && (battery_voltage > 4.0f)) {
        // The battery appears to be fully charged.
        if (RETAINED->power_battery_level < 1.0) {
            RETAINED->power_battery_level = 1.0;
        }
    }
}

void fd_power_update_callback(void) {
    fd_hal_system_start_conversions();
}

void fd_power_charge_current_callback(void) {
    fd_power_t power;
    fd_power_get(&power);
    bool is_usb_powered = fd_hal_usb_is_powered();
    bool is_charging = fd_hal_system_is_charging();
    if (is_usb_powered) {
        if (is_charging) {
            RETAINED->power_battery_level += CHARGE_LEVEL_CHANGE_PER_INTERVAL;
            if (RETAINED->power_battery_level > 1.0) {
                RETAINED->power_battery_level = 1.0;
            }
        } else {
            RETAINED->power_battery_level = 1.0;
        }
    } else {
        RETAINED->power_battery_level -= DISCHARGE_LEVEL_CHANGE_PER_INTERVAL;
        if (RETAINED->power_battery_level < 0.0) {
            RETAINED->power_battery_level = 0.0;
        }
    }

    float battery_voltage = fd_hal_system_get_battery_voltage();
    if (battery_voltage < 3.5f) {
        // The battery appears to be almost discharged.
        if (RETAINED->power_battery_level > 0.05) {
            RETAINED->power_battery_level = 0.05;
        }
    }

    fd_timer_start_next(&fd_power_update_timer, UPDATE_INTERVAL);
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

void fd_power_initialize(void) {
    fd_hal_reset_retained_t *retained = RETAINED;
    if (retained->power_battery_level == 0) {
        // battery level is uninitialized/unknown, so make a guess... -denis
        retained->power_battery_level = fd_power_estimate_battery_level();
    }

    fd_event_add_callback(FD_EVENT_CHG_STAT, fd_power_charge_status_callback);

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