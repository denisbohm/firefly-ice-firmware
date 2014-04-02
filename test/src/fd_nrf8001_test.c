#include "fd_nrf8001.h"
#include "fd_nrf8001_callbacks.h"
#include "fd_nrf8001_commands.h"
#include "fd_nrf8001_dispatch.h"
#include "fd_nrf8001_types.h"

#include "services.h"

#define HAL_ACI_MAX_LENGTH 31

typedef struct hal_aci_data_t {
  uint8_t status_byte;
  uint8_t buffer[HAL_ACI_MAX_LENGTH+1];
} hal_aci_data_t;

static hal_aci_data_t setup_msgs[NB_SETUP_MESSAGES] = SETUP_MESSAGES_CONTENT;

#define MAX_CHARACTERISTIC_SIZE 20

#define BIT(n) (1 << (n))

#define fd_bluetooth_setup_continue_step BIT(0)
#define fd_bluetooth_set_device_name_step BIT(1)
#define fd_bluetooth_connect_step BIT(2)
#define fd_bluetooth_change_timing_request_step BIT(3)
#define fd_bluetooth_open_remote_pipe_step BIT(4)
#define fd_bluetooth_sleep_step BIT(5)
#define fd_bluetooth_wakeup_step BIT(6)
#define fd_bluetooth_test_enable_step BIT(7)
#define fd_bluetooth_test_command_step BIT(8)
#define fd_bluetooth_test_exit_step BIT(9)

static uint32_t fd_bluetooth_system_steps;
static volatile uint32_t fd_bluetooth_data_acks;
static uint32_t fd_bluetooth_initial_data_credits;
static uint32_t fd_bluetooth_setup_index;
static uint64_t fd_bluetooth_pipes_open;
static uint64_t fd_bluetooth_pipes_closed;
static bool fd_bluetooth_idle;
static uint16_t fd_bluetooth_dtm_request;
static uint16_t fd_bluetooth_dtm_data;
static uint8_t fd_bluetooth_out_data[MAX_CHARACTERISTIC_SIZE];

uint8_t error_code;
uint8_t error_status;

bool fd_bluetooth_did_setup;
bool fd_bluetooth_did_connect;
bool fd_bluetooth_did_open_pipes;
bool fd_bluetooth_did_receive_data;

uint8_t *fd_bluetooth_result;
uint32_t fd_bluetooth_result_length;
bool fd_bluetooth_done;

typedef struct {
    uint8_t bytes[20];
} fd_test_packet_t;

static fd_test_packet_t fd_send_packets[10];
static uint32_t fd_send_packet_index;

bool fd_bluetooth_spi_transfer(void);
void fd_bluetooth_step(void);

void fd_bluetooth_ready(void) {
    fd_bluetooth_spi_transfer();
    fd_bluetooth_step();
}

void fd_bluetooth_initialize(void) {
    fd_bluetooth_system_steps = 0;
    fd_bluetooth_data_acks = 0;
    fd_bluetooth_initial_data_credits = 0;
    fd_bluetooth_setup_index = 0;
    fd_bluetooth_pipes_open = 0;
    fd_bluetooth_pipes_closed = 0;
    fd_bluetooth_idle = false;

    fd_bluetooth_did_setup = false;
    fd_bluetooth_did_connect = false;
    fd_bluetooth_did_open_pipes = false;
    fd_bluetooth_did_receive_data = false;

    fd_bluetooth_result = 0;
    fd_bluetooth_result_length = 0;
    fd_bluetooth_done = false;

    fd_send_packet_index = 0;
    for (int i = 0; i < 10; ++i) {
        fd_test_packet_t *packet = &fd_send_packets[i];
        for (int j = 0; j < 20; ++j) {
            packet->bytes[j] = i + 1;
        }
    }

    error_code = 0;
    error_status = 0;
}

void fd_nrf8001_error(void) {
    error_status = 1;
}

bool fd_bluetooth_is_asleep(void) {
    return fd_bluetooth_idle;
}

void fd_bluetooth_sleep(void) {
    fd_bluetooth_system_steps = fd_bluetooth_sleep_step;
}

void fd_bluetooth_wake(void) {
    fd_bluetooth_idle = false;
    fd_bluetooth_system_steps = fd_bluetooth_wakeup_step;
}

void fd_bluetooth_step_queue(uint32_t step) {
    fd_bluetooth_system_steps |= step;
}

void fd_bluetooth_step_complete(uint32_t step) {
    fd_bluetooth_system_steps &= ~step;
}

static uint8_t hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

#define UNIQUE 0x0FE081F0

