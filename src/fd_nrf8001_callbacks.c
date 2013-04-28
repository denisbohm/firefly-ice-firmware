#include "fd_nrf8001.h"
#include "fd_nrf8001_callbacks.h"

#define WEAK __attribute__((weak))
// command responses

WEAK
void fd_nrf8001_test_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_dtm_command_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_dtm_command_success(uint16_t dtm_command __attribute__((unused))) {
}

WEAK
void fd_nrf8001_wakeup_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_wakeup_success(void) {
}

WEAK
void fd_nrf8001_setup_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_setup_continue(void) {
}

WEAK
void fd_nrf8001_setup_complete(void) {
}

WEAK
void fd_nrf8001_read_dynamic_data_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_read_dynamic_data_continue(uint32_t sequence_number __attribute__((unused)), uint8_t *data __attribute__((unused)), uint32_t length __attribute__((unused))) {
}

WEAK
void fd_nrf8001_read_dynamic_data_complete(void) {
}

WEAK
void fd_nrf8001_write_dynamic_data_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_write_dynamic_data_continue(void) {
}

WEAK
void fd_nrf8001_write_dynamic_data_complete(void) {
}

WEAK
void fd_nrf8001_get_device_version_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_get_device_version_success(
    uint16_t configuration_id __attribute__((unused)),
    uint8_t aci_protocol_version __attribute__((unused)),
    uint8_t current_setup_format __attribute__((unused)),
    uint32_t setup_id __attribute__((unused)),
    uint8_t configuration_status __attribute__((unused))
) {
}

WEAK
void fd_nrf8001_get_device_address_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_get_device_address_success(
    uint8_t *address __attribute__((unused)),
    uint8_t configuration_id __attribute__((unused))
) {
}

WEAK
void fd_nrf8001_get_battery_level_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_get_battery_level_success(float voltage __attribute__((unused))) {
}

WEAK
void fd_nrf8001_get_temperature_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_get_temperature_success(float temperature __attribute__((unused))) {
}

WEAK
void fd_nrf8001_radio_reset_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_radio_reset_success(void) {
}

WEAK
void fd_nrf8001_connect_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_connect_success(void) {
}

WEAK
void fd_nrf8001_bond_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_bond_success(void) {
}

WEAK
void fd_nrf8001_disconnect_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_disconnect_success(void) {
}

WEAK
void fd_nrf8001_set_tx_power_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_set_tx_power_success(void) {
}

WEAK
void fd_nrf8001_change_timing_request_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_change_timing_request_success(void) {
}

WEAK
void fd_nrf8001_open_remove_pipe_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_open_remove_pipe_success(void) {
}

WEAK
void fd_nrf8001_set_application_latency_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_set_application_latency_success(void) {
}

WEAK
void fd_nrf8001_set_key_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_set_key_success(void) {
}

WEAK
void fd_nrf8001_open_advertising_pipe_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_open_advertising_pipe_success(void) {
}

WEAK
void fd_nrf8001_broadcast_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_broadcast_success(void) {
}

WEAK
void fd_nrf8001_bond_security_request_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_bond_security_request_success(void) {
}

WEAK
void fd_nrf8001_directed_connect_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_directed_connect_success(void) {
}

WEAK
void fd_nrf8001_close_remote_pipe_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_close_remote_pipe_success(void) {
}

WEAK
void fd_nrf8001_set_local_data_error(uint8_t status __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_set_local_data_success(void) {
}

// events

WEAK
void fd_nrf8001_device_started_event(
    uint8_t operating_mode __attribute__((unused)),
    uint8_t hardware_error __attribute__((unused)),
    uint8_t data_credit_available __attribute__((unused))
) {
}

WEAK
void fd_nrf8001_echo_event(uint8_t *buffer __attribute__((unused)), uint32_t length __attribute__((unused))) {
}

WEAK
void fd_nrf8001_hardware_error_event(uint16_t line __attribute__((unused)), char *filename __attribute__((unused))) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_connected_event(
    uint8_t address_type __attribute__((unused)),
    uint8_t *peer_address __attribute__((unused)),
    uint16_t connection_interval __attribute__((unused)),
    uint16_t slave_latency __attribute__((unused)),
    uint16_t supervision_timeout __attribute__((unused)),
    uint8_t masterClockAccuracy __attribute__((unused))
) {
}

WEAK
void fd_nrf8001_disconnected_event(
    uint8_t aci_status __attribute__((unused)),
    uint8_t btle_status __attribute__((unused))
) {
}

WEAK
void fd_nrf8001_bond_status_event(
    uint8_t bond_status_code __attribute__((unused)),
    uint8_t bond_status_source __attribute__((unused)),
    uint8_t bond_status_sec_mode1 __attribute__((unused)),
    uint8_t bond_status_sec_mode2 __attribute__((unused)),
    uint8_t bond_status_key_exch_slave __attribute__((unused)),
    uint8_t bond_status_key_exch_master __attribute__((unused))
) {
}

WEAK
void fd_nrf8001_pipe_status_event(uint64_t pipes_open __attribute__((unused)), uint64_t pipes_closed __attribute__((unused))) {
}

WEAK
void fd_nrf8001_timing_event(
    uint16_t connection_interval __attribute__((unused)),
    uint16_t slave_latency __attribute__((unused)),
    uint16_t supervision_timeout __attribute__((unused))
) {
}

WEAK
void fd_nrf8001_display_key_event(char *passkey __attribute__((unused))) {
}

WEAK
void fd_nrf8001_key_request_event(uint8_t key_type __attribute__((unused))) {
}

WEAK
void fd_nrf8001_data_credit_event(uint8_t data_credits __attribute__((unused))) {
}

WEAK
void fd_nrf8001_pipe_error_event(
    uint8_t service_pipe_number __attribute__((unused)),
    uint8_t error_code __attribute__((unused)),
    uint8_t *error_data __attribute__((unused)),
    uint32_t error_data_length __attribute__((unused))
) {
    fd_nrf8001_error();
}

WEAK
void fd_nrf8001_data_received_event(
    uint8_t service_pipe_number __attribute__((unused)),
    uint8_t *data __attribute__((unused)),
    uint32_t data_length __attribute__((unused))
) {
}

WEAK
void fd_nrf8001_data_ack_event(uint8_t service_pipe_number __attribute__((unused))) {
}