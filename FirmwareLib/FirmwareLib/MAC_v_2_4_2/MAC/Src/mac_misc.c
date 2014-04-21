/**
 * @file mac_misc.c
 *
 * @brief This file implements miscellaneous MAC sublayer components.
 *
 * $Id: mac_misc.c 19865 2010-01-13 10:20:18Z sschneid $
 *
 * @author    Atmel Corporation: http://www.atmel.com
 * @author    Support email: avr@atmel.com
 */
/*
 * Copyright (c) 2009, Atmel Corporation All rights reserved.
 *
 * Licensed under Atmel's Limited License Agreement --> EULA.txt
 */

/* === Includes ============================================================ */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "pal.h"
#include "return_val.h"
#include "bmm.h"
#include "qmm.h"
#include "tal.h"
#include "ieee_const.h"
#include "mac_msg_const.h"
#include "mac_api.h"
#include "mac_msg_types.h"
#include "mac_data_structures.h"
#if (MAC_INDIRECT_DATA_FFD == 1)
#include "indirect_data_structures.h"
#endif  /* (MAC_INDIRECT_DATA_FFD == 1) */
#include "stack_config.h"
#include "mac_internal.h"
#include "mac.h"
#include "mac_config.h"
#include "mac_build_config.h"

/* === Macros =============================================================== */


/* === Globals ============================================================= */


/* === Prototypes ========================================================== */

static void do_init_pib(void);
static void flush_queues(void);
static uint8_t mac_reset(uint8_t init_pib);
static void mac_soft_reset(uint8_t init_pib);
static void reset_globals(void);
static void send_reset_conf(uint8_t *m, uint8_t status);

/* === Implementation ====================================================== */

/*
 * @brief Initializes the MAC global variables
 */
static void reset_globals(void)
{
    mac_busy = false;
    mac_state = MAC_IDLE;
    mac_radio_sleep_state = RADIO_AWAKE;
    mac_scan_state = MAC_SCAN_IDLE;
    mac_sync_state = MAC_SYNC_NEVER;
    mac_poll_state = MAC_POLL_IDLE;
#ifdef BEACON_SUPPORT
    mac_final_cap_slot = FINAL_CAP_SLOT_DEFAULT;
    mac_bc_data_indicated = false;
#endif  /* BEACON_SUPPORT */
    mac_last_dsn = 0;
    mac_last_src_addr = 0;

#if (MAC_INDIRECT_DATA_FFD == 1)
    uint8_t index;

    /* Clear the indirect frame array */
    for (index = 0; index < INDIRECT_DATA_QUEUE_CAPACITY; index ++)
    {
        mac_indirect_array[index].active = false;
        mac_indirect_array[index].buf_ptr = NULL;
        mac_indirect_array[index].persistence_time = 0;
    }

    mac_indirect_flag = false;
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

    mac_rx_enabled = false;
}



/**
 * @brief Initializes the MAC sublayer
 *
 * @return MAC_SUCCESS  if TAL is intialized successfully else FAILURE
 */
retval_t mac_init(void)
{
    /* Initialize TAL */
    if (tal_init() != MAC_SUCCESS)
    {
        return FAILURE;
    }

    /* Calibrate MCU's RC oscillator */
    if (!pal_calibrate_rc_osc())
    {
        return FAILURE;
    }


    mac_soft_reset(true);

    /* Set radio to sleep if allowed */
    mac_sleep_trans();

    /* Initialize the queues */
    qmm_queue_init(&nhle_mac_q, NHLE_MAC_QUEUE_CAPACITY);
    qmm_queue_init(&tal_mac_q, TAL_MAC_QUEUE_CAPACITY);
#if (MAC_INDIRECT_DATA_FFD == 1)
    qmm_queue_init(&indirect_data_q, INDIRECT_DATA_QUEUE_CAPACITY);
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */
#if (MAC_START_REQUEST_CONFIRM == 1)
#ifdef BEACON_SUPPORT
    qmm_queue_init(&broadcast_q, BROADCAST_QUEUE_CAPACITY);
#endif  /* BEACON_SUPPORT */
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */
    return MAC_SUCCESS;
}



/**
 * @brief Resets the MAC helper variables and transition to idle state
 *
 * This function sets the MAC to idle state and resets
 * MAC helper variables
 */
