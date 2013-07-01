#include "fd_bluetooth.h"
#include "fd_control.h"
#include "fd_detour.h"
#include "fd_log.h"
#include "fd_nrf8001.h"
#include "fd_nrf8001_callbacks.h"
#include "fd_nrf8001_commands.h"
#include "fd_nrf8001_types.h"

#define PIPE_FIREFLY_ICE_DETOUR_TX 1
#define PIPE_FIREFLY_ICE_DETOUR_RX_ACK 2

#define NB_SETUP_MESSAGES 17
#define SETUP_MESSAGES_CONTENT {\
    {0x00,\
        {\
            0x07,0x06,0x00,0x00,0x03,0x02,0x41,0xd7,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x02,0x01,0x01,0x00,0x00,0x06,0x00,0x07,\
            0xd0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x10,0x1c,0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x14,0x03,0x90,0x01,0xff,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x10,0x38,0xff,0xff,0x02,0x58,0x00,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x05,0x06,0x10,0x54,0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x00,0x04,0x04,0x02,0x02,0x00,0x01,0x28,0x00,0x01,0x00,0x18,0x04,0x04,0x05,0x05,0x00,\
            0x02,0x28,0x03,0x01,0x0e,0x03,0x00,0x00,0x2a,0x04,0x14,0x07,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x1c,0x07,0x00,0x03,0x2a,0x00,0x01,0x46,0x69,0x72,0x65,0x66,0x6c,0x79,0x04,0x04,0x05,\
            0x05,0x00,0x04,0x28,0x03,0x01,0x02,0x05,0x00,0x01,0x2a,0x06,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x38,0x04,0x03,0x02,0x00,0x05,0x2a,0x01,0x01,0x00,0x00,0x04,0x04,0x05,0x05,0x00,0x06,\
            0x28,0x03,0x01,0x02,0x07,0x00,0x04,0x2a,0x06,0x04,0x09,0x08,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x54,0x00,0x07,0x2a,0x04,0x01,0xff,0xff,0xff,0xff,0x00,0x00,0xff,0xff,0x04,0x04,0x02,\
            0x02,0x00,0x08,0x28,0x00,0x01,0x01,0x18,0x04,0x04,0x10,0x10,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x70,0x00,0x09,0x28,0x00,0x01,0x99,0x63,0x84,0x81,0xa6,0xb7,0xbd,0xb0,0x91,0x50,0x95,\
            0x1b,0x01,0x00,0x0a,0x31,0x04,0x04,0x13,0x13,0x00,0x0a,0x28,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x8c,0x03,0x01,0x18,0x0b,0x00,0x99,0x63,0x84,0x81,0xa6,0xb7,0xbd,0xb0,0x91,0x50,0x95,\
            0x1b,0x02,0x00,0x0a,0x31,0x54,0x10,0x14,0x00,0x00,0x0b,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0xa8,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
            0x00,0x00,0x00,0x00,0x00,0x00,0x46,0x14,0x03,0x02,0x00,0x0c,\
        },\
    },\
    {0x00,\
        {\
            0x09,0x06,0x20,0xc4,0x29,0x02,0x01,0x00,0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x17,0x06,0x40,0x00,0x2a,0x00,0x01,0x00,0x00,0x04,0x00,0x03,0x00,0x00,0x00,0x02,0x02,0x00,0x12,0x04,\
            0x00,0x0b,0x00,0x0c,\
        },\
    },\
    {0x00,\
        {\
            0x13,0x06,0x50,0x00,0x99,0x63,0x84,0x81,0xa6,0xb7,0xbd,0xb0,0x91,0x50,0x95,0x1b,0x00,0x00,0x0a,0x31,\
        },\
    },\
    {0x00,\
        {\
            0x09,0x06,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x06,0x06,0xf0,0x00,0x03,0x2e,0x67,\
        },\
    },\
}

#define HAL_ACI_MAX_LENGTH 31

typedef struct hal_aci_data_t {
  uint8_t status_byte;
  uint8_t buffer[HAL_ACI_MAX_LENGTH+1];
} hal_aci_data_t;

static const hal_aci_data_t setup_msgs[NB_SETUP_MESSAGES] = SETUP_MESSAGES_CONTENT;

#define MAX_CHARACTERISTIC_SIZE 20

#define BIT(n) (1 << (n))

#define fd_nrf8001_setup_continue_step BIT(0)
#define fd_nrf8001_connect_step BIT(1)
#define fd_nrf8001_change_timing_request_step BIT(2)
#define fd_nrf8001_open_remote_pipe_step BIT(3)
#define fd_nrf8001_sleep_step BIT(4)
#define fd_nrf8001_wakeup_step BIT(5)

#define fd_nrf8001_detour_send_data_ack_step BIT(0)

