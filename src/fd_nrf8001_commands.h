#ifndef FD_NRF8001_COMMANDS_H
#define FD_NRF8001_COMMANDS_H

#include <stdint.h>

void fd_nrf8001_test(uint8_t test_feature);

void fd_nrf8001_echo(uint8_t *data, uint32_t length);

void fd_nrf8001_dtm_command(uint16_t dtm_command);

void fd_nrf8001_sleep(void);

void fd_nrf8001_wakeup(void);

void fd_nrf8001_setup(uint8_t *data, uint32_t length);

void fd_nrf8001_read_dynamic_data(void);

void fd_nrf8001_write_dynamic_data(uint8_t sequence_number, uint8_t *data, uint32_t length);

void fd_nrf8001_get_device_version(void);

void fd_nrf8001_get_device_address(void);

void fd_nrf8001_get_battery_level(void);

void fd_nrf8001_get_temperature(void);

void fd_nrf8001_radio_reset(void);

void fd_nrf8001_connect(uint16_t timeout, uint16_t interval);

void fd_nrf8001_bond(uint16_t timeout, uint16_t interval);

void fd_nrf8001_disconnect(uint8_t reason);

void fd_nrf8001_set_tx_power(uint8_t level);

void fd_nrf8001_change_timing_request_as_setup(void);

void fd_nrf8001_change_timing_request(
    uint16_t interval_min,
    uint16_t interval_max,
    uint16_t latency,
    uint16_t timeout
);

void fd_nrf8001_open_remote_pipe(uint8_t service_pipe_number);

void fd_nrf8001_set_application_latency(uint8_t mode, uint16_t latency);

void fd_nrf8001_set_key_rejected(void);

void fd_nrf8001_set_key(uint8_t *key);

void fd_nrf8001_open_advertising_pipe(uint64_t pipes);

void fd_nrf8001_broadcast(uint16_t timeout, uint16_t interval);

void fd_nrf8001_bond_security_request(void);

void fd_nrf8001_directed_connect(void);

void fd_nrf8001_close_remote_pipe(uint8_t service_pipe_number);

void fd_nrf8001_set_local_data(uint8_t service_pipe_number, uint8_t *data, uint32_t length);

void fd_nrf8001_send_data(uint8_t service_pipe_number, uint8_t *data, uint32_t length);

void fd_nrf8001_send_data_ack(uint8_t service_pipe_number);

void fd_nrf8001_request_data(uint8_t service_pipe_number);

void fd_nrf8001_send_data_nack(uint8_t service_pipe_number, uint8_t error_code);

#endif