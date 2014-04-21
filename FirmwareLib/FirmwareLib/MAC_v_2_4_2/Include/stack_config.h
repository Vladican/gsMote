/**
 * @file stack_config.h
 *
 * @brief Stack configuration parameters
 *
 * $Id: stack_config.h 18826 2009-10-23 12:40:41Z sschneid $
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
#ifndef STACK_CONFIG_H
#define STACK_CONFIG_H

/* === Includes ============================================================= */


/* === Macros =============================================================== */

/* Highest stack layer definitions */
#define PAL                                 (1)
#define TINY_TAL                            (2)
#define TAL                                 (3)
#define MAC                                 (4)
#define NWK                                 (5)

#if (HIGHEST_STACK_LAYER == NWK)
    /* Reduce the header file dependency by using hard-coded values */
    #ifdef FFD
        #define LARGE_BUFFER_SIZE           (166)
        #define SMALL_BUFFER_SIZE           (69)
    #else   /* RFD */
        #define LARGE_BUFFER_SIZE           (162)
        #define SMALL_BUFFER_SIZE           (65)
    #endif
#elif (HIGHEST_STACK_LAYER == PAL)
    /* Reduce the header file dependency by using hard-coded values */
    #define LARGE_BUFFER_SIZE               (160)
    #define SMALL_BUFFER_SIZE               (68)
#else  /* !NWK && !PAL */
/**
 * The following macro holds the size of a large buffer.
 */
#ifdef FFD
#define LARGE_BUFFER_SIZE                   (((sizeof(indirect_data_t) + \
                                               sizeof(frame_info_t) + \
                                               aMaxPHYPacketSize) / 4 + 1) * 4)
#else   /* RFD */
#define LARGE_BUFFER_SIZE                   (((sizeof(frame_info_t) + \
                                               aMaxPHYPacketSize) / 4 + 1) * 4)
#endif  /* FFD / RFD */

/**
 * The following macro holds the size of a small buffer.
 */
#ifdef FFD
#define SMALL_BUFFER_SIZE                   (((sizeof(indirect_data_t) + \
                                               sizeof(frame_info_t) + \
                                               MAX_MGMT_FRAME_LENGTH) / 4 + 1) * 4)
#else   /* RFD */
#define SMALL_BUFFER_SIZE                   (((sizeof(frame_info_t) + \
                                               MAX_MGMT_FRAME_LENGTH) / 4 + 1) * 4)
#endif  /* FFD/RFD */
#endif  /* #if (HIGHEST_STACK_LAYER == NWK) */

/* Configuration if PAL is the highest stack layer */
#if (HIGHEST_STACK_LAYER == PAL)
#define NUMBER_OF_TOTAL_STACK_TIMERS        (0)
#define LAST_STACK_TIMER_ID                 (0)
#define NUMBER_OF_LARGE_STACK_BUFS          (0)
#define NUMBER_OF_SMALL_STACK_BUFS          (0)
#endif  /* (HIGHEST_STACK_LAYER == PAL) */

/* Configuration if TINY_TAL is the highest stack layer */
#if (HIGHEST_STACK_LAYER == TINY_TAL)
#include "tal_config.h"
#define NUMBER_OF_TOTAL_STACK_TIMERS        (NUMBER_OF_TAL_TIMERS)
#define LAST_STACK_TIMER_ID                 (TAL_LAST_TIMER_ID)
#define NUMBER_OF_LARGE_STACK_BUFS          (0)
#define NUMBER_OF_SMALL_STACK_BUFS          (0)
#endif  /* (HIGHEST_STACK_LAYER == TINY_TAL) */

/* Configuration if TAL is the highest stack layer */
#if (HIGHEST_STACK_LAYER == TAL)
#include "tal_config.h"
#define NUMBER_OF_TOTAL_STACK_TIMERS        (NUMBER_OF_TAL_TIMERS)
#define LAST_STACK_TIMER_ID                 (TAL_LAST_TIMER_ID)
#define NUMBER_OF_LARGE_STACK_BUFS          (4)
#define NUMBER_OF_SMALL_STACK_BUFS          (0)
#endif  /* (HIGHEST_STACK_LAYER == TAL) */

/* Configuration if MAC is the highest stack layer */
#if (HIGHEST_STACK_LAYER == MAC)
#include "mac_config.h"
#define NUMBER_OF_TOTAL_STACK_TIMERS        (NUMBER_OF_TAL_TIMERS + NUMBER_OF_MAC_TIMERS)
#define LAST_STACK_TIMER_ID                 (MAC_LAST_TIMER_ID)
#define NUMBER_OF_LARGE_STACK_BUFS          (6)
#define NUMBER_OF_SMALL_STACK_BUFS          (0)
#endif  /* (HIGHEST_STACK_LAYER == MAC) */

/* Configuration if NWK is the highest stack layer */
#if (HIGHEST_STACK_LAYER == NWK)
#include "nwk_config.h"
#define NUMBER_OF_TOTAL_STACK_TIMERS        (NUMBER_OF_TAL_TIMERS + NUMBER_OF_MAC_TIMERS + NUMBER_OF_NWK_TIMERS)
#define LAST_STACK_TIMER_ID                 (NWK_LAST_TIMER_ID)
#define NUMBER_OF_LARGE_STACK_BUFS          (5)
#define NUMBER_OF_SMALL_STACK_BUFS          (0)
#endif  /* (HIGHEST_STACK_LAYER == NWK) */

/* === Types ================================================================ */


/* === Externals ============================================================ */


/* === Prototypes =========================================================== */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* STACK_CONFIG_H */
/* EOF */
