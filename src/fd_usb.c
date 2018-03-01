#include "fd_control.h"
#include "fd_detour.h"
#include "fd_event.h"
#include "fd_lock.h"
#include "fd_log.h"
#include "fd_usb.h"

#include <em_system.h>
#include <em_usb.h>
#include <em_usbtypes.h>
#include <em_usbhal.h>

#define VENDOR 0x2333
#define PRODUCT 0x0002

#define DEFAULT_POLL_TIMEOUT 1
#define INTR_IN_EP_ADDR 0x81
#define INTR_OUT_EP_ADDR 0x01

EFM32_ALIGN(4)
static const USB_DeviceDescriptor_TypeDef deviceDesc __attribute__ ((aligned(4))) =
{
  .bLength            = USB_DEVICE_DESCSIZE,
  .bDescriptorType    = USB_DEVICE_DESCRIPTOR,
  .bcdUSB             = 0x0200,
  .bDeviceClass       = 0,
  .bDeviceSubClass    = 0,
  .bDeviceProtocol    = 0,
  .bMaxPacketSize0    = USB_EP0_SIZE,
  .idVendor           = VENDOR,
  .idProduct          = PRODUCT,
  .bcdDevice          = 0x0000,
  .iManufacturer      = 1,
  .iProduct           = 2,
  .iSerialNumber      = 3,
  .bNumConfigurations = 1
};

EFM32_ALIGN(4)
static const char ReportDescriptor[] __attribute__ ((aligned(4)))=
{
    0x06, 0x00, 0xff,              // USAGE_PAGE (Vendor Defined)
    0x09, 0x01,                    // USAGE (Vendor Defined)
    0xa1, 0x01,                    // COLLECTION (Application)

    0x15, 0x01,                    //   LOGICAL_MINIMUM (1)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, USB_MAX_EP_SIZE,         //   REPORT_COUNT (64)
    0x09, 0x01,                    //   VENDOR USAGE
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)

    0x15, 0x01,                    //   LOGICAL_MINIMUM (1)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, USB_MAX_EP_SIZE,         //   REPORT_COUNT (64)
    0x09, 0x01,                    //   VENDOR USAGE
    0x91, 0x02,                    //   Output (Data,Var,Abs)

    0xc0                           // END_COLLECTION
};

EFM32_ALIGN(4)
static const uint8_t configDesc[] __attribute__ ((aligned(4)))=
{
  /*** Configuration descriptor ***/
  USB_CONFIG_DESCSIZE,    /* bLength                                   */
  USB_CONFIG_DESCRIPTOR,  /* bDescriptorType                           */

  USB_CONFIG_DESCSIZE +   /* wTotalLength (LSB)                        */
  USB_INTERFACE_DESCSIZE +
  USB_HID_DESCSIZE +
  (USB_ENDPOINT_DESCSIZE * NUM_EP_USED),

  (USB_CONFIG_DESCSIZE +  /* wTotalLength (MSB)                        */
  USB_INTERFACE_DESCSIZE +
  USB_HID_DESCSIZE +
  (USB_ENDPOINT_DESCSIZE * NUM_EP_USED))>>8,

  1,                      /* bNumInterfaces                            */
  1,                      /* bConfigurationValue                       */
  0,                      /* iConfiguration                            */

  CONFIG_DESC_BM_RESERVED_D7 |    // bmAttrib: Self powered
  CONFIG_DESC_BM_SELFPOWERED,
  CONFIG_DESC_MAXPOWER_mA( 100 ), // bMaxPower: 100 mA

  // *** Interface descriptor ***
  USB_INTERFACE_DESCSIZE,         // bLength
  USB_INTERFACE_DESCRIPTOR,       // bDescriptorType
  0,                              // bInterfaceNumber
  0,                              // bAlternateSetting
  NUM_EP_USED,                    // bNumEndpoints
  0x03,                           // bInterfaceClass (HID)
  0,                              // bInterfaceSubClass
  0,                              // bInterfaceProtocol
  0,                              // iInterface

  /*** HID descriptor ***/
  USB_HID_DESCSIZE,       /* bLength               */
  USB_HID_DESCRIPTOR,     /* bDescriptorType       */
  0x01,                   /* bcdHID (LSB)          */
  0x01,                   /* bcdHID (MSB)          */
  0,                      /* bCountryCode          */
  1,                      /* bNumDescriptors       */
  USB_HID_REPORT_DESCRIPTOR,     /* bDecriptorType        */
  sizeof( ReportDescriptor ),    /* wDescriptorLength(LSB)*/
  sizeof( ReportDescriptor )>>8, /* wDescriptorLength(MSB)*/

  /*** Input Endpoint descriptor ***/
  USB_ENDPOINT_DESCSIZE,  /* bLength               */
  USB_ENDPOINT_DESCRIPTOR,/* bDescriptorType       */
  INTR_IN_EP_ADDR,        /* bEndpointAddress (IN) */
  USB_EPTYPE_INTR,        /* bmAttributes          */
  USB_MAX_EP_SIZE,        /* wMaxPacketSize (LSB)  */
  0,                      /* wMaxPacketSize (MSB)  */
  DEFAULT_POLL_TIMEOUT,   /* bInterval             */

  /*** Output Endpoint descriptor ***/
  USB_ENDPOINT_DESCSIZE,  /* bLength               */
  USB_ENDPOINT_DESCRIPTOR,/* bDescriptorType       */
  INTR_OUT_EP_ADDR,        /* bEndpointAddress (OUT) */
  USB_EPTYPE_INTR,        /* bmAttributes          */
  USB_MAX_EP_SIZE,        /* wMaxPacketSize (LSB)  */
  0,                      /* wMaxPacketSize (MSB)  */
  DEFAULT_POLL_TIMEOUT,   /* bInterval             */
};

