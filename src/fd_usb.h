#ifndef FD_USB_H
#define FD_USB_H

#include <stdbool.h>
#include <stdint.h>

void fd_usb_initialize(void);

uint16_t fd_usb_get_vendor_id(void);
uint16_t fd_usb_get_product_id(void);

bool fd_usb_is_powered(void);
bool fd_usb_is_connected(void);
bool fd_usb_is_safe_to_enter_em2(void);

void fd_usb_wake(void);
void fd_usb_sleep(void);

void fd_usb_transfer(void);

#endif