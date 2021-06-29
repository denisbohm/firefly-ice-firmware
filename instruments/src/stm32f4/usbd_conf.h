#ifndef USBD_CONF_H
#define USBD_CONF_H

#define USBD_CFG_MAX_NUM           1

#define USBD_ITF_MAX_NUM           1

#define USB_MAX_STR_DESC_SIZ       64 

#define USBD_DYNAMIC_DESCRIPTOR_CHANGE_ENABLED 

#define HID_IN_EP                    0x81
#define HID_OUT_EP                   0x01
#define HID_IN_PACKET                64
#define HID_OUT_PACKET               64

#endif