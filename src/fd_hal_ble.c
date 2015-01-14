#include "fd_hal_ble.h"
#include "fd_bluetooth.h"

void fd_hal_ble_get_primary_service_uuid(fd_binary_t *binary) {
    // !!! little-endian to be consistent with fd_binary_t methods -denis
    static const uint8_t uuid[] = {0x99, 0x63, 0x84, 0x81, 0xa6, 0xb7, 0xbd, 0xb0, 0x91, 0x50, 0x95, 0x1b, 0x01, 0x00, 0x0a, 0x31};
    fd_binary_put_bytes(binary, (uint8_t *)uuid, sizeof(uuid));
}

bool fd_hal_ble_is_connected(void) {
    return fd_nrf8001_did_connect;
}