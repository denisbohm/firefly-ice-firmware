#include "fd_control.h"
#include "fd_detour.h"
#include "fd_log.h"
#include "fd_usb.h"

#include <em_usb.h>

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
  .idVendor           = 0x2544,
  .idProduct          = 0x0001,
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
//STATIC_CONST_STRING_DESC( iManufacturer, L"Energy Micro AS" );
//STATIC_CONST_STRING_DESC( iProduct     , L"Vendor Unique LED Device"   );
//STATIC_CONST_STRING_DESC( iSerialNumber, L"000000001234"    );
STATIC_CONST_STRING_DESC( iManufacturer, {'E', 'n', 'e', 'r', 'g', 'y', ' ', 'M', 'i', 'c', 'r', 'o', ' ', 'A', 'S', '\0'});
STATIC_CONST_STRING_DESC( iProduct , {'V', 'e', 'n', 'd', 'o', 'r', ' ', 'U', 'n', 'i', 'q', 'u', 'e', ' ', 'L', 'E', 'D', ' ', 'D', 'e', 'v', 'i', 'c', 'e', '\0'});
STATIC_CONST_STRING_DESC( iSerialNumber, {'0', '0', '0', '0', '0', '0', '0', '0', '1', '2', '3', '4', '\0'});


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

#define DETOUR_SIZE 256

static uint8_t fd_usb_detour_data[DETOUR_SIZE];
static fd_detour_t fd_usb_detour;

static uint8_t fd_usb_out_data[USB_MAX_EP_SIZE];

static fd_detour_source_collection_t fd_usb_detour_source_collection;
static fd_detour_source_t fd_usb_detour_source;

void fd_usb_initialize(void) {
    fd_usb_log_index = 0;

    USBD_Init(&initstruct);

    fd_detour_initialize(&fd_usb_detour, fd_usb_detour_data, DETOUR_SIZE);

    fd_detour_source_collection_initialize(&fd_usb_detour_source_collection);
}

int fd_usb_WriteChar(char c) {
    fd_usb_log_buffer[fd_usb_log_index++] = c;
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

static
void fd_usb_state_change(USBD_State_TypeDef oldState, USBD_State_TypeDef newState) {
    if (newState == USBD_STATE_CONFIGURED) {
        fd_detour_clear(&fd_usb_detour);
    }
}

static
void detour_supplier(uint32_t offset, uint8_t *data, uint32_t length) {
    data[0] = 0x5a;
}

static
int fd_usb_read_complete(USB_Status_TypeDef status, uint32_t xferred, uint32_t remaining) {
    if (status != USB_STATUS_OK) {
        fd_log("");
        fd_detour_clear(&fd_usb_detour);
        return USB_STATUS_OK;
    }

    fd_detour_event(&fd_usb_detour, fd_usb_in_data, xferred);
    switch (fd_detour_state(&fd_usb_detour)) {
        case fd_detour_state_success:
            fd_control_process(fd_usb_detour.data, fd_usb_detour.length);

            // !!! just for testing
            fd_detour_source_initialize(&fd_usb_detour_source, detour_supplier, 1);
            fd_detour_source_collection_push(&fd_usb_detour_source_collection, &fd_usb_detour_source);

            fd_detour_clear(&fd_usb_detour);
        break;
        case fd_detour_state_error:
            fd_log("");
            fd_detour_clear(&fd_usb_detour);
        break;
    }
    return USB_STATUS_OK;
}

static
int fd_usb_write_complete(USB_Status_TypeDef status, uint32_t xferred, uint32_t remaining) {
    fd_detour_source_collection_pop(&fd_usb_detour_source_collection);
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
    }

    if (!USBD_EpIsBusy(INTR_IN_EP_ADDR)) {
        fd_detour_source_t *source = fd_usb_detour_source_collection.first;
        if (source) {
            if (fd_detour_source_get(source, fd_usb_out_data, USB_MAX_EP_SIZE)) {
                USBD_Write(INTR_IN_EP_ADDR, fd_usb_out_data, USB_MAX_EP_SIZE, fd_usb_write_complete);
            }
        }
    }
}