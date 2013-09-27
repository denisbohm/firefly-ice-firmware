#include "fd_nrf8001.h"
#include "fd_nrf8001_commands.h"
#include "fd_nrf8001_types.h"

static
void fd_nrf8001_send_system_command(uint8_t *message, uint32_t length) {
    fd_nrf8001_use_system_credits(1);
    fd_nrf8001_send(message, length);
}

static
void fd_nrf8001_send_system_command_with_data(
    uint8_t *message,
    uint32_t length,
    uint8_t *data,
    uint32_t data_length
) {
    fd_nrf8001_use_system_credits(1);
    fd_nrf8001_send_with_data(message, length, data, data_length);
}

static
void fd_nrf8001_send_data_command(uint8_t *message, uint32_t length) {
    fd_nrf8001_use_data_credits(1);
    fd_nrf8001_send(message, length);
}

static
void fd_nrf8001_send_data_command_with_data(
    uint8_t *message,
    uint32_t length,
    uint8_t *data,
    uint32_t data_length
) {
    fd_nrf8001_use_data_credits(1);
    fd_nrf8001_send_with_data(message, length, data, data_length);
}

void fd_nrf8001_test(uint8_t test_feature) {
    uint8_t message[] = {2, Test, test_feature};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_echo(uint8_t *data, uint32_t length) {
    uint8_t message[] = {1 + length, Echo};
    fd_nrf8001_send_system_command_with_data(message, sizeof(message), data, length);
}

void fd_nrf8001_dtm_command(uint16_t dtm_command) {
    uint8_t message[] = {3, DtmCommand, dtm_command >> 8, dtm_command};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_sleep(void) {
    uint8_t message[] = {1, Sleep};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_wakeup(void) {
    uint8_t message[] = {1, Wakeup};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_setup(uint8_t *data, uint32_t length) {
    uint8_t message[] = {1 + length, Setup};
    fd_nrf8001_send_system_command_with_data(message, sizeof(message), data, length);
}

void fd_nrf8001_read_dynamic_data(void) {
    uint8_t message[] = {1, ReadDynamicData};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_write_dynamic_data(uint8_t sequence_number, uint8_t *data, uint32_t length) {
    uint8_t message[] = {2 + length, WriteDynamicData, sequence_number};
    fd_nrf8001_send_system_command_with_data(message, sizeof(message), data, length);
}

void fd_nrf8001_get_device_version(void) {
    uint8_t message[] = {1, GetDeviceVersion};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_get_device_address(void) {
    uint8_t message[] = {1, GetDeviceAddress};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_get_battery_level(void) {
    uint8_t message[] = {1, GetBatteryLevel};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_get_temperature(void) {
    uint8_t message[] = {1, GetTemperature};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_radio_reset(void) {
    uint8_t message[] = {1, RadioReset};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_connect(uint16_t timeout, uint16_t interval) {
    uint8_t message[] = {5, Connect, timeout, timeout >> 8, interval, interval >> 8};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_bond(uint16_t timeout, uint16_t interval) {
    uint8_t message[] = {5, Bond, timeout, timeout >> 8, interval, interval >> 8};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_disconnect(uint8_t reason) {
    uint8_t message[] = {2, Disconnect, reason};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_set_tx_power(uint8_t level) {
    uint8_t message[] = {2, SetTxPower, level};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_change_timing_request_as_setup(void) {
    uint8_t message[] = {1, ChangeTimingRequest};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_change_timing_request(uint16_t interval_min, uint16_t interval_max, uint16_t latency, uint16_t timeout) {
    uint8_t message[] = {9, ChangeTimingRequest, interval_min, interval_min >> 8, interval_max, interval_max >> 8, latency, latency >> 8, timeout, timeout >> 8};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_open_remote_pipe(uint8_t service_pipe_number) {
    uint8_t message[] = {2, OpenRemotePipe, service_pipe_number};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_set_application_latency(uint8_t mode, uint16_t latency) {
    uint8_t message[] = {4, Bond, mode, latency, latency >> 8};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_set_key_rejected(void) {
    uint8_t message[] = {2, SetKey, 0x00};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_set_key(uint8_t *key) {
    uint8_t message[] = {8, SetKey, 0x01, key[0], key[1], key[2], key[3], key[4], key[5]};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_open_advertising_pipe(uint64_t pipes) {
    uint8_t message[] = {9, OpenAdvPipe, pipes, pipes >> 8, pipes >> 16, pipes >> 24, pipes >> 32, pipes >> 40, pipes >> 48, pipes >> 56};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_broadcast(uint16_t timeout, uint16_t interval) {
    uint8_t message[] = {5, Broadcast, timeout, timeout >> 8, interval, interval >> 8};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_bond_security_request(void) {
    uint8_t message[] = {1, BondSecRequest};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_directed_connect(void) {
    uint8_t message[] = {1, DirectedConnect};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_close_remote_pipe(uint8_t service_pipe_number) {
    uint8_t message[] = {2, CloseRemotePipe, service_pipe_number};
    fd_nrf8001_send_system_command(message, sizeof(message));
}

void fd_nrf8001_set_local_data(uint8_t service_pipe_number, uint8_t *data, uint32_t length) {
    uint8_t message[] = {2 + length, SetLocalData, service_pipe_number};
    fd_nrf8001_send_system_command_with_data(message, sizeof(message), data, length);
}

void fd_nrf8001_send_data(uint8_t service_pipe_number, uint8_t *data, uint32_t length) {
    uint8_t message[] = {2 + length, SendData, service_pipe_number};
    fd_nrf8001_send_data_command_with_data(message, sizeof(message), data, length);
}

void fd_nrf8001_send_data_ack(uint8_t service_pipe_number) {
    uint8_t message[] = {2, SendDataAck, service_pipe_number};
    fd_nrf8001_send_data_command(message, sizeof(message));
}

void fd_nrf8001_request_data(uint8_t service_pipe_number) {
    uint8_t message[] = {2, RequestData, service_pipe_number};
    fd_nrf8001_send_data_command(message, sizeof(message));
}

void fd_nrf8001_send_data_nack(uint8_t service_pipe_number, uint8_t error_code) {
    uint8_t message[] = {3, RequestData, service_pipe_number, error_code};
    fd_nrf8001_send_data_command(message, sizeof(message));
}