void mac_idle_trans(void)
{
    if (RADIO_SLEEPING == mac_radio_sleep_state)
    {
        /* Wake up radio first */
        mac_trx_wakeup();
    }

    {
        uint16_t default_shortaddress = macShortAddress_def;
        uint16_t default_panid = macPANId_def;
#if (DEBUG > 1)
        retval_t set_status;
        set_status =
#endif
        set_tal_pib_internal(macShortAddress, (void *)&default_shortaddress);
#if (DEBUG > 1)
            ASSERT(MAC_SUCCESS == set_status);
            set_status =
#endif
                set_tal_pib_internal(macPANId, (void *)&default_panid);

#if (DEBUG > 1)
            ASSERT(MAC_SUCCESS == set_status);
#endif
    }

    mac_soft_reset(true);

    /* Set radio to sleep if allowed */
    mac_sleep_trans();

}



/*
 * @brief Initializes MAC PIBs
 *
 * This function initializes all MAC PIBs to their defaults as stated by
 * 802.15.4.
 */
static void do_init_pib(void)
{
#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1)
    mac_pib_macAssociatedPANCoord = macAssociatedPANCoord_def;
#endif /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */
#if ((MAC_INDIRECT_DATA_BASIC == 1) || defined(BEACON_SUPPORT))
    mac_pib_macMaxFrameTotalWaitTime = macMaxFrameTotalWaitTime_def;
#endif  /* ((MAC_INDIRECT_DATA_BASIC == 1) || defined(BEACON_SUPPORT)) */
    mac_pib_macResponseWaitTime = macResponseWaitTime_def;
    mac_pib_macSecurityEnabled = macSecurityEnabled_def;

#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)
    mac_pib_macAssociationPermit = macAssociationPermit_def;
#endif /* (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) */

#if (MAC_START_REQUEST_CONFIRM == 1)
    mac_pib_macBeaconPayloadLength = macBeaconPayloadLength_def;
    mac_pib_macBSN = (uint8_t)rand();
#endif  /* (MAC_START_REQUEST_CONFIRM == 1) */

#if (MAC_INDIRECT_DATA_FFD == 1)
    mac_pib_macTransactionPersistenceTime = macTransactionPersistenceTime_def;
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

    mac_pib_macAutoRequest = macAutoRequest_def;
    mac_pib_macBattLifeExtPeriods = macBattLifeExtPeriods_def;
    mac_pib_macCoordExtendedAddress = CLEAR_ADDR_64;
    mac_pib_macCoordShortAddress = macCoordShortAddress_def;
    mac_pib_macDSN = (uint8_t)rand();
    mac_pib_macRxOnWhenIdle = macRxOnWhenIdle_def;
#ifdef TEST_HARNESS
    mac_pib_privateIllegalFrameType = 1;
    mac_pib_privateNoDataAfterAssocReq = 0;
    mac_pib_privateTransactionOverflow = 0;
    mac_pib_privateVirtualPANs = 0;
#endif /* TEST_HARNESS */
}



/**
 * @brief Resets the MAC layer
 *
 * The MLME-RESET.request primitive allows the next higher layer to request
 * that the MLME performs a reset operation.
 *
 * @param m Pointer to the MLME_RESET.request given by the NHLE
 */
void mlme_reset_request(uint8_t *m)
{
    mlme_reset_req_t *reset;
    reset = (mlme_reset_req_t *)BMM_BUFFER_POINTER(((buffer_t *)m));

    if (RADIO_SLEEPING == mac_radio_sleep_state)
    {
        /* Wakeup the radio */
        mac_trx_wakeup();
    }

    /* Start MAC reset functionality */
    uint8_t status = mac_reset(reset->SetDefaultPIB);

    /* Set radio to sleep if allowed */
    mac_sleep_trans();

    /*
     * As this is a mlme_reset request, all the requests, data (whether direct
     * or indirect), incoming frames are removed from the queues
     */
    flush_queues();

    send_reset_conf(m, status);
} /* mlme_reset_request() */



/*
 * @brief Internal MAC reset function
 *
 * This function resets the MAC variables, stops all running timers and
 * initializes the PIBs.
 *
 * @param init_pib Boolean indicates whether PIB attributes shall be
 * initialized or not.
 *
 * @return Success or failure status
 */
