#ifndef FD_NRF8001_CALLBACKS_H
#define FD_NRF8001_CALLBACKS_H

#include <stdint.h>

// command responses

void fd_nrf8001_test_error(uint8_t status);

void fd_nrf8001_dtm_command_error(uint8_t status);

void fd_nrf8001_dtm_command_event_error(void);

void fd_nrf8001_dtm_command_success(uint16_t data);

void fd_nrf8001_wakeup_error(uint8_t status);

void fd_nrf8001_wakeup_success(void);

void fd_nrf8001_setup_error(uint8_t status);

void fd_nrf8001_setup_continue(void);

void fd_nrf8001_setup_complete(void);

void fd_nrf8001_read_dynamic_data_error(uint8_t status);

void fd_nrf8001_read_dynamic_data_continue(uint32_t sequence_number, uint8_t *data, uint32_t length);

void fd_nrf8001_read_dynamic_data_complete(void);

void fd_nrf8001_write_dynamic_data_error(uint8_t status);

void fd_nrf8001_write_dynamic_data_continue(void);

void fd_nrf8001_write_dynamic_data_complete(void);

void fd_nrf8001_get_device_version_error(uint8_t status);

void fd_nrf8001_get_device_version_success(
    uint16_t configuration_id,
    uint8_t aci_protocol_version,
    uint8_t current_setup_format,
    uint32_t setup_id,
    uint8_t configuration_status
);

void fd_nrf8001_get_device_address_error(uint8_t status);

void fd_nrf8001_get_device_address_success(
    uint8_t *address,
    uint8_t configuration_id
);

void fd_nrf8001_get_battery_level_error(uint8_t status);

void fd_nrf8001_get_battery_level_success(float voltage);

void fd_nrf8001_get_temperature_error(uint8_t status);

void fd_nrf8001_get_temperature_success(float temperature);

void fd_nrf8001_radio_reset_error(uint8_t status);

void fd_nrf8001_radio_reset_success(void);

void fd_nrf8001_connect_error(uint8_t status);

void fd_nrf8001_connect_success(void);

void fd_nrf8001_bond_error(uint8_t status);

void fd_nrf8001_bond_success(void);

void fd_nrf8001_disconnect_error(uint8_t status);

void fd_nrf8001_disconnect_success(void);

void fd_nrf8001_set_tx_power_error(uint8_t status);

void fd_nrf8001_set_tx_power_success(void);

void fd_nrf8001_change_timing_request_error(uint8_t status);

void fd_nrf8001_change_timing_request_success(void);

void fd_nrf8001_open_remote_pipe_error(uint8_t status);

void fd_nrf8001_open_remote_pipe_success(void);

void fd_nrf8001_set_application_latency_error(uint8_t status);

void fd_nrf8001_set_application_latency_success(void);

void fd_nrf8001_set_key_error(uint8_t status);

void fd_nrf8001_set_key_success(void);

void fd_nrf8001_open_advertising_pipe_error(uint8_t status);

void fd_nrf8001_open_advertising_pipe_success(void);

void fd_nrf8001_broadcast_error(uint8_t status);

void fd_nrf8001_broadcast_success(void);

void fd_nrf8001_bond_security_request_error(uint8_t status);

void fd_nrf8001_bond_security_request_success(void);

void fd_nrf8001_directed_connect_error(uint8_t status);

void fd_nrf8001_directed_connect_success(void);

void fd_nrf8001_close_remote_pipe_error(uint8_t status);

void fd_nrf8001_close_remote_pipe_success(void);

void fd_nrf8001_set_local_data_error(uint8_t status);

void fd_nrf8001_set_local_data_success(void);

// events

void fd_nrf8001_device_started_event(
    uint8_t operating_mode,
    uint8_t hardware_error,
    uint8_t data_credit_available
);

void fd_nrf8001_echo_event(uint8_t *buffer, uint32_t length);

void fd_nrf8001_hardware_error_event(uint16_t line, char *filename);

void fd_nrf8001_connected_event(
    uint8_t address_type,
    uint8_t *peer_address,
    uint16_t connection_interval,
    uint16_t slave_latency,
    uint16_t supervision_timeout,
    uint8_t masterClockAccuracy
);

void fd_nrf8001_disconnected_event(
    uint8_t aci_status,
    uint8_t btle_status
);

void fd_nrf8001_bond_status_event(
    uint8_t bond_status_code,
    uint8_t bond_status_source,
    uint8_t bond_status_sec_mode1,
    uint8_t bond_status_sec_mode2,
    uint8_t bond_status_key_exch_slave,
    uint8_t bond_status_key_exch_master
);

void fd_nrf8001_pipe_status_event(uint64_t pipes_open, uint64_t pipes_closed);

void fd_nrf8001_timing_event(
    uint16_t connection_interval,
    uint16_t slave_latency,
    uint16_t supervision_timeout
);

void fd_nrf8001_display_key_event(char *passkey);

void fd_nrf8001_key_request_event(uint8_t key_type);

void fd_nrf8001_data_credit_event(uint8_t data_credits);

void fd_nrf8001_pipe_error_event(
    uint8_t service_pipe_number,
    uint8_t error_code,
    uint8_t *error_data,
    uint32_t error_data_length
);

void fd_nrf8001_data_received_event(
    uint8_t service_pipe_number,
    uint8_t *data,
    uint32_t data_length
);

void fd_nrf8001_data_ack_event(uint8_t service_pipe_number);

#endif