static uint32_t fd_bluetooth_system_steps;
static uint32_t fd_bluetooth_data_steps;
static uint32_t fd_bluetooth_initial_data_credits;
static uint32_t fd_bluetooth_setup_index;
static uint64_t fd_bluetooth_pipes_open;
static uint64_t fd_bluetooth_pipes_closed;
static bool fd_bluetooth_idle;

#define DETOUR_SIZE 256

static uint8_t fd_bluetooth_detour_data[DETOUR_SIZE];
static fd_detour_t fd_bluetooth_detour;

static uint8_t fd_bluetooth_out_data[MAX_CHARACTERISTIC_SIZE];

fd_detour_source_collection_t fd_bluetooth_detour_source_collection;

bool fd_nrf8001_did_setup;
bool fd_nrf8001_did_connect;
bool fd_nrf8001_did_open_pipes;
bool fd_nrf8001_did_receive_data;

void fd_bluetooth_initialize(void) {
    fd_bluetooth_system_steps = 0;
    fd_bluetooth_data_steps = 0;
    fd_bluetooth_initial_data_credits = 0;
    fd_bluetooth_setup_index = 0;
    fd_bluetooth_pipes_open = 0;
    fd_bluetooth_pipes_closed = 0;
    fd_bluetooth_idle = false;

    fd_detour_initialize(&fd_bluetooth_detour, fd_bluetooth_detour_data, DETOUR_SIZE);

    fd_detour_source_collection_initialize(&fd_bluetooth_detour_source_collection);

    fd_nrf8001_did_setup = false;
    fd_nrf8001_did_connect = false;
    fd_nrf8001_did_open_pipes = false;
    fd_nrf8001_did_receive_data = false;
}

bool fd_bluetooth_is_asleep(void) {
    return fd_bluetooth_idle;
}

void fd_bluetooth_sleep(void) {
    fd_bluetooth_system_steps = fd_nrf8001_sleep_step;
}

void fd_bluetooth_wake(void) {
    fd_bluetooth_idle = false;
    fd_bluetooth_system_steps = fd_nrf8001_wakeup_step;
}

void fd_bluetooth_step_queue(uint32_t step) {
    fd_bluetooth_system_steps |= step;
}

void fd_bluetooth_step_complete(uint32_t step) {
    fd_bluetooth_system_steps &= ~step;
}

void fd_bluetooth_data_step_queue(uint32_t step) {
    fd_bluetooth_data_steps |= step;
}

void fd_bluetooth_data_step_complete(uint32_t step) {
    fd_bluetooth_data_steps &= ~step;
}

void fd_bluetooth_system_step(void) {
    while ((fd_bluetooth_system_steps != 0) && fd_nrf8001_has_system_credits()) {
        if (fd_bluetooth_system_steps & fd_nrf8001_sleep_step) {
            fd_nrf8001_sleep();
            fd_bluetooth_step_complete(fd_nrf8001_sleep_step);
            fd_bluetooth_idle = true;
        } else
        if (fd_bluetooth_system_steps & fd_nrf8001_wakeup_step) {
            fd_nrf8001_wakeup();
            fd_bluetooth_step_complete(fd_nrf8001_wakeup_step);
        } else
        if (fd_bluetooth_system_steps & fd_nrf8001_setup_continue_step) {
            fd_nrf8001_setup_continue();
        } else
        if (fd_bluetooth_system_steps & fd_nrf8001_connect_step) {
            uint16_t timeout = 0; // infinite advertisement - no timeout
            uint16_t interval = 32; // 20ms (0.625ms units)
            fd_nrf8001_connect(timeout, interval);
            fd_bluetooth_step_complete(fd_nrf8001_connect_step);
        } else
        if (fd_bluetooth_system_steps & fd_nrf8001_change_timing_request_step) {
            uint16_t interval_min = 16; // 20ms (1.25ms units)
            uint16_t interval_max = 32; // 40ms (1.25ms units)
            uint16_t latency = 0;
            uint16_t timeout = 600; // 6s (10ms units)
            fd_nrf8001_change_timing_request(interval_min, interval_max, latency, timeout);
            fd_bluetooth_step_complete(fd_nrf8001_change_timing_request_step);
        } else
        if (fd_bluetooth_system_steps & fd_nrf8001_open_remote_pipe_step) {
            for (int i = 1; i < 63; ++i) {
                uint64_t mask = 1 << i;
                if (fd_bluetooth_pipes_closed & mask) {
                    fd_nrf8001_open_remote_pipe(i);
                    fd_bluetooth_pipes_closed &= ~mask;
                    break;
                }
            }
            if (fd_bluetooth_pipes_closed == 0) {
                fd_bluetooth_step_complete(fd_nrf8001_open_remote_pipe_step);
            }
        }
    }
}

