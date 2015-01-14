#ifndef FD_HAL_BLE_H
#define FD_HAL_BLE_H

#include "fd_binary.h"

void fd_hal_ble_get_primary_service_uuid(fd_binary_t *binary);

bool fd_hal_ble_is_connected(void);

#endif