static uint8_t mac_reset(uint8_t init_pib)
{
    uint8_t status = MAC_DISABLE_TRX_FAILURE;

    /*
     * Transceiver is going to stop giving out clock for some time
     * so change to internal clock
     */
    pal_timer_source_select(TMR_CLK_SRC_DURING_TRX_SLEEP);

    /* Reset TAL */
    status = tal_reset(init_pib);

    /* Transceiver is now giving out clock, switch back to external clock */
    pal_timer_source_select(TMR_CLK_SRC_DURING_TRX_AWAKE);


    mac_soft_reset(init_pib);

    return status;
}



/*
 * @brief Internal MAC soft reset function
 *
 * This function resets the MAC variables, stops all running timers and
 * initializes the PIBs.
 *
 * @param init_pib Boolean indicates whether PIB attributes shall be
 * initialized or not.
 */
static void mac_soft_reset(uint8_t init_pib)
{
    reset_globals();

    /* Set trx to PHY_TRX_OFF */
    tal_rx_enable(PHY_TRX_OFF);

#if (MAC_LAST_TIMER_ID > 0)
    ENTER_CRITICAL_REGION();
    /* In case we have timers in MAC, all regular MAC timers are stopped. */
    for (uint8_t index = MAC_FIRST_TIMER_ID; index <= MAC_LAST_TIMER_ID; index++)
    {
        pal_timer_stop(index);
    }
    LEAVE_CRITICAL_REGION();
#endif

    if (init_pib)
    {
        do_init_pib();
    }
}



/**
 * @brief Puts the radio to sleep if this is allowed
 */
void mac_sleep_trans(void)
{
    /* Go to sleep? */
    if ((!mac_pib_macRxOnWhenIdle) && (!mac_rx_enabled))
    {
#if (MAC_SYNC_REQUEST == 1)
        /*
         * In case we are currently synced with our parent, and the
         * tracking beacon timer it NOT running (i.e. the timer is
         * expired and not started again yet), we need to stay awake,
         * until we receive a new beacon frame form our parent.
         */
        if ((MAC_SYNC_NEVER != mac_sync_state) && (!pal_is_timer_running(T_Beacon_Tracking_Period)))
        {
            // Stays awake
        }
        else
        {
            mac_trx_init_sleep();
        }
#else
        mac_trx_init_sleep();
#endif /* (MAC_SYNC_REQUEST == 1) */
    }
}



/*
 * @brief Flushes all queues
 */
static void flush_queues(void)
{
    /* Flush NHLE MAC queue */
    qmm_queue_flush(&nhle_mac_q);

    /* Flush TAL_MAC queue */
    qmm_queue_flush(&tal_mac_q);

#if (HIGHEST_STACK_LAYER == MAC)
    /* Flush MAC-NHLE queue */
    qmm_queue_flush(&mac_nhle_q);
#endif

#if (MAC_INDIRECT_DATA_FFD == 1)
    /* Flush MAC indirect queue */
    qmm_queue_flush(&indirect_data_q);
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

#if (MAC_START_REQUEST_CONFIRM == 1)
#ifdef BEACON_SUPPORT
    /* Flush MAC broadcast queue */
    qmm_queue_flush(&broadcast_q);
#endif  /* BEACON_SUPPORT */
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */
}



/*
 * @brief Sends mlme reset confirm
 *
 * @param m Buffer for reset confirm
 * @param status Status of MAC reset operation
 */
static void send_reset_conf(uint8_t *m, uint8_t status)
{
    mlme_reset_conf_t *mrc;

    mrc = (mlme_reset_conf_t *)BMM_BUFFER_POINTER(((buffer_t *)m));

    mrc->status = status;
    mrc->cmdcode = MLME_RESET_CONFIRM;

    /* Append the mlme reset confirm to the MAC-NHLE queue */
    qmm_queue_append(&mac_nhle_q, (buffer_t *)m);
}



#if (MAC_COMM_STATUS_INDICATION == 1)
/**
 * @brief Creates a Communication Status Indication message to the upper layer
 *
 * @param srcAddrMode Address mode of transmitting device
 * @param srcAddr Address of transmitting device
 * @param dstAddrMode Address mode of receiving device
 * @param dstAddr Address of receiving device
 * @param status Status of the last operation
 * @param buf Buffer for Communication Status Indication to the NHLE
 */