static void get_hardware_id(uint8_t *s) {
    uint8_t *p = (uint8_t*)UNIQUE;
    for (int i = 0; i < 8; ++i) {
        uint8_t b = *p++;
        *s++ = hex[b >> 4];
        *s++ = hex[b & 0xf];
    }
}

void fd_bluetooth_system_step(void) {
    while ((fd_bluetooth_system_steps != 0) && fd_nrf8001_has_system_credits()) {
        if (fd_bluetooth_system_steps & fd_bluetooth_setup_continue_step) {
            // nothing to do, just waiting for response
        } else
        if (fd_bluetooth_system_steps & fd_bluetooth_sleep_step) {
            fd_nrf8001_sleep();
            fd_bluetooth_step_complete(fd_bluetooth_sleep_step);
            fd_bluetooth_idle = true;
        } else
        if (fd_bluetooth_system_steps & fd_bluetooth_wakeup_step) {
            fd_nrf8001_wakeup();
            fd_bluetooth_step_complete(fd_bluetooth_wakeup_step);
        } else
        if (fd_bluetooth_system_steps & fd_bluetooth_set_device_name_step) {
            uint8_t hwid[20] = "hwid";
            get_hardware_id(&hwid[4]);
            fd_nrf8001_set_local_data(PIPE_GAP_DEVICE_NAME_SET, hwid, 20);
            fd_bluetooth_step_complete(fd_bluetooth_set_device_name_step);
            fd_bluetooth_step_queue(fd_bluetooth_connect_step);
        } else
        if (fd_bluetooth_system_steps & fd_bluetooth_connect_step) {
            uint16_t timeout = 0; // infinite advertisement - no timeout
            uint16_t interval = 32; // 20ms (0.625ms units)
            fd_nrf8001_connect(timeout, interval);
            fd_bluetooth_step_complete(fd_bluetooth_connect_step);
        } else
        if (fd_bluetooth_system_steps & fd_bluetooth_change_timing_request_step) {
            uint16_t interval_min = 16; // 20ms (1.25ms units)
            uint16_t interval_max = 32; // 40ms (1.25ms units)
            uint16_t latency = 0;
            uint16_t timeout = 600; // 6s (10ms units)
            fd_nrf8001_change_timing_request(interval_min, interval_max, latency, timeout);
            fd_bluetooth_step_complete(fd_bluetooth_change_timing_request_step);
        } else
        if (fd_bluetooth_system_steps & fd_bluetooth_open_remote_pipe_step) {
            for (int i = 1; i < 63; ++i) {
                uint64_t mask = 1 << i;
                if (fd_bluetooth_pipes_closed & mask) {
                    fd_nrf8001_open_remote_pipe(i);
                    fd_bluetooth_pipes_closed &= ~mask;
                    break;
                }
            }
            if (fd_bluetooth_pipes_closed == 0) {
                fd_bluetooth_step_complete(fd_bluetooth_open_remote_pipe_step);
            }
        } else
        if (fd_bluetooth_system_steps & fd_bluetooth_test_enable_step) {
            fd_nrf8001_test(TestFeatureEnableDTMOverACI);
        } else
        if (fd_bluetooth_system_steps & fd_bluetooth_test_command_step) {
            fd_nrf8001_dtm_command(fd_bluetooth_dtm_request);
        } else
        if (fd_bluetooth_system_steps & fd_bluetooth_test_exit_step) {
            fd_nrf8001_test(TestFeatureExitTestMode);
        }
    }
}

bool fd_bluetooth_is_pipe_open(uint32_t service_pipe_number) {
    return fd_bluetooth_pipes_open & (1 << service_pipe_number) ? true : false;
}

void fd_bluetooth_data_step(void) {
    while ((fd_bluetooth_data_acks > 0) && fd_nrf8001_has_data_credits() && fd_bluetooth_is_pipe_open(PIPE_DEVICE_INFORMATION_MODEL_NUMBER_STRING_TX) && (fd_send_packet_index < 10)) {
        fd_nrf8001_send_data(PIPE_DEVICE_INFORMATION_MODEL_NUMBER_STRING_TX, fd_send_packets[fd_send_packet_index++].bytes, 20);
        --fd_bluetooth_data_acks;
    }
}

void fd_bluetooth_transfer(void) {
}

void fd_bluetooth_step(void) {
    if (fd_bluetooth_idle) {
        return;
    }

    fd_bluetooth_system_step();
    fd_bluetooth_data_step();
    fd_bluetooth_transfer();
}

