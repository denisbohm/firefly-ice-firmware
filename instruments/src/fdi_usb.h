#ifndef FDI_USB_H
#define FDI_USB_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef void (*fdi_usb_data_callback_t)(uint8_t *buffer, uint32_t length);
typedef void (*fdi_usb_tx_ready_callback_t)(void);

void fdi_usb_initialize(void);
void fdi_usb_set_tx_ready_callback(fdi_usb_tx_ready_callback_t callback);
void fdi_usb_set_data_callback(fdi_usb_data_callback_t callback);
void fdi_usb_power_up(void);
bool fdi_usb_can_send(void);
void fdi_usb_send(uint8_t *buffer, size_t length);

#endif