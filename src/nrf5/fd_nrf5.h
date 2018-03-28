#ifndef FD_NRF5_H
#define FD_NRF5_H

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wold-style-declaration"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include "nrf_soc.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"

// !!! missing from nRF SDK 15 -denis
#ifndef TWIM_SHORTS_LASTRX_SUSPEND_Msk
/* Bit 8 : Shortcut between LASTTX event and SUSPEND task */
#define TWIM_SHORTS_LASTRX_SUSPEND_Pos (8UL) /*!< Position of LASTRX_SUSPEND field. */
#define TWIM_SHORTS_LASTRX_SUSPEND_Msk (0x1UL << TWIM_SHORTS_LASTRX_SUSPEND_Pos) /*!< Bit mask of LASTRX_SUSPEND field. */
#define TWIM_SHORTS_LASTRX_SUSPEND_Disabled (0UL) /*!< Disable shortcut */
#define TWIM_SHORTS_LASTRX_SUSPEND_Enabled (1UL) /*!< Enable shortcut */
#endif

#pragma GCC diagnostic pop

#endif
