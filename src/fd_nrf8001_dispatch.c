#include "fd_binary.h"
#include "fd_log.h"
#include "fd_nrf8001.h"
#include "fd_nrf8001_callbacks.h"
#include "fd_nrf8001_dispatch.h"
#include "fd_nrf8001_types.h"

#include <stdint.h>

void fd_nrf8001_dispatch_test_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    fd_nrf8001_test_error(status);
}

void fd_nrf8001_dispatch_dtm_command_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_dtm_command_error(status);
        return;
    }
    if (response_data_length != 2) {
        fd_log_assert_fail("");
        return;
    }
    uint16_t dtm_command = fd_binary_get_uint16(response_data);
    fd_nrf8001_dtm_command_success(dtm_command);
}

void fd_nrf8001_dispatch_wakeup_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_wakeup_error(status);
        return;
    }
    fd_nrf8001_wakeup_success();
}

void fd_nrf8001_dispatch_setup_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    switch (status) {
        case ACI_STATUS_TRANSACTION_CONTINUE:
            fd_nrf8001_setup_continue();
        break;
        case ACI_STATUS_TRANSACTION_COMPLETE:
            fd_nrf8001_setup_complete();
        break;
        default:
            fd_nrf8001_setup_error(status);
        break;
    }
}

void fd_nrf8001_dispatch_read_dynamic_data_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    switch (status) {
        case ACI_STATUS_TRANSACTION_CONTINUE: {
            uint8_t sequence_number = response_data[0];
            uint8_t *data = &response_data[1];
            uint32_t length = response_data_length - 1;
            fd_nrf8001_read_dynamic_data_continue(data, length);
        } break;
        case ACI_STATUS_TRANSACTION_COMPLETE:
            fd_nrf8001_read_dynamic_data_complete();
        break;
        default:
            fd_nrf8001_read_dynamic_data_error(status);
        break;
    }
}

void fd_nrf8001_dispatch_write_dynamic_data_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    switch (status) {
        case ACI_STATUS_TRANSACTION_CONTINUE:
            fd_nrf8001_write_dynamic_data_continue();
        break;
        case ACI_STATUS_TRANSACTION_COMPLETE:
            fd_nrf8001_write_dynamic_data_complete();
        break;
        default:
            fd_nrf8001_write_dynamic_data_error(status);
        break;
    }
}

void fd_nrf8001_dispatch_get_device_version_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_get_device_version_error(status);
        return;
    }
    uint16_t configuration_id = fd_binary_get_uint16(response_data);
    uint8_t aci_protocol_version = response_data[2];
    uint8_t current_setup_format = response_data[3];
    uint32_t setup_id = fd_binary_get_uint32(&response_data[4]);
    uint8_t configuration_status = response_data[8];
    fd_nrf8001_get_device_version_success(
        configuration_id,
        aci_protocol_version,
        current_setup_format,
        setup_id,
        configuration_status
    );
}

void fd_nrf8001_dispatch_get_device_address_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_get_device_address_error(status);
        return;
    }
    uint8_t *address = response_data;
    uint8_t configuration_id = response_data[6];
    fd_nrf8001_get_device_address_success(address, configuration_id);
}

void fd_nrf8001_dispatch_get_battery_level_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_get_battery_level_error(status);
        return;
    }
    uint16_t level = fd_binary_get_uint16(response_data);
    float voltage = level * 3.52f;
    fd_nrf8001_get_battery_level_success(voltage);
}

void fd_nrf8001_dispatch_get_temperature_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_get_temperature_error(status);
        return;
    }
    int16_t level = fd_binary_get_uint16(response_data);
    float temperature = level * 0.25f;
    fd_nrf8001_get_temperature_success(temperature);
}

void fd_nrf8001_dispatch_radio_reset_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_radio_reset_error(status);
        return;
    }
    fd_nrf8001_radio_reset_success();
}

void fd_nrf8001_dispatch_connect_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_connect_error(status);
        return;
    }
    fd_nrf8001_connect_success();
}

void fd_nrf8001_dispatch_bond_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_bond_error(status);
        return;
    }
    fd_nrf8001_bond_success();
}

void fd_nrf8001_dispatch_disconnect_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_disconnect_error(status);
        return;
    }
    fd_nrf8001_disconnect_success();
}

void fd_nrf8001_dispatch_set_tx_power_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_set_tx_power_error(status);
        return;
    }
    fd_nrf8001_set_tx_power_success();
}

