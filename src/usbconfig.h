#define USB_DEVICE
#define NUM_EP_USED 2

#define USB_USBC_32kHz_CLK USB_USBC_32kHz_CLK_LFRCO
//#define USB_USBC_32kHz_CLK USB_USBC_32kHz_CLK_LFXO

#define USB_TIMER USB_TIMER1

#define DEBUG_USB_API

extern int fd_usb_WriteChar(char c);
#define USER_PUTCHAR fd_usb_WriteChar