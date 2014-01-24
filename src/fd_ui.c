#include "fd_bluetooth.h"
#include "fd_event.h"
#include "fd_indicator.h"
#include "fd_lock.h"
#include "fd_processor.h"
#include "fd_rtc.h"
#include "fd_timer.h"
#include "fd_ui.h"
#include "fd_usb.h"

#include <em_gpio.h>

static
fd_timer_t error_check_timer;

void fd_ui_charge_status_callback(void) {
    bool is_usb_powered = fd_usb_is_powered();
    bool is_charging = !GPIO_PinInGet(CHG_STAT_PORT_PIN);
    if (is_usb_powered) {
        if (is_charging) {
            fd_indicator_set_usb_condition(fd_indicator_usb_condition_powered_charging);
        } else {
            fd_indicator_set_usb_condition(fd_indicator_usb_condition_powered_not_charging);
        }
    } else {
        fd_indicator_set_usb_condition(fd_indicator_usb_condition_unpowered);
    }
}

void fd_ui_usb_state_callback(void) {
    fd_ui_charge_status_callback();

    if (fd_usb_is_connected()) {
        if (fd_lock_owner(fd_lock_identifier_sync) == fd_lock_owner_usb) {
            fd_indicator_set_usb_connection_condition(fd_indicator_connection_condition_syncing);
        } else {
            fd_indicator_set_usb_connection_condition(fd_indicator_connection_condition_not_syncing);
        }
    } else {
        fd_indicator_set_usb_connection_condition(fd_indicator_connection_condition_unconnected);
    }
}

void fd_ui_ble_state_callback(void) {
    if (fd_nrf8001_did_connect) {
        if (fd_lock_owner(fd_lock_identifier_sync) == fd_lock_owner_ble) {
            fd_indicator_set_ble_connection_condition(fd_indicator_connection_condition_syncing);
        } else {
            fd_indicator_set_ble_connection_condition(fd_indicator_connection_condition_not_syncing);
        }
    } else {
        fd_indicator_set_ble_connection_condition(fd_indicator_connection_condition_unconnected);
    }
}

void fd_ui_lock_state_callback(void) {
    fd_ui_usb_state_callback();
    fd_ui_ble_state_callback();
}

#define JAN_1_2014 1388534400

static
void error_check_timer_callback(void) {
    uint32_t seconds = fd_rtc_get_seconds();
    if (seconds >= JAN_1_2014) {
        fd_indicator_set_error_condition(fd_indicator_error_condition_inactive);
    } else {
        fd_indicator_set_error_condition(fd_indicator_error_condition_active);
    }

    fd_time_t duration;
    duration.seconds = 2;
    duration.microseconds = 0;
    fd_timer_start(&error_check_timer, duration);
}

void fd_ui_initialize(void) {
    fd_event_add_callback(FD_EVENT_CHG_STAT, fd_ui_charge_status_callback);
    fd_event_add_callback(FD_EVENT_USB_STATE, fd_ui_usb_state_callback);
    fd_event_add_callback(FD_EVENT_BLE_STATE, fd_ui_ble_state_callback);
    fd_event_add_callback(FD_EVENT_LOCK_STATE, fd_ui_lock_state_callback);

    fd_timer_add(&error_check_timer, error_check_timer_callback);
    error_check_timer_callback();

    fd_ui_charge_status_callback();
    fd_ui_lock_state_callback();
}
