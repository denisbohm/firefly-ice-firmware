#include "fd_bluetooth.h"
#include "fd_event.h"
#include "fd_hal_processor.h"
#include "fd_hal_rtc.h"
#include "fd_hal_ui.h"
#include "fd_indicator.h"
#include "fd_led.h"
#include "fd_lock.h"
#include "fd_log.h"
#include "fd_pins.h"
#include "fd_timer.h"
#include "fd_usb.h"

#include <em_gpio.h>

static
fd_timer_t error_check_timer;

typedef enum {
    fd_hal_ui_owner_indicate_state_unset,
    fd_hal_ui_owner_indicate_state_false,
    fd_hal_ui_owner_indicate_state_true,
} fd_hal_ui_owner_indicate_state_t;

typedef struct {
    fd_lock_owner_t owner;
    fd_hal_ui_owner_indicate_state_t state;
    uint32_t time;
} fd_hal_ui_owner_indicate_t;

#define OWNER_INDICATES_COUNT 2

void fd_hal_ui_update(void);

static
fd_hal_ui_owner_indicate_t owner_indicates[OWNER_INDICATES_COUNT];

void fd_hal_ui_set_indicate_unset(fd_lock_owner_t owner) {
    for (int i = 0; i < OWNER_INDICATES_COUNT; ++i) {
        fd_hal_ui_owner_indicate_t *owner_indicate = &owner_indicates[i];
        if (owner_indicate->owner == owner) {
            owner_indicate->state = fd_hal_ui_owner_indicate_state_unset;
            owner_indicate->time = fd_hal_rtc_get_seconds();
            return;
        }
    }
}

bool fd_hal_ui_get_indicate(fd_lock_owner_t owner) {
    for (int i = 0; i < OWNER_INDICATES_COUNT; ++i) {
        fd_hal_ui_owner_indicate_t *owner_indicate = &owner_indicates[i];
        if (owner_indicate->owner == owner) {
            return owner_indicate->state == fd_hal_ui_owner_indicate_state_true;
        }
    }
    return false;
}

void fd_hal_ui_set_indicate(fd_lock_owner_t owner, bool indicate) {
    for (int i = 0; i < OWNER_INDICATES_COUNT; ++i) {
        fd_hal_ui_owner_indicate_t *owner_indicate = &owner_indicates[i];
        if (owner_indicate->owner == owner) {
            owner_indicate->state = indicate ? fd_hal_ui_owner_indicate_state_true : fd_hal_ui_owner_indicate_state_false;
            owner_indicate->time = fd_hal_rtc_get_seconds();
            fd_hal_ui_update();
            return;
        }
    }
}

void fd_hal_ui_check_indicate(fd_lock_owner_t owner) {
    uint32_t now = fd_hal_rtc_get_seconds();
    for (int i = 0; i < OWNER_INDICATES_COUNT; ++i) {
        fd_hal_ui_owner_indicate_t *owner_indicate = &owner_indicates[i];
        if (owner_indicate->owner == owner) {
            if (owner_indicate->state == fd_hal_ui_owner_indicate_state_unset) {
                uint32_t duration = now - owner_indicate->time;
                if (duration >= 5) {
                    fd_hal_ui_set_indicate(owner, true);
                }
            }
            return;
        }
    }
}

void fd_hal_ui_set_identify(fd_time_t duration) {
    fd_indicator_set_identify_condition_active(duration);
}

void fd_hal_ui_clear_identify(void) {
    fd_indicator_set_identify_condition(fd_indicator_identify_condition_inactive);
}

void fd_hal_ui_set_led(fd_hal_ui_led_state_t *state, fd_time_t duration) {
    fd_led_override(state, duration);
}

