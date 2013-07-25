#include "fd_adc.h"
#include "fd_event.h"
#include "fd_power.h"
#include "fd_processor.h"
#include "fd_timer.h"
#include "fd_usb.h"

#include <em_gpio.h>

#define MAGIC 0xa610efcc

static uint32_t fd_power_magic __attribute__ ((section(".non_init")));
static double fd_power_battery_level __attribute__ ((section(".non_init")));
static fd_timer_t fd_power_timer;

// fill up full charge over 2 hours
#define CHARGE_LEVEL_CHANGE_PER_INTERVAL 1.0 / (2.0 * 60.0);

// use up full charge over 2 weeks
#define DISCHARGE_LEVEL_CHANGE_PER_INTERVAL 1.0 / (14.0 * 24.0 * 60.0);

void fd_power_charge_status_callback(void) {
    float battery_voltage = fd_adc_get_battery_voltage();
    bool is_usb_powered = fd_usb_is_powered();
    bool is_charging = GPIO_PinInGet(CHG_STAT_PORT_PIN);
    if (is_usb_powered && !is_charging && (battery_voltage > 4.0f)) {
        // The battery appears to be fully charged.
        if (fd_power_battery_level < 1.0) {
            fd_power_battery_level = 1.0;
        }
    }
}

void fd_power_timer_callback(void) {
    if (GPIO_PinInGet(CHG_STAT_PORT_PIN)) {
        fd_power_battery_level -= CHARGE_LEVEL_CHANGE_PER_INTERVAL;
        if (fd_power_battery_level > 1.0) {
            fd_power_battery_level = 1.0;
        }
    } else {
        fd_power_battery_level -= DISCHARGE_LEVEL_CHANGE_PER_INTERVAL;
        if (fd_power_battery_level < 0.0) {
            fd_power_battery_level = 0.0;
        }
    }

    float battery_voltage = fd_adc_get_battery_voltage();
    if (battery_voltage < 3.5f) {
        // The battery appears to be almost discharged.
        if (fd_power_battery_level > 0.05) {
            fd_power_battery_level = 0.05;
        }
    }

    fd_time_t interval = {.seconds = 60, .microseconds = 0};
    fd_timer_start(&fd_power_timer, interval);
}

double fd_power_estimate_battery_level(void) {
    float battery_voltage = fd_adc_get_battery_voltage();
    bool is_usb_powered = fd_usb_is_powered();
    bool is_charging = GPIO_PinInGet(CHG_STAT_PORT_PIN);
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
    if ((fd_power_magic != MAGIC) || (fd_power_battery_level < 0.0) || (fd_power_battery_level > 1.0)) {
        // battery level is uninitialized/unknown, so make a guess... -denis
        fd_power_battery_level = fd_power_estimate_battery_level();
        fd_power_magic = MAGIC;
    }

    fd_event_add_callback(FD_EVENT_CHG_STAT, fd_power_charge_status_callback);

    fd_timer_add(&fd_power_timer, fd_power_timer_callback);
    fd_time_t interval = {.seconds = 5 * 60, .microseconds = 0};
    fd_timer_start(&fd_power_timer, interval);
}

void fd_power_get(fd_power_t *power) {
    power->battery_level = fd_power_battery_level;
    power->battery_voltage = fd_adc_get_battery_voltage();
    power->is_usb_powered = fd_usb_is_powered();
    power->is_charging = GPIO_PinInGet(CHG_STAT_PORT_PIN);
    power->charge_current = fd_adc_get_charge_current();
    power->temperature = fd_adc_get_temperature();
}

void fd_power_set(float battery_level) {
    fd_power_battery_level = battery_level;
}