void fd_nrf8001_dispatch_change_timing_request_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_change_timing_request_error(status);
        return;
    }
    fd_nrf8001_change_timing_request_success();
}

void fd_nrf8001_dispatch_open_remove_pipe_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_open_remove_pipe_error(status);
        return;
    }
    fd_nrf8001_open_remove_pipe_success();
}

void fd_nrf8001_dispatch_set_application_latency_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_set_application_latency_error(status);
        return;
    }
    fd_nrf8001_set_application_latency_success();
}

void fd_nrf8001_dispatch_set_key_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_set_key_error(status);
        return;
    }
    fd_nrf8001_set_key_success();
}

void fd_nrf8001_dispatch_open_advertising_pipe_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_open_advertising_pipe_error(status);
        return;
    }
    fd_nrf8001_open_advertising_pipe_success();
}

void fd_nrf8001_dispatch_broadcast_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_broadcast_error(status);
        return;
    }
    fd_nrf8001_broadcast_success();
}

void fd_nrf8001_dispatch_bond_security_request_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_bond_security_request_error(status);
        return;
    }
    fd_nrf8001_bond_security_request_success();
}

void fd_nrf8001_dispatch_directed_connect_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_directed_connect_error(status);
        return;
    }
    fd_nrf8001_directed_connect_success();
}

void fd_nrf8001_dispatch_close_remote_pipe_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_close_remote_pipe_error(status);
        return;
    }
    fd_nrf8001_close_remote_pipe_success();
}

void fd_nrf8001_dispatch_set_local_data_response(
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    if (status != ACI_STATUS_SUCCESS) {
        fd_nrf8001_set_local_data_error(status);
        return;
    }
    fd_nrf8001_set_local_data_success();
}

void fd_nrf8001_command_response_event(
    uint8_t command_op_code,
    uint8_t status,
    uint8_t *response_data,
    uint32_t response_data_length
) {
    switch (command_op_code) {
        case Test:
            fd_nrf8001_dispatch_test_response(status, response_data, response_data_length);
        break;
        case DtmCommand:
            fd_nrf8001_dispatch_dtm_command_response(status, response_data, response_data_length);
        break;
        case Wakeup:
            fd_nrf8001_dispatch_wakeup_response(status, response_data, response_data_length);
        break;
        case Setup:
            fd_nrf8001_dispatch_setup_response(status, response_data, response_data_length);
        break;
        case ReadDynamicData:
            fd_nrf8001_dispatch_read_dynamic_data_response(status, response_data, response_data_length);
        break;
        case WriteDynamicData:
            fd_nrf8001_dispatch_write_dynamic_data_response(status, response_data, response_data_length);
        break;
        case GetDeviceVersion:
            fd_nrf8001_dispatch_get_device_version_response(status, response_data, response_data_length);
        break;
        case GetDeviceAddress:
            fd_nrf8001_dispatch_get_device_address_response(status, response_data, response_data_length);
        break;
        case GetBatteryLevel:
            fd_nrf8001_dispatch_get_battery_level_response(status, response_data, response_data_length);
        break;
        case GetTemperature:
            fd_nrf8001_dispatch_get_temperature_response(status, response_data, response_data_length);
        break;
        case RadioReset:
            fd_nrf8001_dispatch_radio_reset_response(status, response_data, response_data_length);
        break;
        case Connect:
            fd_nrf8001_dispatch_connect_response(status, response_data, response_data_length);
        break;
        case Bond:
            fd_nrf8001_dispatch_bond_response(status, response_data, response_data_length);
        break;
        case Disconnect:
            fd_nrf8001_dispatch_disconnect_response(status, response_data, response_data_length);
        break;
        case SetTxPower:
            fd_nrf8001_dispatch_set_tx_power_response(status, response_data, response_data_length);
        break;
        case ChangeTimingRequest:
            fd_nrf8001_dispatch_change_timing_request_response(status, response_data, response_data_length);
        break;
        case OpenRemotePipe:
            fd_nrf8001_dispatch_open_remove_pipe_response(status, response_data, response_data_length);
        break;
        case SetApplicationLatency:
            fd_nrf8001_dispatch_set_application_latency_response(status, response_data, response_data_length);
        break;
        case SetKey:
            fd_nrf8001_dispatch_set_key_response(status, response_data, response_data_length);
        break;
        case OpenAdvPipe:
            fd_nrf8001_dispatch_open_advertising_pipe_response(status, response_data, response_data_length);
        break;
        case Broadcast:
            fd_nrf8001_dispatch_broadcast_response(status, response_data, response_data_length);
        break;
        case BondSecRequest:
            fd_nrf8001_dispatch_bond_security_request_response(status, response_data, response_data_length);
        break;
        case DirectedConnect:
            fd_nrf8001_dispatch_directed_connect_response(status, response_data, response_data_length);
        break;
        case CloseRemotePipe:
            fd_nrf8001_dispatch_close_remote_pipe_response(status, response_data, response_data_length);
        break;
        case SetLocalData:
            fd_nrf8001_dispatch_set_local_data_response(status, response_data, response_data_length);
        break;
        default:
            fd_log_assert_fail("");
        break;
    }
}

