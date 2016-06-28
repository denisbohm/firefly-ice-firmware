#include "fdi_usb.h"

#include <usb_dcd_int.h>
#include <usb_regs.h>
#include <usbd_conf.h>
#include <usbd_core.h>
#include <usbd_desc.h>
#include <usbd_hid_core.h>
#include <usbd_req.h>
#include <usbd_usr.h>
#include <usbd_ioreq.h>

#include "usbd_desc.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    fdi_usb_status_reset,
    fdi_usb_status_configured,
    fdi_usb_status_suspended,
    fdi_usb_status_resumed,
    fdi_usb_status_connected,
    fdi_usb_status_disconnected,
} fdi_usb_status_t;

typedef void (*fdi_usb_state_change_callback_t)(fdi_usb_status_t oldState, fdi_usb_status_t newState);

static bool fdi_usb_initialized = false;

fdi_usb_state_change_callback_t fdi_usb_state_change_callback;
fdi_usb_status_t fdi_usb_status;

__ALIGN_BEGIN USB_OTG_CORE_HANDLE USB_OTG_dev __ALIGN_END;

USBD_Usr_cb_TypeDef USR_cb = {
    USBD_USR_Init,
    USBD_USR_DeviceReset,
    USBD_USR_DeviceConfigured,
    USBD_USR_DeviceSuspended,
    USBD_USR_DeviceResumed,
    // Can only get connected and disconnected events when VBUS sensing is present
    USBD_USR_DeviceConnected,
    USBD_USR_DeviceDisconnected,
};

typedef uint8_t (*USBD_HID_Init)(void *pdev, uint8_t cfgidx);

USBD_HID_Init fdi_usbd_hid_init;

__ALIGN_BEGIN uint8_t fdi_usbd_hid_rx_buffer[HID_OUT_PACKET] __ALIGN_END;

fdi_usb_data_callback_t fdi_usb_data_callback;

fdi_usb_tx_ready_callback_t fdi_usb_tx_ready_callback;

void fdi_usb_initialize(void) {
    fdi_usb_state_change_callback = 0;
    fdi_usb_data_callback = 0;
    fdi_usb_tx_ready_callback = 0;
}

void fdi_usb_set_data_callback(fdi_usb_data_callback_t callback) {
    fdi_usb_data_callback = callback;
}

void fdi_usb_set_tx_ready_callback(fdi_usb_tx_ready_callback_t callback) {
    fdi_usb_tx_ready_callback = callback;
}

bool fdi_usb_is_powered_up(void) {
    return (RCC->AHB2ENR & RCC_AHB2Periph_OTG_FS) != 0;
}

void fdi_usb_send(uint8_t *buffer, size_t length) {
    USBD_HID_SendReport(&USB_OTG_dev, buffer, length);
}

void breakpoint(void) {
    static uint32_t count = 0;
    ++count;
}

static uint32_t tx_sent_count = 0;

uint8_t EP0_TxSent(void *pdev) {
    ++tx_sent_count;
    breakpoint();
}
   
static uint32_t rx_ready_count = 0;

uint8_t EP0_RxReady(void *pdev) {
    ++rx_ready_count;
    breakpoint();
}

static uint32_t data_in_count = 0;

uint8_t DataIn(void *pdev, uint8_t epnum) {
    ++data_in_count;
    breakpoint();

    if (fdi_usb_tx_ready_callback) {
        fdi_usb_tx_ready_callback();
    }
}

static uint32_t data_out_count = 0;

// called when data is received
uint8_t DataOut(void *pdev, uint8_t epnum) {
    ++data_out_count;
    breakpoint();

    if (fdi_usb_data_callback) {
        fdi_usb_data_callback(fdi_usbd_hid_rx_buffer, sizeof(fdi_usbd_hid_rx_buffer));
    }

    DCD_EP_PrepareRx(pdev, HID_OUT_EP, fdi_usbd_hid_rx_buffer, HID_OUT_PACKET);
}

uint8_t fdi_usbd_hid_init_and_prepare_rx(void *pdev, uint8_t cfgidx) {
    fdi_usbd_hid_init(pdev, cfgidx);
    DCD_EP_PrepareRx(pdev, HID_OUT_EP, fdi_usbd_hid_rx_buffer, HID_OUT_PACKET);
    return USBD_OK;
}