void mac_mlme_comm_status(uint8_t srcAddrMode,
                          uint64_t *srcAddr,
                          uint8_t dstAddrMode,
                          uint64_t *dstAddr,
                          uint8_t status,
                          buffer_t *buf)
{
    /*
     * The pointer to the destination address (received as one of the function
     * paramters) points to the location in the buffer, in which the indirect is
     * present. As the same buffer is used to generate the comm status
     * indication, it is typecasted to the 'mlme_comm_status_ind_t'. This may
     * result in loosing destination address (which is still a part of this
     * buffer), hence the destination address is backed up in a stack variable.
     */
    uint64_t destination_address = *dstAddr;

    mlme_comm_status_ind_t *csi =
        (mlme_comm_status_ind_t*)BMM_BUFFER_POINTER(buf);

    csi->cmdcode = MLME_COMM_STATUS_INDICATION;

    csi->PANId = tal_pib_PANId;

    csi->SrcAddrMode = srcAddrMode;

    /* Initialize the source address */
    csi->SrcAddr = *srcAddr;

    csi->DstAddrMode = dstAddrMode;

    /* Initialize the destination address */
    csi->DstAddr = destination_address;

    csi->status = status;

    qmm_queue_append(&mac_nhle_q, buf);
}
#endif /* MAC_COMM_STATUS_INDICATION */



/**
 * @brief MAC function to put the radio to sleep mode
 */
void mac_trx_init_sleep(void)
{
    /* If the radio is not sleeping, it is put to sleep */
    if (RADIO_AWAKE == mac_radio_sleep_state)
    {
        pal_timer_source_select(TMR_CLK_SRC_DURING_TRX_SLEEP);

        if (MAC_SUCCESS == tal_trx_sleep(SLEEP_MODE_1))
        {
            mac_radio_sleep_state = RADIO_SLEEPING;
        }
        else
        {
            pal_timer_source_select(TMR_CLK_SRC_DURING_TRX_AWAKE);
        }
    }
}



/**
 * @brief MAC function to wake-up the radio from sleep state
 */
void mac_trx_wakeup(void)
{
     /* If the radio is sleeping, it is woken-up */
    if (RADIO_SLEEPING == mac_radio_sleep_state)
    {
        if (FAILURE != tal_trx_wakeup())
        {
            pal_timer_source_select(TMR_CLK_SRC_DURING_TRX_AWAKE);

            mac_radio_sleep_state = RADIO_AWAKE;
        }
    }
}



#if (MAC_INDIRECT_DATA_FFD == 1)
/**
 * @brief Adds persistence time for a new indirect data
 *
 * For a new indirect data, this function creates a entry in the perisistence_t
 * structure, updates the persistance time, blocks the index of the structure
 * for the indirect data and updates the buffer pointer to the indirect data.
 *
 * @param buffer Pointer to indirect data
 */
void add_persistence_time(uint8_t *buffer)
{
    uint8_t index;

    for (index = 0; index < INDIRECT_DATA_QUEUE_CAPACITY; index++)
    {
        if (!mac_indirect_array[index].active)
        {
            /*
             * Add the persistence time for the data which is added
             * to the indirect queue
             */
            mac_indirect_array[index].persistence_time =
                mac_pib_macTransactionPersistenceTime;

            /*
             * Store the corresponding address of the buffer which is appended
             * into indirect queue
             */
            mac_indirect_array[index].buf_ptr = buffer;
            mac_indirect_array[index].active = true;
            break;
        }
    }
}
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */



#if (MAC_INDIRECT_DATA_FFD == 1)
/**
 * @brief Rearranges the indirect data persistence array
 *
 * This function rearranges the indirect data persistence array to have
 * all active items together.
 */
void rearrange_persistence_array()
{
    uint8_t index = 0;
    uint8_t first_inactive_item;

    /* Loop until the first inactive item is found */
    while (mac_indirect_array[index].active)
    {
        index++;
    }

    first_inactive_item = index;
    index++;

    while (index < INDIRECT_DATA_QUEUE_CAPACITY)
    {
        if (mac_indirect_array[index].active)
        {
            /*
             * Move the active element to the empty location
             * created due to purging
             */
            mac_indirect_array[first_inactive_item].active =
                mac_indirect_array[index].active;

            mac_indirect_array[first_inactive_item].buf_ptr =
                mac_indirect_array[index].buf_ptr;

            mac_indirect_array[first_inactive_item].persistence_time =
                mac_indirect_array[index].persistence_time;

            /* Clear the element which was moved */
            mac_indirect_array[index].active = false;
            mac_indirect_array[index].buf_ptr = NULL;
            mac_indirect_array[index].persistence_time = 0;

            first_inactive_item++;
        }
        index++;
    }
}
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

/* EOF */