STATIC_CONST_STRING_DESC_LANGID( langID, 0x04, 0x09 );
STATIC_CONST_STRING_DESC( iManufacturer, L"Firefly Design LLC" );
STATIC_CONST_STRING_DESC( iProduct, L"Firefly Ice" );

#define STATIC_STRING_DESC( name, value )  \
typedef struct                      \
{                                   \
  uint8_t len;                      \
  uint8_t type;                     \
  wchar_t name[sizeof( value )/2];  \
} __attribute__ ((packed)) _##name; \
EFM32_ALIGN( 4 )                    \
EFM32_PACK_START( 1 )               \
static _##name name __attribute__ ((aligned(4)))= \
{                                   \
  .len = sizeof(value),             \
  .type = USB_STRING_DESCRIPTOR,    \
  .name = value                     \
}                                   \
EFM32_PACK_END()

STATIC_STRING_DESC( iSerialNumber, L"0000000000000000" );

static const void * const strings[] =
{
  &langID,
  &iManufacturer,
  &iProduct,
  &iSerialNumber
};

// Endpoint buffer sizes
// 1 = single buffer, 2 = double buffering, 3 = tripple buffering ...
static const uint8_t bufferingMultiplier[ NUM_EP_USED + 1 ] = { 1, 1, 1 };

static void fd_usb_state_change(USBD_State_TypeDef oldState, USBD_State_TypeDef newState);
static int fd_usb_setup(const USB_Setup_TypeDef *setup);

