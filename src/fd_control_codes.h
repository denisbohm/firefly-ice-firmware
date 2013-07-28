#ifndef FD_CONTROL_CODES_H
#define FD_CONTROL_CODES_H

#define FD_CONTROL_PING 1

#define FD_CONTROL_GET_PROPERTIES 2
#define FD_CONTROL_SET_PROPERTIES 3

#define FD_CONTROL_PROVISION 4
#define FD_CONTROL_RESET 5

#define FD_CONTROL_UPDATE_GET_SECTOR_HASHES 6
#define FD_CONTROL_UPDATE_ERASE_SECTORS 7
#define FD_CONTROL_UPDATE_WRITE_PAGE 8
#define FD_CONTROL_UPDATE_COMMIT 9

#define FD_CONTROL_RADIO_DIRECT_TEST_MODE_ENTER 10
#define FD_CONTROL_RADIO_DIRECT_TEST_MODE_EXIT 11
#define FD_CONTROL_RADIO_DIRECT_TEST_MODE_REPORT 12

#define FD_CONTROL_DISCONNECT 13

#define FD_CONTROL_INDICATOR_OVERRIDE 14

#define FD_CONTROL_SYNC_START 15
#define FD_CONTROL_SYNC_DATA 16
#define FD_CONTROL_SYNC_ACK 17

// property bits for get/set propery commands
#define FD_CONTROL_PROPERTY_VERSION     0x00000001
#define FD_CONTROL_PROPERTY_HARDWARE_ID 0x00000002
#define FD_CONTROL_PROPERTY_DEBUG_LOCK  0x00000004
#define FD_CONTROL_PROPERTY_RTC         0x00000008
#define FD_CONTROL_PROPERTY_POWER       0x00000010
#define FD_CONTROL_PROPERTY_SITE        0x00000020

#define FD_CONTROL_RESET_SYSTEM_REQUEST 1
#define FD_CONTROL_RESET_WATCHDOG 2
#define FD_CONTROL_RESET_HARD_FAULT 3

#endif