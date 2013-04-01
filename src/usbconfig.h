#define USB_DEVICE
#define NUM_EP_USED 0

#define USB_USBC_32kHz_CLK USB_USBC_32kHz_CLK_LFRCO

#define DEBUG_USB_API

extern int fd_usb_WriteChar(char c);
#define USER_PUTCHAR fd_usb_WriteChar