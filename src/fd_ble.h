#ifndef FD_BLE_H
#define FD_BLE_H

#include <stdbool.h>
#include <stdint.h>

#define FD_UNUSED __attribute__((unused))

typedef void (*fd_ble_service_characteristic_value_handler_t)(uint16_t uuid, const uint8_t* data, uint16_t length);

#define FD_BLE_CHARACTERISTIC_FLAG_WRITE                  0x01
#define FD_BLE_CHARACTERISTIC_FLAG_WRITE_WITHOUT_RESPONSE 0x02
#define FD_BLE_CHARACTERISTIC_FLAG_NOTIFY                 0x04

typedef struct {
    uint16_t uuid;
    uint32_t flags;
    fd_ble_service_characteristic_value_handler_t value_handler;
} fd_ble_characteristic_t;

typedef struct {
    uint8_t base_uuid[16];
    uint16_t uuid;
    fd_ble_characteristic_t *characteristics;
    uint32_t characteristics_count;
} fd_ble_service_t;

struct fd_ble_l2cap_channel_s;

typedef void (*fd_ble_channel_on_open_t)(struct fd_ble_l2cap_channel_s *channel);
typedef void (*fd_ble_channel_on_close_t)(struct fd_ble_l2cap_channel_s *channel);
typedef void (*fd_ble_stream_on_read_available_t)(struct fd_ble_l2cap_channel_s *channel);
typedef void (*fd_ble_stream_on_write_available_t)(struct fd_ble_l2cap_channel_s *channel);

typedef bool (*fd_ble_stream_is_read_available_t)(struct fd_ble_l2cap_channel_s *channel);
typedef uint32_t (*fd_ble_stream_read_t)(struct fd_ble_l2cap_channel_s *channel, uint8_t *data, uint32_t length);

typedef bool (*fd_ble_stream_is_write_available_t)(struct fd_ble_l2cap_channel_s *channel);
typedef uint32_t (*fd_ble_stream_write_t)(struct fd_ble_l2cap_channel_s *channel, uint8_t *data, uint32_t length);

typedef struct fd_ble_l2cap_channel_s {
    uint16_t protocol_service_multiplexer;
    fd_ble_channel_on_open_t on_open;
    fd_ble_channel_on_close_t on_close;
    fd_ble_stream_on_read_available_t on_read_available;
    fd_ble_stream_on_write_available_t on_write_available;

    bool read_available;
    fd_ble_stream_is_read_available_t is_read_available;
    fd_ble_stream_read_t read;
    bool write_available;
    fd_ble_stream_is_write_available_t is_write_available;
    fd_ble_stream_write_t write;
} fd_ble_l2cap_channel_t;

typedef void (*fd_ble_on_tick_t)(void);

typedef struct {
    uint8_t *name;
    fd_ble_service_t *services;
    uint32_t service_count;
    fd_ble_l2cap_channel_t *channels;
    uint32_t channel_count;
    fd_ble_on_tick_t on_tick;
} fd_ble_configuration_t;

void fd_ble_initialize(fd_ble_configuration_t *configuration);
void fd_ble_main_loop(void);
bool fd_ble_set_characteristic_value(uint16_t uuid, uint8_t *value, uint32_t length);

#endif
