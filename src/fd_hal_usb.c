#include "fd_hal_usb.h"
#include "fd_usb.h"

uint16_t fd_hal_usb_get_vendor_id(void) {
    return fd_usb_get_vendor_id();
}

uint16_t fd_hal_usb_get_product_id(void) {
    return fd_usb_get_product_id();
}

bool fd_hal_usb_is_connected(void) {
    return fd_usb_is_connected();
}

bool fd_hal_usb_is_powered(void) {
    return fd_usb_is_powered();
}
