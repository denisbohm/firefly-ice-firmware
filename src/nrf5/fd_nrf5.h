#ifndef FD_NRF5_H
#define FD_NRF5_H

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wold-style-declaration"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdm.h"
#include "nrf_soc.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_mbr.h"
#include "nrf_nvmc.h"
#include "nrf_temp.h"

#ifndef FD_NRF5_NO_CRYPTO
#include "crys_hash.h"
#include "ssi_aes.h"
#include "sns_silib.h"
#endif

#include "app_scheduler.h"

#pragma GCC diagnostic pop

#endif
