#include "fd_bluetooth.h"
#include "fd_event.h"
#include "fd_indicator.h"
#include "fd_processor.h"
#include "fd_ui.h"
#include "fd_usb.h"

#include <em_gpio.h>

void fd_ui_charge_status_callback(void) {
    bool is_usb_powered = fd_usb_is_powered();
    bool is_charging = GPIO_PinInGet(CHG_STAT_PORT_PIN);
    if (is_usb_powered) {
        if (is_charging) {
            fd_indicator_set_usb(0xff, 0x00);
        } else {
            fd_indicator_set_usb(0x00, 0xff);
        }
    } else {
        fd_indicator_set_usb(0x00, 0x00);
    }
}

void fd_ui_usb_state_callback(void) {
    fd_ui_charge_status_callback();

    if (fd_usb_is_connected()) {
        fd_indicator_set_d4(0xff);
    } else {
        fd_indicator_set_d4(0x00);
    }
}

void fd_ui_ble_state_callback(void) {
    if (fd_nrf8001_did_connect) {
        fd_indicator_set_d0(0xff);
    } else {
        fd_indicator_set_d0(0x00);
    }
}

void fd_ui_initialize(void) {
    fd_event_add_callback(FD_EVENT_CHG_STAT, fd_ui_charge_status_callback);
    fd_event_add_callback(FD_EVENT_USB_STATE, fd_ui_usb_state_callback);
    fd_event_add_callback(FD_EVENT_BLE_STATE, fd_ui_ble_state_callback);
    fd_ui_usb_state_callback();
    fd_ui_ble_state_callback();
}
