/**
 * @file tal_irq_handler.c
 *
 * @brief This file handles the interrupts generated by the transceiver.
 *
 * $Id: tal_irq_handler.c 19597 2009-12-18 15:53:11Z sschneid $
 *
 * @author    Atmel Corporation: http://www.atmel.com
 * @author    Support email: avr@atmel.com
 */
/*
 * Copyright (c) 2009, Atmel Corporation All rights reserved.
 *
 * Licensed under Atmel's Limited License Agreement --> EULA.txt
 */

/* === INCLUDES ============================================================ */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "pal.h"
#include "return_val.h"
#include "tal.h"
#include "ieee_const.h"
#include "stack_config.h"
#include "bmm.h"
#include "qmm.h"
#include "tal_irq_handler.h"
#include "tal_rx.h"
#include "at86rf212.h"
#include "tal_internal.h"
#include "tal_constants.h"
#include "tal_tx.h"
#include "mac_build_config.h"

/* === TYPES =============================================================== */


/* === MACROS ============================================================== */


/* === GLOBALS ============================================================= */


/* === PROTOTYPES ========================================================== */


/* === IMPLEMENTATION ====================================================== */

/**
 * @brief Transceiver interrupt handler
 *
 * This function handles the transceiver generated interrupts.
 */
void trx_irq_handler_cb(void)
{
    trx_irq_reason_t trx_irq_cause;

#ifdef DISABLE_TSTAMP_IRQ
    /*
     * Get timestamp.
     *
     * In case Antenna diversity is used or the utilization of
     * the Timestamp IRQ is disabled, the timestamp needs to be read now
     * the "old-fashioned" way.
     */
    pal_get_current_time(&tal_rx_timestamp);
#endif

    trx_irq_cause = (trx_irq_reason_t)pal_trx_reg_read(RG_IRQ_STATUS);

    if (trx_irq_cause & TRX_IRQ_TRX_END)
    {
        /*
         * TRX_END reason depends on if the trx is currently used for
         * transmission or reception.
         */
#if ((MAC_START_REQUEST_CONFIRM == 1) && (defined BEACON_SUPPORT))
        if ((tal_state == TAL_TX_AUTO) || tal_beacon_transmission)
#else
        if (tal_state == TAL_TX_AUTO)
#endif
        {
            /* Get the result and push it to the queue. */
            if (trx_irq_cause & TRX_IRQ_TRX_UR)
            {
                handle_tx_end_irq(true);            // see tal_tx.c
            }
            else
            {
                handle_tx_end_irq(false);            // see tal_tx.c
            }
        }
        else   /* Other tal_state than TAL_TX_... */
        {
            /* Handle rx interrupt. */
            handle_received_frame_irq();    // see tal_rx.c
        }
    }

#if (DEBUG > 0)
    /* Other IRQ than TRX_END */
    if (trx_irq_cause != TRX_IRQ_TRX_END)
    {
        /* PLL_LOCK interrupt migth be set, because poll mode is enabled. */
        /*
        if (trx_irq_cause & TRX_IRQ_PLL_LOCK)
        {
            ASSERT("unexpected IRQ: TRX_IRQ_PLL_LOCK" == 0);
        }
        */
        if (trx_irq_cause & TRX_IRQ_PLL_UNLOCK)
        {
            ASSERT("unexpected IRQ: TRX_IRQ_PLL_UNLOCK" == 0);
        }
        /* RX_START interrupt migth be set, because poll mode is enabled. */
        /*
        if (trx_irq_cause & TRX_IRQ_RX_START)
        {
            ASSERT("unexpected IRQ: TRX_IRQ_RX_START" == 0);
        }
        */
        if (trx_irq_cause & TRX_IRQ_CCA_ED_READY)
        {
            ASSERT("unexpected IRQ: TRX_IRQ_CCA_ED_READY" == 0);
        }
        /* AMI interrupt might set, because poll mode is enabled. */
        /*
        if (trx_irq_cause & TRX_IRQ_AMI)
        {
            ASSERT("unexpected IRQ: TRX_IRQ_AMI" == 0);
        }
        */
        if (trx_irq_cause & TRX_IRQ_TRX_UR)
        {
            ASSERT("unexpected IRQ: TRX_IRQ_TRX_UR" == 0);
        }
        if (trx_irq_cause & TRX_IRQ_BAT_LOW)
        {
            ASSERT("unexpected IRQ: TRX_IRQ_BAT_LOW" == 0);
        }
    }
#endif

}/* trx_irq_handler_cb() */

#ifndef DISABLE_TSTAMP_IRQ
/**
 * @brief Timestamp interrupt handler
 *
 * This function handles the interrupts handling the timestamp.
 * This function is only enabled, in case the Timestamp IRQ is
 * not disabled, which can be done explicitly or once Antenna diversity
 * is used.
 */
void trx_irq_timestamp_handler_cb(void)
{
    pal_trx_read_timestamp(&tal_rx_timestamp);
}
#endif


/* EOF */

