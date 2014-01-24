#ifndef FD_INDICATOR_H
#define FD_INDICATOR_H

#include "fd_timer.h"

void fd_indicator_initialize(void);

typedef enum {
    fd_indicator_usb_condition_unpowered,            // USB LEDs are off
    fd_indicator_usb_condition_powered_not_charging, // USB LEDs are steady green (eased in & out)
    fd_indicator_usb_condition_powered_charging,     // USB LEDs are breathing orange (eased in & out)
} fd_indicator_usb_condition_t;

void fd_indicator_set_usb_condition(fd_indicator_usb_condition_t condition);

typedef enum {
    fd_indicator_connection_condition_unconnected, // RGB is off
    fd_indicator_connection_condition_not_syncing, // RGB intermittent breathing low (eased in & out)
    fd_indicator_connection_condition_syncing,     // RGB breathing (eased in & out)
} fd_indicator_connection_condition_t;

void fd_indicator_set_usb_connection_condition(fd_indicator_connection_condition_t condition);
void fd_indicator_set_ble_connection_condition(fd_indicator_connection_condition_t condition);

typedef enum {
    fd_indicator_identify_condition_inactive, // RGB 2 is off
    fd_indicator_identify_condition_active,   // RGB 2 quick breathing white (eased in & out)
} fd_indicator_identify_condition_t;

void fd_indicator_set_identify_condition(fd_indicator_identify_condition_t condition);
void fd_indicator_set_identify_condition_active(fd_time_t duration);

typedef enum {
    fd_indicator_error_condition_inactive, // R LEDs are off
    fd_indicator_error_condition_active,   // R LED 0 and 4 alternately breathing (eased in & out)
} fd_indicator_error_condition_t;

void fd_indicator_set_error_condition(fd_indicator_error_condition_t condition);

// fuel gauge: light up all 3 RGB LEDs in sequence green, then drop down to charge level and hold for a few seconds.
// When dropping down change color to orange (1 to 2 days of charge left) or red (less than 1 day of charge left).

#endif