void fd_bluetooth_data_step(void) {
    while ((fd_bluetooth_data_steps != 0) && fd_nrf8001_has_data_credits()) {
        if (fd_bluetooth_data_steps & fd_nrf8001_detour_send_data_ack_step) {
            fd_nrf8001_send_data_ack(PIPE_FIREFLY_ICE_DETOUR_RX_ACK);
            fd_bluetooth_data_step_complete(fd_nrf8001_detour_send_data_ack_step);
        }
    }
}

bool fd_bluetooth_is_pipe_open(uint32_t service_pipe_number) {
    return fd_bluetooth_pipes_open & (1 << service_pipe_number) ? true : false;
}

void fd_bluetooth_transfer(void) {
    if (fd_nrf8001_has_data_credits() && fd_bluetooth_is_pipe_open(PIPE_FIREFLY_ICE_DETOUR_TX)) {
        fd_detour_source_t *source = fd_bluetooth_detour_source_collection.first;
        if (source) {
            if (fd_detour_source_get(source, fd_bluetooth_out_data, MAX_CHARACTERISTIC_SIZE)) {
                fd_nrf8001_send_data(PIPE_FIREFLY_ICE_DETOUR_TX, fd_bluetooth_out_data, MAX_CHARACTERISTIC_SIZE);
                if (source->state != fd_detour_state_intermediate) {
                    fd_detour_source_collection_pop(&fd_bluetooth_detour_source_collection);
                }
            }
        }
    }
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
            fd_bluetooth_step_queue(fd_nrf8001_connect_step);
        } break;
        case OperatingModeSetup: {
            fd_nrf8001_set_system_credits(1);
            fd_bluetooth_setup_index = 0;
            fd_bluetooth_step_queue(fd_nrf8001_setup_continue_step);
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
    fd_nrf8001_did_setup = true;

    fd_bluetooth_step_complete(fd_nrf8001_setup_continue_step);
}

void fd_nrf8001_connected_event(
    uint8_t address_type __attribute__((unused)),
    uint8_t *peer_address __attribute__((unused)),
    uint16_t connection_interval __attribute__((unused)),
    uint16_t slave_latency __attribute__((unused)),
    uint16_t supervision_timeout __attribute__((unused)),
    uint8_t masterClockAccuracy __attribute__((unused))
) {
    fd_nrf8001_did_connect = true;

    fd_nrf8001_set_data_credits(fd_bluetooth_initial_data_credits);
    fd_detour_clear(&fd_bluetooth_detour);

    fd_bluetooth_step_queue(fd_nrf8001_change_timing_request_step);
}

void fd_nrf8001_disconnected_event(
    uint8_t aci_status __attribute__((unused)),
    uint8_t btle_status __attribute__((unused))
) {
    fd_bluetooth_system_steps = 0;
    fd_bluetooth_data_steps = 0;
    fd_bluetooth_pipes_open = 0;
    fd_bluetooth_pipes_closed = 0;
    fd_nrf8001_did_connect = false;
    fd_nrf8001_did_open_pipes = false;
    fd_nrf8001_did_receive_data = false;

    fd_nrf8001_set_data_credits(0);

    fd_bluetooth_step_queue(fd_nrf8001_connect_step);
}

void fd_nrf8001_data_credit_event(uint8_t data_credits) {
    fd_nrf8001_add_data_credits(data_credits);
}

void fd_nrf8001_pipe_status_event(uint64_t pipes_open, uint64_t pipes_closed) {
    fd_bluetooth_pipes_open = pipes_open;
    fd_bluetooth_pipes_closed = pipes_closed;
    if (fd_bluetooth_pipes_closed) {
        fd_bluetooth_step_queue(fd_nrf8001_open_remote_pipe_step);
    } else {
        fd_nrf8001_did_open_pipes = true;
    }
}

void fd_nrf8001_detour_data_received(
    uint8_t *data,
    uint32_t data_length
) {
    fd_detour_event(&fd_bluetooth_detour, data, data_length);
    switch (fd_detour_state(&fd_bluetooth_detour)) {
        case fd_detour_state_clear:
        case fd_detour_state_intermediate:
        break;
        case fd_detour_state_success:
            fd_control_process(&fd_bluetooth_detour_source_collection, fd_bluetooth_detour.data, fd_bluetooth_detour.length);
            fd_detour_clear(&fd_bluetooth_detour);
        break;
        case fd_detour_state_error:
            fd_log("");
            fd_detour_clear(&fd_bluetooth_detour);
        break;
    }
    fd_bluetooth_data_step_queue(fd_nrf8001_detour_send_data_ack_step);
}

void fd_nrf8001_data_received_event(
    uint8_t service_pipe_number,
    uint8_t *data,
    uint32_t data_length
) {
    fd_nrf8001_did_receive_data = true;

    if (service_pipe_number == PIPE_FIREFLY_ICE_DETOUR_RX_ACK) {
        fd_nrf8001_detour_data_received(data, data_length);
        return;
    }
}