void fd_hal_ui_charge_status_callback(void) {
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

void fd_hal_ui_usb_state_callback(void) {
    fd_hal_ui_charge_status_callback();

    if (fd_usb_is_connected()) {
        if (fd_hal_ui_get_indicate(fd_lock_owner_usb)) {
            if (fd_lock_owner(fd_lock_identifier_sync) == fd_lock_owner_usb) {
                fd_indicator_set_usb_connection_condition(fd_indicator_connection_condition_syncing);
            } else {
                fd_indicator_set_usb_connection_condition(fd_indicator_connection_condition_not_syncing);
            }
        } else {
            fd_indicator_set_usb_connection_condition(fd_indicator_connection_condition_unconnected);
        }
    } else {
        fd_hal_ui_set_indicate_unset(fd_lock_owner_usb);
        fd_indicator_set_usb_connection_condition(fd_indicator_connection_condition_unconnected);
    }
}

void fd_hal_ui_ble_state_callback(void) {
    if (fd_nrf8001_did_connect) {
        if (fd_hal_ui_get_indicate(fd_lock_owner_ble)) {
            if (fd_lock_owner(fd_lock_identifier_sync) == fd_lock_owner_ble) {
                fd_indicator_set_ble_connection_condition(fd_indicator_connection_condition_syncing);
            } else {
                fd_indicator_set_ble_connection_condition(fd_indicator_connection_condition_not_syncing);
            }
        } else {
            fd_indicator_set_ble_connection_condition(fd_indicator_connection_condition_unconnected);
        }
    } else {
        fd_hal_ui_set_indicate_unset(fd_lock_owner_ble);
        fd_indicator_set_ble_connection_condition(fd_indicator_connection_condition_unconnected);
    }
}

void fd_hal_ui_lock_state_callback(void) {
    fd_hal_ui_usb_state_callback();
    fd_hal_ui_ble_state_callback();
}

#define JAN_1_2014 1388534400

static
void error_check_timer_callback(void) {
    uint32_t seconds = fd_hal_rtc_get_seconds();
    if (seconds >= JAN_1_2014) {
        fd_indicator_set_error_condition(fd_indicator_error_condition_inactive);
    } else {
        fd_indicator_set_error_condition(fd_indicator_error_condition_active);
    }

    if (fd_usb_is_connected()) {
        fd_hal_ui_check_indicate(fd_lock_owner_ble);
    } else {
        fd_hal_ui_set_indicate_unset(fd_lock_owner_ble);
    }
    if (fd_nrf8001_did_connect) {
        fd_hal_ui_check_indicate(fd_lock_owner_usb);
    } else {
        fd_hal_ui_set_indicate_unset(fd_lock_owner_usb);
    }

    fd_time_t duration;
    duration.seconds = 2;
    duration.microseconds = 0;
    fd_timer_start(&error_check_timer, duration);
}

void fd_hal_ui_update(void) {
    fd_hal_ui_charge_status_callback();
    fd_hal_ui_lock_state_callback();

    error_check_timer_callback();
}

void fd_hal_ui_sleep(void) {
    fd_indicator_sleep();
    fd_led_sleep();
}

void fd_hal_ui_wake(void) {
    fd_hal_ui_update();
}

void fd_hal_ui_initialize(void) {
    fd_led_initialize();
    fd_indicator_initialize();

    owner_indicates[0].owner = fd_lock_owner_ble;
    owner_indicates[0].state = fd_hal_ui_owner_indicate_state_unset;
    owner_indicates[0].time = 0;

    owner_indicates[1].owner = fd_lock_owner_usb;
    owner_indicates[1].state = fd_hal_ui_owner_indicate_state_unset;
    owner_indicates[1].time = 0;

    fd_event_add_callback(FD_EVENT_CHG_STAT, fd_hal_ui_charge_status_callback);
    fd_event_add_callback(FD_EVENT_USB_STATE, fd_hal_ui_usb_state_callback);
    fd_event_add_callback(FD_EVENT_BLE_STATE, fd_hal_ui_ble_state_callback);
    fd_event_add_callback(FD_EVENT_LOCK_STATE, fd_hal_ui_lock_state_callback);

    fd_timer_add(&error_check_timer, error_check_timer_callback);

    fd_hal_ui_update();
}
