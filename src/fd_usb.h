#ifndef FD_USB_H
#define FD_USB_H

void fd_usb_initialize(void);

void fd_usb_wake(void);
void fd_usb_sleep(void);

void fd_usb_transfer(void);

#endif