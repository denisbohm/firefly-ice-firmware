#include "fd_hal_ble.h"
#include "fd_bluetooth.h"

bool fd_hal_ble_is_connected(void) {
    return fd_nrf8001_did_connect;
}