void fdi_usb_power_up(void) {
    RCC_AHB2PeriphResetCmd(RCC_AHB2Periph_OTG_FS, DISABLE); // Exit reset state for USB-OTG-FS block
    RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_OTG_FS, ENABLE); // Enable USB-OTG-FS clock

    fdi_usbd_hid_init = USBD_HID_cb.Init;
    USBD_HID_cb.Init = fdi_usbd_hid_init_and_prepare_rx;
    USBD_HID_cb.EP0_TxSent = EP0_TxSent;
    USBD_HID_cb.EP0_RxReady = EP0_RxReady;
    USBD_HID_cb.DataIn = DataIn;
    USBD_HID_cb.DataOut = DataOut;

    USBD_Init(
        &USB_OTG_dev,
#ifdef USE_USB_OTG_HS
        USB_OTG_HS_CORE_ID,
#else
        USB_OTG_FS_CORE_ID,
#endif
        &USR_desc,
        &USBD_HID_cb,
        &USR_cb
    );
    fdi_usb_initialized = true;
}

void fdi_usb_power_down(void) {
    if (!fdi_usb_initialized) {
        fdi_usb_power_up();
    }
    RCC_AHB2PeriphResetCmd(RCC_AHB2Periph_OTG_FS, ENABLE); // Enter reset state for USB-OTG-FS block
    RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_OTG_FS, DISABLE); // Disable USB-OTG-FS clock

    fdi_usb_status = fdi_usb_status_reset;
}

void fdi_usb_set_state_change_callback(fdi_usb_state_change_callback_t callback) {
    fdi_usb_state_change_callback = callback;
}

void fdi_usb_set_status(fdi_usb_status_t status) {
    fdi_usb_status_t old_status = fdi_usb_status;
    fdi_usb_status = status;
    if (fdi_usb_state_change_callback) {
        (*fdi_usb_state_change_callback)(old_status, status);
    }
}

fdi_usb_status_t fdi_usb_get_status(void) {
    return fdi_usb_status;
}

void USBD_USR_Init(void) {   
    fdi_usb_status = fdi_usb_status_reset;
}

void USBD_USR_DeviceReset(uint8_t speed __attribute__((unused))) {
}

void USBD_USR_DeviceConfigured(void) {
    fdi_usb_set_status(fdi_usb_status_configured);
}

void USBD_USR_DeviceSuspended(void) {
    fdi_usb_set_status(fdi_usb_status_suspended);
}

void USBD_USR_DeviceResumed(void) {
    fdi_usb_set_status(fdi_usb_status_resumed);
}

void USBD_USR_DeviceConnected(void) {
    fdi_usb_set_status(fdi_usb_status_connected);
}

void USBD_USR_DeviceDisconnected(void) {
    fdi_usb_set_status(fdi_usb_status_disconnected);
}

void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE *pdev __attribute__((unused))) {
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);  
  
    /* Configure DM DP Pins */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);  
  
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource11,GPIO_AF_OTG1_FS);
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource12,GPIO_AF_OTG1_FS);
  
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_OTG_FS, ENABLE); 
}

void USB_OTG_BSP_uDelay(const uint32_t usec) {
    uint32_t count = 0;
    const uint32_t utime = 120 * usec / 7;
    do {
        if (++count > utime) {
            return;
        }
    } while (true);
}

void USB_OTG_BSP_mDelay(const uint32_t msec) {
    USB_OTG_BSP_uDelay(msec * 1000);   
}

void USB_OTG_BSP_EnableInterrupt(USB_OTG_CORE_HANDLE *pdev __attribute__((unused))) {
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    NVIC_InitTypeDef NVIC_InitStructure; 
    NVIC_InitStructure.NVIC_IRQChannel = OTG_FS_IRQn;  
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);  
}

void OTG_FS_IRQHandler(void) {
    USBD_OTG_ISR_Handler(&USB_OTG_dev);
}

void OTG_FS_WKUP_IRQHandler(void) {
    if (USB_OTG_dev.cfg.low_power) {
        *(uint32_t *)(0xE000ED10) &= 0xFFFFFFF9;
        SystemInit();
        USB_OTG_UngateClock(&USB_OTG_dev);
    }
    EXTI_ClearITPendingBit(EXTI_Line18);
}