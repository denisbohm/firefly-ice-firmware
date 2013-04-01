#include "fd_usb.h"

#include <em_usb.h>

EFM32_ALIGN(4)
static const USB_DeviceDescriptor_TypeDef deviceDesc __attribute__ ((aligned(4))) =
{
  .bLength            = USB_DEVICE_DESCSIZE,
  .bDescriptorType    = USB_DEVICE_DESCRIPTOR,
  .bcdUSB             = 0x0200,
  .bDeviceClass       = 0xFF,
  .bDeviceSubClass    = 0,
  .bDeviceProtocol    = 0,
  .bMaxPacketSize0    = USB_EP0_SIZE,
  .idVendor           = 0x2544,
  .idProduct          = 0x0001,
  .bcdDevice          = 0x0000,
  .iManufacturer      = 1,
  .iProduct           = 2,
  .iSerialNumber      = 3,
  .bNumConfigurations = 1
};

EFM32_ALIGN(4)
static const uint8_t configDesc[] __attribute__ ((aligned(4)))=
{
  // *** Configuration descriptor ***
  USB_CONFIG_DESCSIZE,            // bLength
  USB_CONFIG_DESCRIPTOR,          // bDescriptorType
  USB_CONFIG_DESCSIZE +           // wTotalLength (LSB)
  USB_INTERFACE_DESCSIZE,
  (USB_CONFIG_DESCSIZE +          // wTotalLength (MSB)
  USB_INTERFACE_DESCSIZE)>>8,
  1,                              // bNumInterfaces
  1,                              // bConfigurationValue
  0,                              // iConfiguration
  CONFIG_DESC_BM_RESERVED_D7 |    // bmAttrib: Self powered
  CONFIG_DESC_BM_SELFPOWERED,
  CONFIG_DESC_MAXPOWER_mA( 100 ), // bMaxPower: 100 mA

  // *** Interface descriptor ***
  USB_INTERFACE_DESCSIZE,         // bLength
  USB_INTERFACE_DESCRIPTOR,       // bDescriptorType
  0,                              // bInterfaceNumber
  0,                              // bAlternateSetting
  NUM_EP_USED,                    // bNumEndpoints
  0xFF,                           // bInterfaceClass
  0,                              // bInterfaceSubClass
  0,                              // bInterfaceProtocol
  0,                              // iInterface
};

STATIC_CONST_STRING_DESC_LANGID( langID, 0x04, 0x09         );
//STATIC_CONST_STRING_DESC( iManufacturer, L"Energy Micro AS" );
//STATIC_CONST_STRING_DESC( iProduct     , L"Vendor Unique LED Device"   );
//STATIC_CONST_STRING_DESC( iSerialNumber, L"000000001234"    );
STATIC_CONST_STRING_DESC( iManufacturer, {'E', 'n', 'e', 'r', 'g', 'y', ' ', 'M', 'i', 'c', 'r', 'o', ' ', 'A', 'S'});
STATIC_CONST_STRING_DESC( iProduct , {'V', 'e', 'n', 'd', 'o', 'r', ' ', 'U', 'n', 'i', 'q', 'u', 'e', ' ', 'L', 'E', 'D', ' ', 'D', 'e', 'v', 'i', 'c', 'e'});
STATIC_CONST_STRING_DESC( iSerialNumber, {'0', '0', '0', '0', '0', '0', '0', '0', '1', '2', '3', '4'});


static const void * const strings[] =
{
  &langID,
  &iManufacturer,
  &iProduct,
  &iSerialNumber
};

// Endpoint buffer sizes
// 1 = single buffer, 2 = double buffering, 3 = tripple buffering ...
static const uint8_t bufferingMultiplier[ NUM_EP_USED + 1 ] = { 1 };

static int SetupCmd( const USB_Setup_TypeDef *setup );

static const USBD_Callbacks_TypeDef callbacks =
{
  .usbReset        = NULL,
  .usbStateChange  = NULL,
  .setupCmd        = SetupCmd,
  .isSelfPowered   = NULL,
  .sofInt          = NULL
};

static const USBD_Init_TypeDef initstruct =
{
  .deviceDescriptor    = &deviceDesc,
  .configDescriptor    = configDesc,
  .stringDescriptors   = strings,
  .numberOfStrings     = sizeof(strings)/sizeof(void*),
  .callbacks           = &callbacks,
  .bufferingMultiplier = bufferingMultiplier
};

static char fd_usb_log_buffer[256];
static uint8_t fd_usb_log_index;

void fd_usb_initialize(void) {
    fd_usb_log_index = 0;

    USBD_Init(&initstruct);
}

int fd_usb_WriteChar(char c) {
    fd_usb_log_buffer[fd_usb_log_index++] = c;
}

#define VND_GET_LEDS 0x10
#define VND_SET_LED  0x11

static uint16_t leds;

static int SetupCmd( const USB_Setup_TypeDef *setup )
{
  int retVal;
  static uint32_t buffer;
  uint8_t *pBuffer = (uint8_t*)&buffer;

  retVal = USB_STATUS_REQ_UNHANDLED;

  if ( setup->Type == USB_SETUP_TYPE_VENDOR )
  {
    switch ( setup->bRequest )
    {
      case VND_GET_LEDS:
      // ********************
        *pBuffer = leds;
        retVal = USBD_Write( 0, pBuffer, setup->wLength, NULL );
        break;

      case VND_SET_LED:
      // ********************
        if ( setup->wValue )
        {
          leds |= 1 << setup->wIndex;
        }
        else
        {
          leds &= ~( 1 << setup->wIndex );
        }
//        DVK_setLEDs( leds );
        retVal = USB_STATUS_OK;
        break;
    }
  }

  return retVal;
}