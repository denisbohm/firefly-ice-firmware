#include "fd_perform_tek.h"

#include "fd_delay.h"
#include "fd_gpio.h"
#include "fd_i2cm.h"
#include "fd_log.h"

#include <stdio.h>
#include <string.h>

#define ADDRESS 0x88

#define PACKET 0x44

fd_gpio_t fd_perform_tek_pin_hr_wake;
fd_i2cm_bus_t *fd_perform_tek_i2c_bus;
fd_i2cm_device_t *fd_perform_tek_i2c_device;
fd_perform_tek_user_t fd_perform_tek_user;
uint16_t fd_perform_tek_activity_mode;
uint16_t fd_perform_tek_version;
uint16_t fd_perform_tek_post_results;

static
void fd_perform_tek_check_ack(void) {
    fd_log_assert(fd_i2cm_bus_is_enabled(fd_perform_tek_i2c_bus));
    uint8_t response[4];
    do {
        bool result = fd_i2cm_device_rxn(fd_perform_tek_i2c_device, response, sizeof(response));
        fd_log_assert(result);
    } while (response[0] == 0xff);
    fd_log_assert(response[0] == PACKET);
    fd_log_assert(response[1] == sizeof(response) - 2);
    fd_log_assert(response[2] == FD_PERFORM_TEK_COMMAND_Ack);
    fd_log_assert(response[3] == FD_PERFORM_TEK_ACK_ACK);
}

static
bool fd_perform_tek_command(uint8_t command) {
    fd_i2cm_bus_enable(fd_perform_tek_i2c_bus);
    uint8_t packet[] = {PACKET, 1, command};
    bool result = fd_i2cm_device_txn(fd_perform_tek_i2c_device, packet, sizeof(packet));
    if (result) {
        fd_perform_tek_check_ack();
    }
    fd_i2cm_bus_disable(fd_perform_tek_i2c_bus);
    return result;
}

bool fd_perform_tek_start(uint16_t configuration) {
    uint8_t parameters[] = {
        FD_PERFORM_TEK_SET_Age, // in Months, Range: [60-1440] Default: {360} = 30yrs
        FD_PERFORM_TEK_SET_Gender, // 0(Female) – 1(Male), Default: {1}
        FD_PERFORM_TEK_SET_Weight, // in 1/10 kg, Range: [100-5000] Default: {816} = 81.6kg
        FD_PERFORM_TEK_SET_Height, // in cm, Range: [60-250] Default: {180} = 5’11”
        FD_PERFORM_TEK_SET_ActivityMode,
        FD_PERFORM_TEK_SET_AlgorithmConfig,
    };
    uint16_t months = (uint16_t)(fd_perform_tek_user.age * 12.0f);
    uint16_t values[sizeof(parameters)] = {
        months,
        (uint16_t)fd_perform_tek_user.gender,
        (uint16_t)(fd_perform_tek_user.weight * 10.0f),
        (uint16_t)(fd_perform_tek_user.height * 100.0f),
        fd_perform_tek_activity_mode,
        configuration,
    };
    return fd_perform_tek_set(sizeof(parameters), parameters, values);
}

bool fd_perform_tek_start_with_recovery(uint16_t configuration) {
    bool result = fd_perform_tek_get_1(FD_PERFORM_TEK_GET_PostResults, &fd_perform_tek_post_results);
    if (result && ((fd_perform_tek_post_results & ~FD_PERFORM_TEK_POST_external_oscillator_not_detected) != 0)) {
        char buffer[32];
        sprintf(buffer, "HR POST %04x", fd_perform_tek_post_results);
        fd_log_assert_fail(buffer);
        
        // attempt to recover by doing a system reset -denis
        fd_perform_tek_software_reset();
        result = fd_perform_tek_start(configuration);
        if (!result) {
            fd_log_assert_fail("HR RESET START");
        } else {
            fd_perform_tek_get_1(FD_PERFORM_TEK_GET_PostResults, &fd_perform_tek_post_results);
            char buffer[32];
            sprintf(buffer, "HR RESET POST %04x", fd_perform_tek_post_results);
            fd_log_assert_fail(buffer);
            result = fd_perform_tek_start(configuration);
        }
    } else {
        result = fd_perform_tek_start(configuration);
    }
    return result;
}

bool fd_perform_tek_stop(void) {
    return fd_perform_tek_command(FD_PERFORM_TEK_COMMAND_Stop);
}

