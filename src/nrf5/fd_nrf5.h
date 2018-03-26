#ifndef FD_NRF5_H
#define FD_NRF5_H

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wold-style-declaration"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include "nrf_soc.h"
#include "nrf_delay.h"
#include "nrf_drv_twi.h"
#include "nrf_gpio.h"
#include "nrf_gpiote.h"
#include "nrf_mbr.h"
#include "nrf_nvmc.h"
#include "nrf_sdh.h"
#include "nrf_sdm.h"
#include "nrf_soc.h"
#include "nrf_temp.h"

#include "app_error.h"
#include "app_gpiote.h"
#include "app_scheduler.h"
#include "app_timer.h"
#include "app_util.h"
#include "app_util_platform.h"

#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_bas.h"
#include "ble_conn_params.h"
#include "ble_dis.h"
#include "ble_dtm.h"

#pragma GCC diagnostic pop

#endif
