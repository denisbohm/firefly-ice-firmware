#include "fdi_usb.h"

#include "fdi_stm32.h"

#include "usbd_def.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_hid.h"

USBD_HandleTypeDef USBD_Device;

void fdi_usb_initialize(void) {
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWREx_EnableVddUSB();

  USBD_Init(&USBD_Device, &HID_Desc, 0);
  USBD_RegisterClass(&USBD_Device, &USBD_HID);
  USBD_Start(&USBD_Device);
}

void fdi_usb_set_tx_ready_callback(fdi_usb_tx_ready_callback_t callback) {
}

void fdi_usb_set_data_callback(fdi_usb_data_callback_t callback) {
}

void fdi_usb_power_up(void) {
}

bool fdi_usb_can_send(void) {
    return false;
}

void fdi_usb_send(uint8_t *buffer, size_t length) {
}
