/**
  * @file    fd_ble_beacon.h
  * @author  Denis Bohm
  * @version V1.0.0
  * @date    2019-06-17
  * @copyright Copyright 2019 Atlas Wearables, Inc.
  */

#ifndef FD_BLE_BEACON_H
#define FD_BLE_BEACON_H

#include "ble_gap.h"
#include "ble_srv_common.h"
#include "ble_types.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    ble_uuid128_t           uuid;
    uint32_t                adv_interval;
    uint16_t                major;
    uint16_t                minor;
    uint16_t                manuf_id;
    uint8_t                 rssi;                               /** measured RSSI at 1 meter distance in dBm*/
    ble_gap_addr_t          beacon_addr;                        /** ble address to be used by the beacon*/
    ble_srv_error_handler_t error_handler;                      /**< Function to be called in case of an error. */
} fd_ble_beacon_init_t;

void fd_ble_beacon_init(fd_ble_beacon_init_t *init);

void fd_ble_beacon_start(void);
void fd_ble_beacon_stop(void);

void fd_ble_beacon_on_sys_evt(uint32_t event);

#endif