/*
 fd_power: Estimate the battery charge level (0% to 100%).
 
 Note that this is tricky because lithium batteries don't have a nice linear change in battery voltage
 proportional to remaining charge.  There are two main ways to estimate the charge level: voltage based
 and/or based on average rate of change.  Average rate of change is nice when your system has predictable
 average current draw for significant periods of time.  When that isn't the case it is more difficult.
 If you have a very stable battery voltage under various loads, etc, then battery voltage may work.
 
 Estimation
 The estimation is based on battery voltage by default.  Once a minute an estimation is made.  When USB
 power is present the battery level is updated only if the estimate is higher.  When USB power is not
 present the battery level is updated only if the estimate is lower.
 
 The main assumptions are:
 1) The battery is charged to 4.2 V.
 2) During charging 85% of the charge is acquired during the constant current phase.
 3) A linear approximation based on voltage is reasonable during the constant current phase.
 4) A linear approximation based on charge current is reasonable during the constant voltage phase.
 5) When not charging a two piece linear approximation is used.
 6) The battery voltage threshold between the two linear approximations is 4.0 V.
 7) In the range 4.2 to 4.0 V the charge is between 100% and 90%.
 8) In the range 4.0 to 3.6 V the charge is between 90% and 0%.

 Notifications:
 1) When the battery voltage is less than 3.625 V and USB is not present consistently for 5 minutes
    then a low battery notification occurs.
 2) When a low battery condition ceases then a high battery notification occurs.
 
 Sanity Checks:
 1) If the battery voltage is below 3.625 V for 5 minutes and the battery level is more than 5% then
    the battery level is reduced to 5%.
 2) If USB power is present and the device is not being charged and the battery voltage is above 4.1 V
    for 2 minutes then the battery level is increased to 100%.
 */

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
static bool fd_power_low_battery;
static fd_power_callback_t fd_power_low_battery_level_callback;
static fd_power_callback_t fd_power_high_battery_level_callback;

#define UPDATE_INTERVAL 60

#define FD_POWER_HIGH_DURATION (2 * 60)
#define FD_POWER_LOW_DURATION (5 * 60)