static const USBD_Callbacks_TypeDef callbacks =
{
  .usbReset        = NULL,
  .usbStateChange  = fd_usb_state_change,
  .setupCmd        = fd_usb_setup,
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

// remove this once things are debugged...
static char fd_usb_log_buffer[256];
static uint8_t fd_usb_log_index;

static int fd_usb_errors = 0;

EFM32_ALIGN(4)
static uint8_t fd_usb_in_data[USB_MAX_EP_SIZE];

#define DETOUR_SIZE 400
static uint8_t fd_usb_detour_data[DETOUR_SIZE];
static fd_detour_t fd_usb_detour;

static uint8_t fd_usb_out_data[USB_MAX_EP_SIZE];

// needs room for detour packet overhead
#define DETOUR_SOURCE_COLLECTION_SIZE 400
static fd_detour_source_collection_t fd_usb_detour_source_collection;
static uint8_t fd_usb_detour_source_collection_data[DETOUR_SOURCE_COLLECTION_SIZE];

void fd_usb_initialize(void) {
    fd_usb_log_index = 0;

    fd_detour_initialize(&fd_usb_detour, fd_usb_detour_data, DETOUR_SIZE);
    fd_detour_source_collection_initialize(
        &fd_usb_detour_source_collection,
        fd_lock_owner_usb,
        USB_MAX_EP_SIZE,
        fd_usb_detour_source_collection_data,
        DETOUR_SOURCE_COLLECTION_SIZE
    );

    fd_event_add_em2_check(fd_usb_is_safe_to_enter_em2);
    fd_event_add_callback(FD_EVENT_USB_TRANSFER, fd_usb_transfer);

    uint64_t unique = SYSTEM_GetUnique();
    for (uint32_t i = 0; i < 16; ++i) {
        uint8_t c = unique & 0xf;
        if (c < 10) {
            c = '0' + c;
        } else {
            c = 'A' + c - 10;
        }
        iSerialNumber.iSerialNumber[i] = c;
        unique >>= 4;
    }
}

uint16_t fd_usb_get_vendor_id(void) {
    return VENDOR;
}

uint16_t fd_usb_get_product_id(void) {
    return PRODUCT;
}

bool fd_usb_is_powered(void) {
    return USBD_GetUsbState() != USBD_STATE_NONE;
}

bool fd_usb_is_connected(void) {
    return USBD_GetUsbState() == USBD_STATE_CONFIGURED;
}

extern volatile bool USBD_poweredDown;

void fd_usb_wake(void) {
    USBD_poweredDown = false;
    USBD_Init(&initstruct);
}

void fd_usb_sleep(void) {
    USBD_Stop();
    USBD_poweredDown = true;
}

int fd_usb_WriteChar(char c) {
    fd_usb_log_buffer[fd_usb_log_index++] = c;
    return 0;
}

static
int fd_usb_setup(const USB_Setup_TypeDef *setup) {
  STATIC_UBUF( hidDesc, USB_HID_DESCSIZE );

  int retVal = USB_STATUS_REQ_UNHANDLED;

  if ((setup->Type == USB_SETUP_TYPE_STANDARD) &&
      (setup->Direction == USB_SETUP_DIR_IN) &&
      (setup->Recipient == USB_SETUP_RECIPIENT_INTERFACE))
  {
    /* A HID device must extend the standard GET_DESCRIPTOR command   */
    /* with support for HID descriptors.                              */
    switch (setup->bRequest)
    {
    case GET_DESCRIPTOR:
      /********************/
      if ((setup->wValue >> 8) == USB_HID_REPORT_DESCRIPTOR)
      {
        USBD_Write(0, (void*) ReportDescriptor,
                   EFM32_MIN(sizeof(ReportDescriptor), setup->wLength),
                   NULL);
        retVal = USB_STATUS_OK;
      }
      else if ((setup->wValue >> 8) == USB_HID_DESCRIPTOR)
      {
        /* The HID descriptor might be misaligned ! */
        memcpy( hidDesc,
                &configDesc[ USB_CONFIG_DESCSIZE + USB_INTERFACE_DESCSIZE ],
                USB_HID_DESCSIZE );
        USBD_Write(0, hidDesc, EFM32_MIN(USB_HID_DESCSIZE, setup->wLength),
                   NULL);
        retVal = USB_STATUS_OK;
      }
      break;
    }
  }
#if 0
  else if ((setup->Type == USB_SETUP_TYPE_CLASS) &&
           (setup->Recipient == USB_SETUP_RECIPIENT_INTERFACE))
  {
    /* Implement the necessary HID class specific commands.           */
    switch (setup->bRequest)
    {
    case USB_HID_SET_REPORT:
      /********************/
      if (((setup->wValue >> 8) == 2) &&            /* Output report */
          ((setup->wValue & 0xFF) == 0) &&          /* Report ID     */
          (setup->wIndex == 0) &&                   /* Interface no. */
          (setup->wLength == 1) &&                  /* Report length */
          (setup->Direction != USB_SETUP_DIR_IN))
      {
        USBD_Read(0, (void*) &tmpBuffer, 1, OutputReportReceived);
        retVal = USB_STATUS_OK;
      }
      break;

    case USB_HID_GET_REPORT:
      /********************/
      if (((setup->wValue >> 8) == 1) &&            /* Input report  */
          ((setup->wValue & 0xFF) == 0) &&          /* Report ID     */
          (setup->wIndex == 0) &&                   /* Interface no. */
          (setup->wLength == 8) &&                  /* Report length */
          (setup->Direction == USB_SETUP_DIR_IN))
      {
        if (keyPushed)
        {
          /* Send a key pushed report */
          USBD_Write(0, (void*) &reportTable[ keySeqNo ],
                     sizeof(KeyReport_TypeDef), NULL);
        }
        else
        {
          /* Send an empty (key released) report */
          USBD_Write(0, (void*) &noKeyReport,
                     sizeof(KeyReport_TypeDef), NULL);
        }
        retVal = USB_STATUS_OK;
      }
      break;

    case USB_HID_SET_IDLE:
      /********************/
      if (((setup->wValue & 0xFF) == 0) &&          /* Report ID     */
          (setup->wIndex == 0) &&                   /* Interface no. */
          (setup->wLength == 0) &&
          (setup->Direction != USB_SETUP_DIR_IN))
      {
        idleRate    = setup->wValue >> 8;
        pollTimeout = 4 * idleRate;
        if (pollTimeout > DEFAULT_POLL_TIMEOUT)
        {
          pollTimeout = DEFAULT_POLL_TIMEOUT;
        }
        retVal = USB_STATUS_OK;
      }
      break;

    case USB_HID_GET_IDLE:
      /********************/
      if ((setup->wValue == 0) &&                   /* Report ID     */
          (setup->wIndex == 0) &&                   /* Interface no. */
          (setup->wLength == 1) &&
          (setup->Direction == USB_SETUP_DIR_IN))
      {
        *(uint8_t*)&tmpBuffer = idleRate;
        USBD_Write(0, (void*) &tmpBuffer, 1, NULL);
        retVal = USB_STATUS_OK;
      }
      break;
    }
  }
#endif

  return retVal;
}

bool fd_usb_is_safe_to_enter_em2(void) {
    return USBD_SafeToEnterEM2();
}

static
void fd_usb_state_change(USBD_State_TypeDef oldState __attribute__((unused)), USBD_State_TypeDef newState) {
    fd_lock_close(fd_lock_owner_usb);

    if (newState == USBD_STATE_CONFIGURED) {
        fd_detour_clear(&fd_usb_detour);
    }

    fd_event_set(FD_EVENT_USB_STATE | FD_EVENT_USB_TRANSFER);
}

static
int fd_usb_read_complete(USB_Status_TypeDef status, uint32_t xferred, uint32_t remaining __attribute__((unused))) {
    fd_event_set(FD_EVENT_USB_TRANSFER);

    if ((status == USB_STATUS_DEVICE_RESET) || (status == USB_STATUS_DEVICE_SUSPENDED)) {
        fd_detour_clear(&fd_usb_detour);
        return USB_STATUS_OK;
    }

    if (status != USB_STATUS_OK) {
        fd_log_assert_fail("");
        fd_detour_clear(&fd_usb_detour);
        return USB_STATUS_OK;
    }

    fd_detour_event(&fd_usb_detour, fd_usb_in_data, xferred);
    switch (fd_detour_state(&fd_usb_detour)) {
        case fd_detour_state_clear:
        case fd_detour_state_intermediate:
        break;
        case fd_detour_state_success:
            fd_control_process(&fd_usb_detour_source_collection, fd_usb_detour.data, fd_usb_detour.length);
            fd_detour_clear(&fd_usb_detour);
        break;
        case fd_detour_state_error:
            fd_log_assert_fail("");
            fd_detour_clear(&fd_usb_detour);
        break;
    }

    return USB_STATUS_OK;
}

static
int fd_usb_write_complete(USB_Status_TypeDef status __attribute__((unused)), uint32_t xferred __attribute__((unused)), uint32_t remaining __attribute__((unused))) {
    fd_event_set(FD_EVENT_USB_TRANSFER);

    return USB_STATUS_OK;
}

void fd_usb_transfer(void) {
    if (USBD_GetUsbState() != USBD_STATE_CONFIGURED) {
        return;
    }

    if (!USBD_EpIsBusy(INTR_OUT_EP_ADDR)) {
        int result = USBD_Read(INTR_OUT_EP_ADDR, fd_usb_in_data, USB_MAX_EP_SIZE, fd_usb_read_complete);
        if (result != USB_STATUS_OK) {
            ++fd_usb_errors;
        }
    } else {
        fd_event_set_exclusive(FD_EVENT_USB_TRANSFER);
    }

    if (!USBD_EpIsBusy(INTR_IN_EP_ADDR)) {
        if (fd_detour_source_collection_get(&fd_usb_detour_source_collection, fd_usb_out_data)) {
            USBD_Write(INTR_IN_EP_ADDR, fd_usb_out_data, USB_MAX_EP_SIZE, fd_usb_write_complete);
        }
    } else {
        fd_event_set_exclusive(FD_EVENT_USB_TRANSFER);
    }
}