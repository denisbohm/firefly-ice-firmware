#ifndef FD_NRF8001_TYPES_H
#define FD_NRF8001_TYPES_H

// System Commands
#define Test 0x01
#define Echo 0x02
#define DtmCommand 0x03
#define Sleep 0x04
#define Wakeup 0x05
#define Setup 0x06
#define ReadDynamicData 0x07
#define WriteDynamicData 0x08
#define GetDeviceVersion 0x09
#define GetDeviceAddress 0x0A
#define GetBatteryLevel 0x0B
#define GetTemperature 0x0C
#define RadioReset 0x0E
#define Connect 0x0F
#define Bond 0x10
#define Disconnect 0x11
#define SetTxPower 0x12
#define ChangeTimingRequest 0x13
#define OpenRemotePipe 0x14
#define SetApplicationLatency 0x19
#define SetKey 0x1A
#define OpenAdvPipe 0x1B
#define Broadcast 0x1C
#define BondSecRequest 0x1D
#define DirectedConnect 0x1E
#define CloseRemotePipe 0x1F

// Data Commands
#define SendData 0x15
#define RequestData 0x17
#define SetLocalData 0x0d
#define SendDataAck 0x16
#define SendDataNack 0x18

// System Events
#define DeviceStartedEvent 0x81
#define EchoEvent 0x82
#define HardwareErrorEvent 0x83
#define CommandResponseEvent 0x84
#define ConnectedEvent 0x85
#define DisconnectedEvent 0x86
#define BondStatusEvent 0x87
#define PipeStatusEvent 0x88
#define TimingEvent 0x89
#define DisplayKeyEvent 0x8e
#define KeyRequestEvent 0x8f

// Data Events
#define DataCreditEvent 0x8a
#define PipeErrorEvent 0x8d
#define DataReceivedEvent 0x8c
#define DataAckEvent 0x8b

// ACI Status Codes
#define ACI_STATUS_SUCCESS 0x00
#define ACI_STATUS_TRANSACTION_CONTINUE 0x01
#define ACI_STATUS_TRANSACTION_COMPLETE 0x02
#define ACI_STATUS_EXTENDED 0x03
#define ACI_STATUS_ERROR_UNKNOWN 0x80
#define ACI_STATUS_ERROR_INTERNAL 0x81
#define ACI_STATUS_ERROR_CMD_UNKNOWN 0x82
#define ACI_STATUS_ERROR_DEVICE_STATE_INVALID 0x83
#define ACI_STATUS_ERROR_INVALID_LENGTH 0x84
#define ACI_STATUS_ERROR_INVALID_PARAMETER 0x85
#define ACI_STATUS_ERROR_BUSY 0x86
#define ACI_STATUS_ERROR_INVALID_DATA 0x87
#define ACI_STATUS_ERROR_CRC_MISMATCH 0x88
#define ACI_STATUS_ERROR_UNSUPPORTED_SETUP_FORMAT 0x89
#define ACI_STATUS_ERROR_INVALID_SEQ_NO 0x8a
#define ACI_STATUS_ERROR_SETUP_LOCKED 0x8b
#define ACI_STATUS_ERROR_LOCK_FAILED 0x8c
#define ACI_STATUS_ERROR_BOND_REQUIRED 0x8d
#define ACI_STATUS_ERROR_REJECTED 0x8e
#define ACI_STATUS_ERROR_DATA_SIZE 0x8f
#define ACI_STATUS_ERROR_PIPE_INVALID 0x90
#define ACI_STATUS_ERROR_CREDIT_NOT_AVAILABLE 0x91
#define ACI_STATUS_ERROR_PEER_ATT_ERROR 0x92
#define ACI_STATUS_ERROR_ADVT_TIMEOUT 0x93
#define ACI_STATUS_ERROR_PEER_SMP_ERROR 0x94
#define ACI_STATUS_ERROR_PIPE_TYPE_INVALID 0x95
#define ACI_STATUS_ERROR_PIPE_STATE_INVALID 0x96
#define ACI_STATUS_ERROR_INVALID_KEY_SIZE 0x97
#define ACI_STATUS_ERROR_INVALID_KEY_DATA 0x98

// Bonding Status Codes
#define ACI_BOND_STATUS_SUCCESS 0x00
#define ACI_BOND_STATUS_FAILED 0x01
#define ACI_BOND_STATUS_FAILED_TIMED_OUT 0x02
#define ACI_BOND_STATUS_FAILED_PASSKEY_ENTRY_FAILED 0x81
#define ACI_BOND_STATUS_FAILED_OOB_UNAVAILABLE 0x82
#define ACI_BOND_STATUS_FAILED_AUTHENTICATION_REQ 0x83
#define ACI_BOND_STATUS_FAILED_CONFIRM_VALUE 0x84
#define ACI_BOND_STATUS_FAILED_PAIRING_UNSUPPORTED 0x85
#define ACI_BOND_STATUS_FAILED_ENCRYPTION_KEY_SIZE 0x86
#define ACI_BOND_STATUS_FAILED_SMP_CMD_UNSUPPORTED 0x87
#define ACI_BOND_STATUS_FAILED_UNSPECIFIED_REASON 0x88
#define ACI_BOND_STATUS_FAILED_REPEATED_ATTEMPTS 0x89
#define ACI_BOND_STATUS_FAILED_INVALID_PARAMETERS 0x8a

#define OperatingModeTest 0x01
#define OperatingModeSetup 0x02
#define OperatingModeStandby 0x03

#define TestFeatureEnableDTMOverUART 0x01
#define TestFeatureEnableDTMOverACI 0x02
#define TestFeatureExitTestMode 0xff

#define DTM_CMD_LE_RESET (0x00 << 14)
#define DTM_CMD_LE_RECEIVER_TEST (0x01 << 14)
#define DTM_CMD_LE_TRANSMITTER_TEST (0x02 << 14)
#define DTM_CMD_LE_TEST_END (0x03 << 14)

#endif