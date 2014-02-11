#ifndef FD_BLUETOOTH_H
#define FD_BLUETOOTH_H

#include "fd_binary.h"
#include "fd_time.h"

#include <stdbool.h>
#include <stdint.h>

void fd_bluetooth_initialize(void);

void fd_bluetooth_reset(void);

void fd_bluetooth_sleep(void);
void fd_bluetooth_wake(void);

bool fd_bluetooth_is_asleep(void);

uint8_t fd_bluetooth_get_tx_power(void);
void fd_bluetooth_set_tx_power(uint8_t level);

void fd_bluetooth_direct_test_mode_enter(uint16_t request, fd_time_t duration);
void fd_bluetooth_direct_test_mode_exit(void);
uint16_t fd_bluetooth_direct_test_mode_report(void);

void fd_bluetooth_step(void);

extern bool fd_nrf8001_did_setup;
extern bool fd_nrf8001_did_connect;
extern bool fd_nrf8001_did_receive_data;

void fd_bluetooth_diagnostics(fd_binary_t *binary);

void fd_bluetooth_set_name(uint8_t *name, uint32_t length);
uint32_t fd_bluetooth_get_name(uint8_t *name);

#endif