void fd_nrf8001_decode_device_started_event(uint8_t *buffer, uint32_t length) {
    if (length != 4) {
        fd_log_assert_fail("");
        return;
    }
    uint8_t operating_mode = buffer[1];
    uint8_t hardware_error = buffer[2];
    uint8_t data_credit_available = buffer[3];
    fd_nrf8001_device_started_event(operating_mode, hardware_error, data_credit_available);
}

void fd_nrf8001_decode_echo_event(uint8_t *buffer, uint32_t length) {
    fd_nrf8001_echo_event(&buffer[1], length - 1);
}

void fd_nrf8001_decode_hardware_error_event(uint8_t *buffer, uint32_t length) {
    uint16_t line = fd_binary_get_uint16(&buffer[1]);
    char *filename = (char *)&buffer[3];
    fd_nrf8001_hardware_error_event(line, filename);
}

void fd_nrf8001_decode_command_response_event(uint8_t *buffer, uint32_t length) {
    if (length < 3) {
        fd_log_assert_fail("");
        return;
    }
    uint8_t command_op_code = buffer[1];
    uint8_t status = buffer[2];
    uint8_t *response_data = &buffer[3];
    uint32_t response_data_length = length - 3;
    fd_nrf8001_command_response_event(command_op_code, status, response_data, response_data_length);
}

void fd_nrf8001_decode_connected_event(uint8_t *buffer, uint32_t length) {
    if (length != 15) {
        fd_log_assert_fail("");
        return;
    }
    uint8_t address_type = buffer[1];
    uint8_t *peer_address = &buffer[2];
    uint16_t connection_interval = fd_binary_get_uint16(&buffer[8]);
    uint16_t slave_latency = fd_binary_get_uint16(&buffer[10]);
    uint16_t supervision_timeout = fd_binary_get_uint16(&buffer[12]);
    uint8_t masterClockAccuracy = buffer[14];
    fd_nrf8001_connected_event(address_type, peer_address, connection_interval, slave_latency, supervision_timeout, masterClockAccuracy);
}

void fd_nrf8001_decode_disconnected_event(uint8_t *buffer, uint32_t length) {
    if (length != 3) {
        fd_log_assert_fail("");
        return;
    }
    uint8_t aci_status = buffer[1];
    uint8_t btle_status = buffer[2];
    fd_nrf8001_disconnected_event(aci_status, btle_status);
}

void fd_nrf8001_decode_bond_status_event(uint8_t *buffer, uint32_t length) {
    if (length != 7) {
        fd_log_assert_fail("");
        return;
    }
    uint8_t bond_status_code = buffer[1];
    uint8_t bond_status_source = buffer[2];
    uint8_t bond_status_sec_mode1 = buffer[3];
    uint8_t bond_status_sec_mode2 = buffer[4];
    uint8_t bond_status_key_exch_slave = buffer[5];
    uint8_t bond_status_key_exch_master = buffer[6];
    fd_nrf8001_bond_status_event(
        bond_status_code,
        bond_status_source,
        bond_status_sec_mode1,
        bond_status_sec_mode2,
        bond_status_key_exch_slave,
        bond_status_key_exch_master
    );
}

void fd_nrf8001_decode_pipe_status_event(uint8_t *buffer, uint32_t length) {
    if (length != 17) {
        fd_log_assert_fail("");
        return;
    }
    uint64_t pipes_open = fd_binary_get_uint64(&buffer[1]);
    uint64_t pipes_closed = fd_binary_get_uint64(&buffer[9]);
    fd_nrf8001_pipe_status_event(pipes_open, pipes_closed);
}

