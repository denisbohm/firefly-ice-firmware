#ifndef FD_INDICATOR_H
#define FD_INDICATOR_H

void fd_indicator_initialize(void);

typedef enum {
    fd_indicator_usb_condition_unpowered,            // USB LEDs are off
    fd_indicator_usb_condition_powered_not_charging, // USB LEDs are steady green (eased in & out)
    fd_indicator_usb_condition_powered_charging,     // USB LEDs are breathing orange (eased in & out)
} fd_indicator_usb_condition_t;

void fd_indicator_set_usb_condition(fd_indicator_usb_condition_t condition);

#endif