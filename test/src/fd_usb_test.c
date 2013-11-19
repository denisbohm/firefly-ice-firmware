#include "fd_log.h"
#include "fd_processor.h"

#include <em_gpio.h>
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
USB_DeviceDescriptor_TypeDef deviceDesc __attribute__ ((aligned(4))) =
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
STATIC_CONST_STRING_DESC( iManufacturer, 'F', 'i', 'r', 'e', 'f', 'l', 'y', ' ', 'D', 'e', 's', 'i', 'g', 'n', ' ', 'L', 'L', 'C');
STATIC_CONST_STRING_DESC( iProduct, 'F', 'i', 'r', 'e', 'f', 'l', 'y', ' ', 'I', 'c', 'e');

#define STATIC_STRING_DESC( _name, ... )                  \
EFM32_PACK_START( 1 )                                           \
typedef struct                                                  \
{                                                               \
  uint8_t  len;                                                 \
  uint8_t  type;                                                \
  char16_t name[ 1 + sizeof( (char16_t[]){__VA_ARGS__} ) / 2];  \
} __attribute__ ((packed)) _##_name;                            \
EFM32_PACK_END()                                                \
EFM32_ALIGN( 4 )                                                \
EFM32_PACK_START( 1 )                                           \
static _##_name _name __attribute__ ((aligned(4)))=       \
{                                                               \
  .len  = sizeof( _##_name ) - 2,                               \
  .type = USB_STRING_DESCRIPTOR,                                \
  .name = {__VA_ARGS__},                                        \
  .name[ ( ( sizeof( _##_name ) - 2 ) / 2 ) - 1 ] = '\0'        \
}                                                               \
EFM32_PACK_END()

STATIC_STRING_DESC( iSerialNumber, '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0');

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

static int fd_usb_errors = 0;

EFM32_ALIGN(4)
static uint8_t fd_usb_in_data[USB_MAX_EP_SIZE];

void fd_usb_initialize(void) {
    uint64_t unique = SYSTEM_GetUnique();
    for (uint32_t i = 0; i < 16; ++i) {
        uint8_t c = unique & 0xf;
        if (c < 10) {
            c = '0' + c;
        } else {
            c = 'A' + c - 10;
        }
        iSerialNumber.name[i] = c;
        unique >>= 4;
    }
}

bool fd_usb_is_powered(void) {
    return USBD_GetUsbState() != USBD_STATE_NONE;
}

bool fd_usb_is_connected(void) {
    return USBD_GetUsbState() == USBD_STATE_CONFIGURED;
}

void fd_usb_wake(void) {
    USBD_Init(&initstruct);
}

void fd_usb_sleep(void) {
    USBD_Stop();
}

int fd_usb_WriteChar(char c) {
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
void fd_usb_clear(void) {
}

static
void fd_usb_state_change(USBD_State_TypeDef oldState __attribute__((unused)), USBD_State_TypeDef newState) {
    if (newState == USBD_STATE_CONFIGURED) {
        fd_usb_clear();
    }
}

#define FD_USB_COMMAND_DATA 1
#define FD_USB_COMMAND_DONE 2

typedef struct {
    uint8_t bytes[USB_MAX_EP_SIZE];
} fd_usb_packet_t;

uint8_t *fd_usb_result;
uint32_t fd_usb_result_length;
uint32_t fd_usb_data_acks;
uint32_t fd_usb_send_index;
static fd_usb_packet_t fd_usb_send_packets[10];

bool fd_usb_done;

static
int fd_usb_read_complete(USB_Status_TypeDef status, uint32_t xferred, uint32_t remaining __attribute__((unused))) {
    if (status != USB_STATUS_OK) {
        fd_log_assert_fail("");
        return USB_STATUS_OK;
    }

    uint8_t *data = fd_usb_in_data;
    uint32_t command = data[0];
    switch (command) {
        case FD_USB_COMMAND_DATA: {
            uint32_t index = data[1];
            uint8_t byte = data[2];
            if (index < fd_usb_result_length) {
                fd_usb_result[index] = byte;
                ++fd_usb_data_acks;
            }
        } break;
        case FD_USB_COMMAND_DONE: {
        } break;
    }

    return USB_STATUS_OK;
}

static
int fd_usb_write_complete(USB_Status_TypeDef status __attribute__((unused)), uint32_t xferred __attribute__((unused)), uint32_t remaining __attribute__((unused))) {
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
    }

    if (!USBD_EpIsBusy(INTR_IN_EP_ADDR) && (fd_usb_send_index < 10)) {
        USBD_Write(INTR_IN_EP_ADDR, fd_usb_send_packets[fd_usb_send_index++].bytes, USB_MAX_EP_SIZE, fd_usb_write_complete);
        --fd_usb_data_acks;
    }
}

uint32_t fd_usb_test(uint16_t pid, uint8_t *result, uint32_t result_length) {
    deviceDesc.idProduct = pid;
    fd_usb_initialize();
    fd_usb_wake();

    fd_usb_result = result;
    fd_usb_result_length = result_length;
    fd_usb_data_acks = 0;
    fd_usb_send_index = 0;
    for (int i = 0; i < 10; ++i) {
        fd_usb_packet_t *packet = &fd_usb_send_packets[i];
        for (int j = 0; j < USB_MAX_EP_SIZE; ++j) {
            packet->bytes[j] = i + 1;
        }
    }
    fd_usb_done = false;
    GPIO_PinOutClear(LED5_PORT_PIN);
    while (!fd_usb_done) {
        fd_usb_transfer();
    }
    GPIO_PinOutSet(LED5_PORT_PIN);
    return fd_usb_errors;
}

void halt(void) {
    __asm("BKPT   #0");
}

void main(void) {
    fd_processor_initialize();
    fd_processor_wake();
    uint8_t bytes[10];
    fd_usb_test(0x0003, bytes, sizeof(bytes));
    halt();
}