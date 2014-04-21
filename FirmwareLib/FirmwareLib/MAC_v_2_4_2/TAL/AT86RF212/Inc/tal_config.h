/**
 * @file tal_config.h
 *
 * @brief File contains TAL configuration parameters.
 *
 * $Id: tal_config.h 16539 2009-07-13 08:11:09Z sschneid $
 *
 * @author    Atmel Corporation: http://www.atmel.com
 * @author    Support email: avr@atmel.com
 */
/*
 * Copyright (c) 2009, Atmel Corporation All rights reserved.
 *
 * Licensed under Atmel's Limited License Agreement --> EULA.txt
 */

/* Prevent double inclusion */
#ifndef TAL_CONFIG_H
#define TAL_CONFIG_H

/* === INCLUDES ============================================================ */

#include "at86rf212.h"

/* === EXTERNALS =========================================================== */

/* === MACROS ============================================================== */

#define TAL_RADIO_WAKEUP_TIME_SYM       (TAL_CONVERT_US_TO_SYMBOLS(SLEEP_TO_TRX_OFF_US))
#define TAL_FIRST_TIMER_ID              (0)

#ifdef ENABLE_FTN_PLL_CALIBRATION
/*
 * PLL calibration and filter tuning timer timeout in minutes
 */
#define TAL_CALIBRATION_TIMEOUT_MIN         (5UL)

/*
 * PLL calibration and filter tuning timer timeout in us,
 */
#define TAL_CALIBRATION_TIMEOUT_US          ((TAL_CALIBRATION_TIMEOUT_MIN) * (60UL) * (1000UL) * (1000UL))
#endif  /* ENABLE_FTN_PLL_CALIBRATION */

/* === TYPES =============================================================== */

/* Timer ID's used by TAL */
#ifdef BEACON_SUPPORT
// Beacon Support
    #ifdef ENABLE_FTN_PLL_CALIBRATION
        typedef enum tal_timer_id_tag
        {
            TAL_CSMA_CCA                    = (TAL_FIRST_TIMER_ID),
            TAL_CSMA_BEACON_LOSS_TIMER      = (TAL_FIRST_TIMER_ID + 1),
            TAL_CALIBRATION                 = (TAL_FIRST_TIMER_ID + 2)
        } tal_timer_id_t;

        #define NUMBER_OF_TAL_TIMERS        (3)
    #else
        typedef enum tal_timer_id_tag
        {
            TAL_CSMA_CCA                    = (TAL_FIRST_TIMER_ID),
            TAL_CSMA_BEACON_LOSS_TIMER      = (TAL_FIRST_TIMER_ID + 1)
        } tal_timer_id_t;

        #define NUMBER_OF_TAL_TIMERS        (2)
    #endif  /* ENABLE_FTN_PLL_CALIBRATION */
#else /* No BEACON_SUPPORT */
    #ifdef ENABLE_FTN_PLL_CALIBRATION
        typedef enum tal_timer_id_tag
        {
            TAL_CALIBRATION                 = (TAL_FIRST_TIMER_ID)
        } tal_timer_id_t;

        #define NUMBER_OF_TAL_TIMERS        (1)
    #else
        #define NUMBER_OF_TAL_TIMERS        (0)
    #endif  /* ENABLE_FTN_PLL_CALIBRATION */
#endif  /* BEACON_SUPPORT */

#if (NUMBER_OF_TAL_TIMERS > 0)
#define TAL_LAST_TIMER_ID    (TAL_FIRST_TIMER_ID + NUMBER_OF_TAL_TIMERS - 1) // -1: timer id starts with 0
#else
#define TAL_LAST_TIMER_ID    (TAL_FIRST_TIMER_ID)
#endif

#define TAL_INCOMING_FRAME_QUEUE_CAPACITY   (5)

/* === PROTOTYPES ========================================================== */


#endif /* TAL_CONFIG_H */
