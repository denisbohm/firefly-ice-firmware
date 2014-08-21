#include "fd_hal_usb.h"
#include "fd_usb.h"

bool fd_hal_usb_is_connected(void) {
    return fd_usb_is_connected();
}

bool fd_hal_usb_is_powered(void) {
    return fd_usb_is_powered();
}
