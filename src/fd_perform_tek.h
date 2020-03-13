#ifndef FD_PERFORM_TEK_H
#define FD_PERFORM_TEK_H

#include "fd_gpio.h"
#include "fd_i2cm.h"

#include <stdbool.h>
#include <stdint.h>

#define FD_PERFORM_TEK_COMMAND_Start   0x01
#define FD_PERFORM_TEK_COMMAND_Stop    0x02
#define FD_PERFORM_TEK_COMMAND_Set     0x04
#define FD_PERFORM_TEK_COMMAND_Get     0x08
#define FD_PERFORM_TEK_COMMAND_Ack     0x10
#define FD_PERFORM_TEK_COMMAND_Data    0x20
#define FD_PERFORM_TEK_COMMAND_Wake    0x40
#define FD_PERFORM_TEK_COMMAND_State   0x80
#define FD_PERFORM_TEK_COMMAND_Version 0xfe
#define FD_PERFORM_TEK_COMMAND_Reset   0xff

#define FD_PERFORM_TEK_SET_SoftwareConfig    0x03
#define FD_PERFORM_TEK_SET_HeartRateRange    0x14
#define FD_PERFORM_TEK_SET_Age               0x16
#define FD_PERFORM_TEK_SET_Gender            0x17
#define FD_PERFORM_TEK_SET_Weight            0x18
#define FD_PERFORM_TEK_SET_Height            0x19
#define FD_PERFORM_TEK_SET_ClearValues       0x1a
#define FD_PERFORM_TEK_SET_ActivityMode      0x1b
#define FD_PERFORM_TEK_SET_WalkCal           0x1c
#define FD_PERFORM_TEK_SET_RunCal            0x1e
#define FD_PERFORM_TEK_SET_HeartRateAlgoCfg  0x2b
#define FD_PERFORM_TEK_SET_AlgorithmConfig   0x2c
#define FD_PERFORM_TEK_SET_DataStreamKey     0x61 // through 64
#define FD_PERFORM_TEK_SET_DataStreamControl 0x65
#define FD_PERFORM_TEK_SET_FirmwareUpdate    0x80

#define FD_PERFORM_TEK_GET_DataAcqState 0x10
#define FD_PERFORM_TEK_GET_SignalFlagAndQuality 0x11
#define FD_PERFORM_TEK_GET_OpticalInputDCLevel 0x12
#define FD_PERFORM_TEK_GET_PostResults 0x13
#define FD_PERFORM_TEK_GET_BatteryVoltage 0x14
#define FD_PERFORM_TEK_GET_HeartRate 0x20
#define FD_PERFORM_TEK_GET_HeartRateAvg 0x21
#define FD_PERFORM_TEK_GET_HeartRateMin 0x22
#define FD_PERFORM_TEK_GET_HeartRateMax 0x23
#define FD_PERFORM_TEK_GET_StepCount 0x2a
#define FD_PERFORM_TEK_GET_StepRate 0x30
#define FD_PERFORM_TEK_GET_Distance 0x31
#define FD_PERFORM_TEK_GET_Timer 0x32
#define FD_PERFORM_TEK_GET_TotalSteps 0x33
#define FD_PERFORM_TEK_GET_Speed 0x34
#define FD_PERFORM_TEK_GET_RriStatus 0x35
#define FD_PERFORM_TEK_GET_RriTimestamp 0x36
#define FD_PERFORM_TEK_GET_RriDataRegister1 0x37
#define FD_PERFORM_TEK_GET_RriDataRegister2 0x38
#define FD_PERFORM_TEK_GET_RriDataRegister3 0x39
#define FD_PERFORM_TEK_GET_RriDataRegister4 0x3a
#define FD_PERFORM_TEK_GET_RriDataRegister5 0x3b
#define FD_PERFORM_TEK_GET_VO2 0x40
#define FD_PERFORM_TEK_GET_CALR 0x41
#define FD_PERFORM_TEK_GET_CALS 0x42
#define FD_PERFORM_TEK_GET_MAXVO2 0x43
#define FD_PERFORM_TEK_GET_FirmwareVerNum 0x44
#define FD_PERFORM_TEK_GET_ProcPartID 0x47
#define FD_PERFORM_TEK_GET_ProcSerialID_U 0x48
#define FD_PERFORM_TEK_GET_ProcSerialID_L 0x49
#define FD_PERFORM_TEK_GET_DetectorPartID 0x4a
#define FD_PERFORM_TEK_GET_DetectorSerialID_U 0x4b
#define FD_PERFORM_TEK_GET_DetectorSerialID_L 0x4c
#define FD_PERFORM_TEK_GET_AccelPartID 0x4d
#define FD_PERFORM_TEK_GET_AccelSerialID_U 0x4e
#define FD_PERFORM_TEK_GET_AccelSerialID_L 0x4f

#define FD_PERFORM_TEK_ACK_ACK 0x00
#define FD_PERFORM_TEK_ACK_NACK 0x01
#define FD_PERFORM_TEK_ACK_MEH 0x02
#define FD_PERFORM_TEK_ACK_NOGO 0x03
#define FD_PERFORM_TEK_ACK_UNK 0x04
#define FD_PERFORM_TEK_ACK_NOSTOP 0x05

#define FD_PERFORM_TEK_VALUE_ProcStateStop 0x02
#define FD_PERFORM_TEK_VALUE_ProcStateStart 0x04

#define FD_PERFORM_TEK_POST_system_failure 0b0001
#define FD_PERFORM_TEK_POST_detector_failure 0b0010
#define FD_PERFORM_TEK_POST_accelerometer_failure 0b0100
#define FD_PERFORM_TEK_POST_external_oscillator_not_detected 0b1000

typedef enum {
    fd_perform_tek_gender_female,
    fd_perform_tek_gender_male,
} fd_perform_tek_gender_t;

typedef struct {
    float height; // m
    float weight; // kg
    float age; // years
    fd_perform_tek_gender_t gender;
} fd_perform_tek_user_t;

extern uint16_t fd_perform_tek_activity_mode;

void fd_perform_tek_initialize(fd_gpio_t pin_hr_wake, fd_i2cm_bus_t *i2c_bus, fd_i2cm_device_t *i2c_device);

bool fd_perform_tek_start(uint16_t configuration);
bool fd_perform_tek_start_with_recovery(uint16_t configuration);
bool fd_perform_tek_stop(void);
void fd_perform_tek_software_reset(void);
bool fd_perform_tek_set(uint8_t count, uint8_t *parameters, uint16_t *values);
bool fd_perform_tek_set_1(uint8_t parameter, uint16_t value);
bool fd_perform_tek_get(uint8_t count, uint8_t *parameters, uint16_t* values);
bool fd_perform_tek_get_1(uint8_t parameter, uint16_t *value);

uint16_t fd_perform_tek_test(void);

#endif
