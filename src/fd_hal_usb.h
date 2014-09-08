#ifndef FD_HAL_USB_H
#define FD_HAL_USB_H

#include <stdbool.h>
#include <stdint.h>

uint16_t fd_hal_usb_get_vendor_id(void);
uint16_t fd_hal_usb_get_product_id(void);

bool fd_hal_usb_is_connected(void);
bool fd_hal_usb_is_powered(void);

#endif