void fd_nrf8001_decode_timing_event(uint8_t *buffer, uint32_t length) {
    if (length != 7) {
        fd_log_assert_fail("");
        return;
    }
    uint16_t connection_interval = fd_binary_get_uint16(&buffer[1]);
    uint16_t slave_latency = fd_binary_get_uint16(&buffer[3]);
    uint16_t supervision_timeout = fd_binary_get_uint16(&buffer[5]);
    fd_nrf8001_timing_event(connection_interval, slave_latency, supervision_timeout);
}

void fd_nrf8001_decode_display_key_event(uint8_t *buffer, uint32_t length) {
    if (length != 7) {
        fd_log_assert_fail("");
        return;
    }
    char *passkey = (char *)&buffer[1];
    fd_nrf8001_display_key_event(passkey);
}

void fd_nrf8001_decode_key_request_event(uint8_t *buffer, uint32_t length) {
    if (length != 2) {
        fd_log_assert_fail("");
        return;
    }
    uint8_t key_type = buffer[1];
    fd_nrf8001_key_request_event(key_type);
}

void fd_nrf8001_decode_data_credit_event(uint8_t *buffer, uint32_t length) {
    if (length != 2) {
        fd_log_assert_fail("");
        return;
    }
    uint8_t data_credits = buffer[1];
    fd_nrf8001_data_credit_event(data_credits);
}

void fd_nrf8001_decode_pipe_error_event(uint8_t *buffer, uint32_t length) {
    if (length < 3) {
        fd_log_assert_fail("");
        return;
    }
    uint8_t service_pipe_number = buffer[1];
    uint8_t error_code = buffer[2];
    uint8_t *error_data = &buffer[3];
    uint32_t error_data_length = length - 3;
    fd_nrf8001_pipe_error_event(service_pipe_number, error_code, error_data, error_data_length);
}

void fd_nrf8001_decode_data_received_event(uint8_t *buffer, uint32_t length) {
    if (length < 2) {
        fd_log_assert_fail("");
        return;
    }
    uint8_t service_pipe_number = buffer[1];
    uint8_t *data = &buffer[2];
    uint32_t data_length = length - 2;
    fd_nrf8001_data_received_event(service_pipe_number, data, data_length);
}

void fd_nrf8001_decode_data_ack_event(uint8_t *buffer, uint32_t length) {
    if (length != 2) {
        fd_log_assert_fail("");
        return;
    }
    uint8_t service_pipe_number = buffer[1];
    fd_nrf8001_data_ack_event(service_pipe_number);
}

void fd_nrf8001_dispatch(uint8_t *buffer, uint32_t length) {
    uint8_t op = buffer[0];
    switch (op) {
        case DeviceStartedEvent:
            fd_nrf8001_decode_device_started_event(buffer, length);
        break;
        case EchoEvent:
            fd_nrf8001_decode_echo_event(buffer, length);
        break;
        case HardwareErrorEvent:
            fd_nrf8001_decode_hardware_error_event(buffer, length);
        break;
        case CommandResponseEvent:
            fd_nrf8001_decode_command_response_event(buffer, length);
        break;
        case ConnectedEvent:
            fd_nrf8001_decode_connected_event(buffer, length);
        break;
        case DisconnectedEvent:
            fd_nrf8001_decode_disconnected_event(buffer, length);
        break;
        case BondStatusEvent:
            fd_nrf8001_decode_bond_status_event(buffer, length);
        break;
        case PipeStatusEvent:
            fd_nrf8001_decode_pipe_status_event(buffer, length);
        break;
        case TimingEvent:
            fd_nrf8001_decode_timing_event(buffer, length);
        break;
        case DisplayKeyEvent:
            fd_nrf8001_decode_display_key_event(buffer, length);
        break;
        case KeyRequestEvent:
            fd_nrf8001_decode_key_request_event(buffer, length);
        break;
        case DataCreditEvent:
            fd_nrf8001_decode_data_credit_event(buffer, length);
        break;
        case PipeErrorEvent:
            fd_nrf8001_decode_pipe_error_event(buffer, length);
        break;
        case DataReceivedEvent:
            fd_nrf8001_decode_data_received_event(buffer, length);
        break;
        case DataAckEvent:
            fd_nrf8001_decode_data_ack_event(buffer, length);
        break;
        default:
            fd_log_assert_fail("");
            return;
        break;
    }
}