void fd_perform_tek_software_reset(void) {
    fd_i2cm_bus_enable(fd_perform_tek_i2c_bus);
    uint8_t packet[] = {PACKET, 4, FD_PERFORM_TEK_COMMAND_Set, 0x1A, 0x80, 0x00};
    bool result = fd_i2cm_device_txn(fd_perform_tek_i2c_device, packet, sizeof(packet));
    fd_log_assert(!result); // should fail because device resets before completing I2C transation

    // wait for post to go high indicating boot process is complete (250 ms max)
    fd_delay_ms(250);
}

bool fd_perform_tek_set(uint8_t count, uint8_t *parameters, uint16_t *values) {
    fd_i2cm_bus_enable(fd_perform_tek_i2c_bus);
    uint8_t packet[3 + 3 * 10] = {PACKET, 1 + count * 3, FD_PERFORM_TEK_COMMAND_Set};
    int index = 3;
    for (int i = 0; i < count; ++i) {
        packet[index++] = parameters[i];
        packet[index++] = values[i] >> 8;
        packet[index++] = values[i];
    }
    bool result = fd_i2cm_device_txn(fd_perform_tek_i2c_device, packet, 3 + 3 * count);
    if (result) {
        fd_perform_tek_check_ack();
    }
    fd_i2cm_bus_disable(fd_perform_tek_i2c_bus);
    return result;
}

bool fd_perform_tek_set_1(uint8_t parameter, uint16_t value) {
    return fd_perform_tek_set(1, &parameter, &value);
}

bool fd_perform_tek_get(uint8_t count, uint8_t *parameters, uint16_t* values) {
    memset(values, 0, count * sizeof(uint16_t));
    fd_i2cm_bus_enable(fd_perform_tek_i2c_bus);
    uint8_t packet[3 + 10] = {PACKET, 1 + count, FD_PERFORM_TEK_COMMAND_Get};
    memcpy(&packet[3], parameters, count);
    uint8_t response[4 + 10 * 3] = {0xff};
    bool result = fd_i2cm_device_txn(fd_perform_tek_i2c_device, packet, 3 + count);
    if (result) {
        do {
            result = fd_i2cm_device_rxn(fd_perform_tek_i2c_device, response, 4 + count * 3);
        } while (result && (response[0] == 0xff));
    }
    fd_i2cm_bus_disable(fd_perform_tek_i2c_bus);
    if (result) {
        fd_log_assert(result);
        fd_log_assert(response[0] == PACKET);
        fd_log_assert(response[1] == 1 + count * 3);
        fd_log_assert(response[2] == FD_PERFORM_TEK_COMMAND_Data);
        int index = 3;
        for (int i = 0; i < count; ++i) {
            uint8_t p __attribute((unused)) = response[index++];
            fd_log_assert(p == parameters[i]);
            uint8_t b1 = response[index++];
            uint8_t b0 = response[index++];
            values[i] = (b1 << 8) | b0;
        }
    }
    return result;
}

bool fd_perform_tek_get_1(uint8_t parameter, uint16_t *value) {
    return fd_perform_tek_get(1, &parameter, value);
}

void fd_perform_tek_initialize(fd_gpio_t pin_hr_wake, fd_i2cm_bus_t *i2c_bus, fd_i2cm_device_t *i2c_device) {
    fd_perform_tek_pin_hr_wake = pin_hr_wake;
    fd_perform_tek_i2c_bus = i2c_bus;
    fd_perform_tek_i2c_device = i2c_device;
    
    fd_perform_tek_user = (fd_perform_tek_user_t) {
        .height = 1.8f, // m
        .weight = 81.6f, // kg
        .age = 30.0f, // years
        .gender = fd_perform_tek_gender_male,
    };
    fd_perform_tek_activity_mode = 0x8000;
    fd_perform_tek_version = 0;
    fd_perform_tek_post_results = 0;

    fd_gpio_configure_output(fd_perform_tek_pin_hr_wake);
    fd_gpio_set(fd_perform_tek_pin_hr_wake, false);
    fd_delay_ms(1);
    fd_gpio_set(fd_perform_tek_pin_hr_wake, true);
    // wait for post to go high indicating boot process is complete (250 ms max)
    fd_delay_ms(250);
    fd_gpio_set(fd_perform_tek_pin_hr_wake, false);
 
    if (!fd_perform_tek_start_with_recovery(0x0101)) {
        return;
    }
    fd_perform_tek_get_1(FD_PERFORM_TEK_GET_FirmwareVerNum, &fd_perform_tek_version);
    fd_log_assert(fd_perform_tek_version >= 6032);
    fd_perform_tek_stop();
}

uint16_t fd_perform_tek_test(void) {
    fd_perform_tek_version = 0;
    fd_perform_tek_get_1(FD_PERFORM_TEK_GET_FirmwareVerNum, &fd_perform_tek_version);
    return fd_perform_tek_version;
}