static
void fd_power_sanity_check(void) {
    bool is_usb_powered = fd_hal_usb_is_powered();
    bool is_charging = fd_hal_system_is_charging();
    float battery_voltage = fd_hal_system_get_battery_voltage();
    
    // sanity check for fully charged battery
    if (is_usb_powered && !is_charging && (battery_voltage > 4.1f)) {
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
    bool low_power_condition = false;
    if (battery_voltage < fd_hal_system_get_almost_low_battery_voltage()) {
        if (fd_power_low_start == 0) {
            fd_power_low_start = fd_hal_rtc_get_seconds();
        }
        uint32_t duration = fd_hal_rtc_get_seconds() - fd_power_low_start;
        if (duration >= FD_POWER_LOW_DURATION) {
            // the battery appears to be almost discharged
            if (RETAINED->power_battery_level > 0.05) {
                RETAINED->power_battery_level = 0.05;
            }
        
            // low power condition if there isn't usb power present
            if (!is_usb_powered) {
                low_power_condition = true;
                if (!fd_power_low_battery) {
                    fd_power_low_battery = true;
                    if (fd_power_low_battery_level_callback) {
                        fd_power_low_battery_level_callback();
                    }
                }
            }
        }
    } else {
        fd_power_low_start = 0;
    }
    
    // !!! need to add some hysteresis -denis
    if (fd_power_low_battery && !low_power_condition) {
        fd_power_low_start = 0;
        fd_power_low_battery = false;
        if (fd_power_high_battery_level_callback) {
            fd_power_high_battery_level_callback();
        }
    }
}

static
void fd_power_usb_power_callback(void) {
    fd_power_high_start = 0;
}

static
double fd_power_estimate_battery_level(void) {
    double level;
    bool is_usb_powered = fd_hal_usb_is_powered();
    bool is_charging = fd_hal_system_is_charging();
    double battery_voltage = fd_hal_system_get_battery_voltage();
    if (is_usb_powered && !is_charging && (battery_voltage > 4.1f)) {
        // The battery appears to be very close to fully charged.
        level = 1.0;
    } else
        if (is_usb_powered && is_charging) {
            // The battery is charging.
            double charge_current = fd_hal_system_get_charge_current();
            // Assume +/- 10% tolerance on charge current limit
            double constant_current_threshold = fd_hal_system_get_charge_current_limit_threshold();
            if (charge_current < constant_current_threshold) {
                // The battery is charging and near or in the constant voltage phase.
                // 85% to 100%
                // *** simple linear approximation ***
                double min = fd_hal_system_get_battery_level_at_cccv_threshold();
                double max_level = 1.0 - min;
                double amount = (constant_current_threshold - charge_current) / constant_current_threshold;
                level = min + amount * max_level;
            } else {
                // The battery is charging and in the constant current phase.
                // 0% to 85%
                // *** simple linear approximation ***
                double min = fd_hal_system_get_low_battery_voltage();
                double max = 4.2;
                double voltage = battery_voltage;
                if (voltage > max) {
                    voltage = max;
                }
                if (voltage < min) {
                    voltage = min;
                }
                double max_level = fd_hal_system_get_battery_level_at_cccv_threshold();
                double amount = (voltage - min) / (max - min);
                level = amount * max_level;
            }
        } else {
            // The battery isn't charging.
            // *** simple 2-piece linear approximation (top of range is more non-linear than the rest) ***
            double threshold_voltage = fd_hal_system_get_discharge_threshold_voltage();
            double threshold_level = fd_hal_system_get_discharge_threshold_level();
            if (battery_voltage > threshold_voltage) {
                // 100% to 90% range when battery voltage is between 4.2 and 4.0 V
                double min = threshold_voltage;
                double max = 4.2;
                double voltage = battery_voltage;
                if (voltage > max) {
                    voltage = max;
                }
                double max_level = 1.0 - threshold_level;
                double amount = (voltage - min) / (max - min);
                level = threshold_level + amount * max_level;
            } else {
                // 90% to 0% range when battery voltage is between 4.0 V and 3.6 V
                double min = fd_hal_system_get_low_battery_voltage();
                double max = threshold_voltage;
                double voltage = battery_voltage;
                if (voltage < min) {
                    voltage = min;
                }
                double max_level = threshold_level;
                double amount = (voltage - min) / (max - min);
                level = amount * max_level;
            }
        }
    if (level > 1.0) {
        level = 1.0;
    }
    if (level < 0.0) {
        level = 0.0;
    }
    return level;
}

static
void fd_power_update_callback(void) {
    bool is_usb_powered = fd_hal_usb_is_powered();
#ifdef FD_POWER_USE_RATE
    bool is_charging = fd_hal_system_is_charging();
    if (is_usb_powered) {
        if (is_charging) {
            RETAINED->power_battery_level += fd_hal_system_get_charge_level_change_per_minute();
            if (RETAINED->power_battery_level > 1.0) {
                RETAINED->power_battery_level = 1.0;
            }
        }
    } else {
        RETAINED->power_battery_level -= fd_hal_system_get_discharge_level_change_per_minute();
        if (RETAINED->power_battery_level < 0.0) {
            RETAINED->power_battery_level = 0.0;
        }
    }
#else
    double level = fd_power_estimate_battery_level();
    if (is_usb_powered) {
        if (level > RETAINED->power_battery_level) {
            RETAINED->power_battery_level = level;
        }
    } else {
        if (level < RETAINED->power_battery_level) {
            RETAINED->power_battery_level = level;
        }
    }
#endif
    
    fd_power_sanity_check();

    fd_hal_system_start_conversions();
    fd_timer_start_next(&fd_power_update_timer, UPDATE_INTERVAL);
}

void fd_power_set_low_battery_level_callback(fd_power_callback_t callback) {
    fd_power_low_battery_level_callback = callback;
}

fd_power_callback_t fd_power_get_low_battery_level_callback(void) {
    return fd_power_low_battery_level_callback;
}

void fd_power_set_high_battery_level_callback(fd_power_callback_t callback) {
    fd_power_high_battery_level_callback = callback;
}

fd_power_callback_t fd_power_get_high_battery_level_callback(void) {
    return fd_power_high_battery_level_callback;
}

void fd_power_initialize(void) {
    fd_hal_reset_retained_t *retained = RETAINED;
    if ((retained->power_battery_level < 0.01) || (retained->power_battery_level > 1.0)) {
        // battery level is uninitialized/unknown/bogus, so make a guess... -denis
        retained->power_battery_level = fd_power_estimate_battery_level();
    }

    fd_power_high_start = 0;
    fd_power_low_start = 0;
    fd_power_low_battery = false;
    fd_power_low_battery_level_callback = 0;
    fd_power_high_battery_level_callback = 0;

    fd_event_add_callback(FD_EVENT_USB_POWER, fd_power_usb_power_callback);

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