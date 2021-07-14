#include "fdi_usb.h"

#include "fd_log.h"

#include "fdi_stm32.h"

#include "usbd_def.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_hid.h"

extern PCD_HandleTypeDef hpcd;

USBD_HandleTypeDef USBD_Device;
fdi_usb_data_callback_t fdi_usb_data_callback;
__ALIGN_BEGIN uint8_t fdi_usb_hid_rx_buffer[64] __ALIGN_END;

uint8_t fdi_usb_hid_data_out(struct _USBD_HandleTypeDef *pdev, uint8_t epnum __attribute__((unused))) {
    if (fdi_usb_data_callback) {
        fdi_usb_data_callback(fdi_usb_hid_rx_buffer, sizeof(fdi_usb_hid_rx_buffer));
    }

    USBD_LL_PrepareReceive(pdev, HID_EPOUT_ADDR, fdi_usb_hid_rx_buffer, sizeof(fdi_usb_hid_rx_buffer));
    return USBD_OK;
}

void fdi_usb_initialize(void) {
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWREx_EnableVddUSB();

    USBD_StatusTypeDef status = USBD_Init(&USBD_Device, &HID_Desc, 0);
    fd_log_assert(status == USBD_OK);
    status = USBD_RegisterClass(&USBD_Device, &USBD_HID);
    fd_log_assert(status == USBD_OK);
    status = USBD_Start(&USBD_Device);
    fd_log_assert(status == USBD_OK);

    USBD_HID.DataOut = fdi_usb_hid_data_out;
}

void fdi_usb_set_tx_ready_callback(fdi_usb_tx_ready_callback_t callback) {
}

void fdi_usb_set_data_callback(fdi_usb_data_callback_t callback) {
    fdi_usb_data_callback = callback;
}

void fdi_usb_power_up(void) {
}

bool fdi_usb_can_send(void) {
    if (USBD_Device.dev_state != USBD_STATE_CONFIGURED) {
        return false;
    }
    USBD_HID_HandleTypeDef *hhid = (USBD_HID_HandleTypeDef *)USBD_Device.pClassData;
    return hhid->state == HID_IDLE;
}

uint8_t fdi_usb_send_data[64];

void fdi_usb_send(uint8_t *buffer, size_t length) {
    memcpy(fdi_usb_send_data, buffer, length);
    memset(&fdi_usb_send_data[length], 0, 64 - length);
    USBD_HID_SendReport(&USBD_Device, fdi_usb_send_data, 64);
}

void OTG_FS_IRQHandler(void) {
    HAL_PCD_IRQHandler(&hpcd);
}

/*******************************************************************************
                       LL Driver Callbacks (PCD -> USB Device Library)
*******************************************************************************/

/**
  * @brief  SetupStage callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_SetupStage(hpcd->pData, (uint8_t *)hpcd->Setup);
}

/**
  * @brief  DataOut Stage callback.
  * @param  hpcd: PCD handle
  * @param  epnum: Endpoint Number
  * @retval None
  */
void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
  USBD_LL_DataOutStage(hpcd->pData, epnum, hpcd->OUT_ep[epnum].xfer_buff);
}

/**
  * @brief  DataIn Stage callback.
  * @param  hpcd: PCD handle
  * @param  epnum: Endpoint Number
  * @retval None
  */
void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
  USBD_LL_DataInStage(hpcd->pData, epnum, hpcd->IN_ep[epnum].xfer_buff);
}

/**
  * @brief  SOF callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_SOF(hpcd->pData);
}

/**
  * @brief  Reset callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
  /* Reset Device */
  USBD_LL_Reset(hpcd->pData);

  /* Set USB Current Speed */
  USBD_LL_SetSpeed(hpcd->pData, USBD_SPEED_FULL);
}

/**
  * @brief  Suspend callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{
  __HAL_PCD_GATE_PHYCLOCK(hpcd);
  USBD_LL_Suspend(hpcd->pData);

  /*Enter in STOP mode */
  if (hpcd->Init.low_power_enable)
  {
    /* Set SLEEPDEEP bit and SleepOnExit of Cortex System Control Register */
    SCB->SCR |= (uint32_t)((uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));
  }
}

/**
  * @brief  Resume callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
  __HAL_PCD_UNGATE_PHYCLOCK(hpcd);
  USBD_LL_Resume(hpcd->pData);
}

/**
  * @brief  ISOOUTIncomplete callback.
  * @param  hpcd: PCD handle
  * @param  epnum: Endpoint Number
  * @retval None
  */
void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
  USBD_LL_IsoOUTIncomplete(hpcd->pData, epnum);
}

/**
  * @brief  ISOINIncomplete callback.
  * @param  hpcd: PCD handle
  * @param  epnum: Endpoint Number
  * @retval None
  */
void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
  USBD_LL_IsoINIncomplete(hpcd->pData, epnum);
}

/**
  * @brief  ConnectCallback callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_DevConnected(hpcd->pData);
}

/**
  * @brief  Disconnect callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_DevDisconnected(hpcd->pData);
}
