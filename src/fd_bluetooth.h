#ifndef FD_BLUETOOTH_H
#define FD_BLUETOOTH_H

#include <stdbool.h>

void fd_bluetooth_initialize(void);

void fd_bluetooth_sleep(void);
void fd_bluetooth_wake(void);

bool fd_bluetooth_is_asleep(void);

void fd_bluetooth_step(void);

extern bool fd_nrf8001_did_setup;
extern bool fd_nrf8001_did_connect;
extern bool fd_nrf8001_did_receive_data;

#endif