void fd_nrf8001_device_started_event(
    uint8_t operating_mode,
    uint8_t hardware_error __attribute__((unused)),
    uint8_t data_credit_available
) {
    fd_bluetooth_initial_data_credits = data_credit_available;

    switch (operating_mode) {
        case OperatingModeTest: {
        } break;
        case OperatingModeStandby: {
            fd_bluetooth_step_queue(fd_bluetooth_set_device_name_step);
        } break;
        case OperatingModeSetup: {
            fd_nrf8001_set_system_credits(1);
            fd_bluetooth_setup_index = 0;
            fd_bluetooth_step_queue(fd_bluetooth_setup_continue_step);
            fd_nrf8001_setup_continue();
        } break;
    }
}

void fd_nrf8001_pipe_error_event(
    uint8_t service_pipe_number __attribute__((unused)),
    uint8_t error_code __attribute__((unused)),
    uint8_t *error_data __attribute__((unused)),
    uint32_t error_data_length __attribute__((unused))
) {
    fd_nrf8001_error();
}

void fd_nrf8001_setup_continue(void) {
    if (fd_bluetooth_setup_index >= NB_SETUP_MESSAGES) {
        return;
    }
    fd_nrf8001_use_system_credits(1);
    const hal_aci_data_t *setup_msg = &setup_msgs[fd_bluetooth_setup_index++];
    uint8_t *buffer = (uint8_t *)setup_msg->buffer;
    uint32_t length = buffer[0] + 1;
    fd_nrf8001_send(buffer, length);
}

void fd_nrf8001_setup_complete(void) {
    fd_bluetooth_did_setup = true;
    fd_bluetooth_step_complete(fd_bluetooth_setup_continue_step);
}

void fd_nrf8001_connected_event(
    uint8_t address_type __attribute__((unused)),
    uint8_t *peer_address __attribute__((unused)),
    uint16_t connection_interval __attribute__((unused)),
    uint16_t slave_latency __attribute__((unused)),
    uint16_t supervision_timeout __attribute__((unused)),
    uint8_t masterClockAccuracy __attribute__((unused))
) {
    fd_bluetooth_did_connect = true;

    fd_nrf8001_set_data_credits(fd_bluetooth_initial_data_credits);

    fd_bluetooth_step_queue(fd_bluetooth_change_timing_request_step);
}

void fd_nrf8001_disconnected_event(
    uint8_t aci_status __attribute__((unused)),
    uint8_t btle_status __attribute__((unused))
) {
    fd_bluetooth_system_steps = 0;
    fd_bluetooth_data_acks = 0;
    fd_bluetooth_pipes_open = 0;
    fd_bluetooth_pipes_closed = 0;
    fd_bluetooth_did_connect = false;
    fd_bluetooth_did_open_pipes = false;
    fd_bluetooth_did_receive_data = false;

    fd_nrf8001_set_data_credits(0);

    fd_bluetooth_step_queue(fd_bluetooth_connect_step);

    fd_bluetooth_done = true;
}

void fd_nrf8001_data_credit_event(uint8_t data_credits) {
    fd_nrf8001_add_data_credits(data_credits);
}

void fd_nrf8001_pipe_status_event(uint64_t pipes_open, uint64_t pipes_closed) {
    fd_bluetooth_pipes_open = pipes_open;
    fd_bluetooth_pipes_closed = pipes_closed;
    if (fd_bluetooth_pipes_closed) {
        fd_bluetooth_step_queue(fd_bluetooth_open_remote_pipe_step);
    } else {
        fd_bluetooth_did_open_pipes = true;
    }
}

#define FD_NRF8001_COMMAND_DATA 1
#define FD_NRF8001_COMMAND_DONE 2

void fd_nrf8001_data_received_event(
    uint8_t service_pipe_number,
    uint8_t *data,
    uint32_t data_length
) {
    fd_bluetooth_did_receive_data = true;

    if ((service_pipe_number != PIPE_DEVICE_INFORMATION_MODEL_NUMBER_STRING_RX) || (data_length < 20)) {
        return;
    }

    uint32_t command = data[0];
    switch (command) {
        case FD_NRF8001_COMMAND_DATA: {
            uint32_t index = data[1];
            uint8_t byte = data[2];
            if (index < fd_bluetooth_result_length) {
                fd_bluetooth_result[index] = byte;
                ++fd_bluetooth_data_acks;
            }
        } break;
        case FD_NRF8001_COMMAND_DONE: {
        } break;
    }
}

uint32_t fd_nrf8001_test_broadcast(uint8_t *result, uint32_t result_length) {
    fd_nrf8001_initialize();
    fd_bluetooth_initialize();
    fd_bluetooth_result = result;
    fd_bluetooth_result_length = result_length;
    while ((error_code == 0) && !fd_bluetooth_done) {
        fd_bluetooth_ready();
    }
    return (fd_bluetooth_system_steps << 16) | (error_code << 8) | error_status;
}
