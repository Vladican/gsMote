/**
 * @file mac_start.c
 *
 * @brief This file implements the MLME-START.request
 * (MAC layer management entity) entry points.
 *
 * $Id: mac_start.c 19526 2009-12-11 18:59:53Z sschneid $
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
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */
#include "stack_config.h"
#include "mac_internal.h"
#include "mac.h"
#include "mac_config.h"
#include "mac_build_config.h"

/* === Macros =============================================================== */


/* === Globals ============================================================== */

#if (MAC_START_REQUEST_CONFIRM == 1) || defined(DOXYGEN)

static mlme_start_req_t start_parms;    /* Intermediate start parameters */

/* === Prototypes =========================================================== */


/* === Implementation ======================================================= */

/*
 * @brief Internal function to generate confirmation
 * for MLME_START.request
 *
 * @param start_buf_ptr Pointer to MLME_START.request
 * @param start_req_status Status of the MLME_START.request
 */
static void gen_mlme_start_conf(uint8_t *start_buf_ptr,
                                uint8_t start_req_status)
{

    /* Use the same start request buffer for start confirm */
    mlme_start_conf_t *msc = (mlme_start_conf_t *)BMM_BUFFER_POINTER((buffer_t *)start_buf_ptr);

    msc->cmdcode = MLME_START_CONFIRM;
    msc->status = start_req_status;

    /* The confirmation message is appended to the MAC-NHLE queue */
    qmm_queue_append(&mac_nhle_q, (buffer_t *)start_buf_ptr);
}



#ifndef REDUCED_PARAM_CHECK
/*
 * @brief Checks the parameters of a MLME_START.request
 *
 * This function validates the parameters of the MLME_START.request.
 *
 * @param msg Pointer to the MLME_START.request message which holds the
 * start parameters
 *
 * @return true if parameters are valid, false otherwise
 */
static bool check_start_parameter(mlme_start_req_t *msg)
{
    bool param_validation_status = false;

#ifndef BEACON_SUPPORT
    /*
     * In a build without beacon support the beacon order and/or
     * superframe order needs to be 15, since only a nonbeacon-enabled network
     * can be created.
     * The following code is valid for starting a new network as well as for
     * for a coordinator realignment attempt.
     */
    if ((msg->BeaconOrder != NON_BEACON_NWK) || (msg->SuperframeOrder != NON_BEACON_NWK))
    {
        /* This is not allowed in a build without beacon support. */
        return param_validation_status;
    }
#endif

    /*
     * Value of beacon order has to be less than or equal to 15 (Non beacon
     * Network). The value of superframe has to be either less than equal to
     * beacon order.
     */
    if ((msg->BeaconOrder <= NON_BEACON_NWK) &&
        ((msg->SuperframeOrder <= msg->BeaconOrder) ||
         (NON_BEACON_NWK == msg->SuperframeOrder)))
    {
        /*
         * Coordinator realignment command can only be given by a
         * PAN coordinator or cordinator (which is associated to a
         * PAN coordinator).
         */
        if ((msg->CoordRealignment) &&
            ((MAC_ASSOCIATED == mac_state) || (MAC_IDLE == mac_state))
           )
        {
            /*
             * We are neigher the requested to be the PAN Coordinator,
             * nor are we associated, so Realignment is not allowed at
             * this stage.
             */
            param_validation_status = false;
        }
        else
        {
            param_validation_status = true;
        }
    }

    return param_validation_status;
}
#endif  /* REDUCED_PARAM_CHECK */



/**
 * @brief The MLME-START.request primitive makes a request for the device to
 * start using a new superframe configuration
 *
 * @param m Pointer to MLME_START.request message issued by the NHLE
 */
void mlme_start_request(uint8_t *m)
{
    mlme_start_req_t *msg = (mlme_start_req_t *)BMM_BUFFER_POINTER((buffer_t *)m);

    /*
     * The MLME_START.request parameters are copied into a global variable
     * structure, which is used by check_start_parameter() function.
     */
    memcpy(&start_parms, msg, sizeof(start_parms));

    if (BROADCAST == tal_pib_ShortAddress)
    {
        /*
         * The device is void of short address. This device cannot start a
         * network, hence a confirmation is given back.
         */
        gen_mlme_start_conf(m, MAC_NO_SHORT_ADDRESS);
        return;
    }

#ifndef REDUCED_PARAM_CHECK
    if (!check_start_parameter(msg))
    {
        /*
         * The MLME_START.request parameters are invalid, hence confirmation
         * is given to NHLE.
         */
        gen_mlme_start_conf(m, MAC_INVALID_PARAMETER);
    }
    else
#endif  /* REDUCED_PARAM_CHECK */
    {
        /*
         * All the start parameters are valid, hence MLME_START.request can
         * proceed.
         */
        set_tal_pib_internal(mac_i_pan_coordinator,
                             (void *)&(msg->PANCoordinator));

        if (start_parms.CoordRealignment)
        {
            /* First inform our devices of the configuration change */
            if (!mac_tx_coord_realignment_command(COORDINATORREALIGNMENT,
                                                  (buffer_t *)m,
                                                  start_parms.PANId,
                                                  start_parms.LogicalChannel,
                                                  start_parms.ChannelPage))
            {
                /*
                 * The coordinator realignment command was unsuccessful,
                 * hence the confiramtion is given to NHLE.
                 */
                gen_mlme_start_conf(m, MAC_INVALID_PARAMETER);
            }
        }
        else
        {
            /* This is a normal MLME_START.request. */

            uint8_t channel_set_status;

            /* The new PIBs are set at the TAL. */
            set_tal_pib_internal(macBeaconOrder, (void *)&(msg->BeaconOrder));

            /* If macBeaconOrder is equal to 15, set also macSuperframeOrder to 15. */
            if (msg->BeaconOrder == NON_BEACON_NWK)
            {
                msg->SuperframeOrder = NON_BEACON_NWK;
            }

            set_tal_pib_internal(macSuperframeOrder, (void *)&(msg->SuperframeOrder));

#ifdef BEACON_SUPPORT
            /*
             * Symbol times are calculated according to the new BO and SO
             * values.
             */
            if (tal_pib_BeaconOrder < NON_BEACON_NWK)
            {
                set_tal_pib_internal(macBattLifeExt,
                                     (void *)&(start_parms.BatteryLifeExtension));
            }
#endif /* BEACON_SUPPORT */

            /* Check is done to see if radio is need to be awaken */
            if (RADIO_SLEEPING == mac_radio_sleep_state)
            {
                /* Wake up radio first */
                mac_trx_wakeup();
            }
// LSR
			/*
			* For nonbeacon-enabled networks we do set macRxOnWhenIdle
			* to have the coordinator always awake.
			* Since currently power save is not implemented for beacon mode
			* the coordinator also has to stay awake always in beacon mode.
			* Once power save will be implemented for beacon mode,
			* the coordinator has to handle this PIB attribute as well
			*/
			mac_pib_macRxOnWhenIdle = true;
// ^

            /* MLME_START.request parameters other than BO and SO are set at TAL */
            set_tal_pib_internal(macPANId, (void *)&(start_parms.PANId));

            channel_set_status = set_tal_pib_internal(phyCurrentChannel,
                (void *)&(start_parms.LogicalChannel));

            set_tal_pib_internal(phyCurrentPage,
                                 (void *)&(start_parms.ChannelPage));

            set_tal_pib_internal(mac_i_pan_coordinator,
                                 (void *)&(start_parms.PANCoordinator));

            if ((MAC_SUCCESS == channel_set_status) &&
                (PHY_RX_ON == tal_rx_enable(PHY_RX_ON))
               )
            {
                if (start_parms.PANCoordinator)
                {
                    mac_state = MAC_PAN_COORD_STARTED;
                }
                else
                {
                    mac_state = MAC_COORDINATOR;
                }

                gen_mlme_start_conf(m, MAC_SUCCESS);

#ifdef BEACON_SUPPORT
                /*
                 * In case we have a beaconing network, the beacon timer needs
                 * to be started now.
                 */
                if (tal_pib_BeaconOrder != NON_BEACON_NWK)
                {
                    mac_start_beacon_timer();
                }
#endif  /* BEACON_SUPPORT */
            }
            else
            {
                /* Start of network failed. */
                gen_mlme_start_conf(m, MAC_INVALID_PARAMETER);
            }

            /* Set radio to sleep if allowed */
            mac_sleep_trans();
        }
    }
}



/**
 * @brief Continues handling of MLME_START.request (Coordinator realignment)
 * command
 *
 * This function is called once the coordinator realignment command is
 * sent out to continue the handling of the MLME_START.request command.
 *
 * @param tx_status Status of the coordinator realignment command
 *                  transmission
 * @param buf Buffer for start confirmation
 */
void mac_coord_realignment_command_tx_success(uint8_t tx_status, buffer_t *buf)
{
    uint8_t conf_status = MAC_INVALID_PARAMETER;
    retval_t channel_set_status;

    if (MAC_SUCCESS == tx_status)
    {
        /* The parameters of the existing PAN are updated. */
        channel_set_status = set_tal_pib_internal(phyCurrentChannel,
            (void *)&start_parms.LogicalChannel);

#if (DEBUG > 1)
        ASSERT(MAC_SUCCESS == channel_set_status);
#endif
        if (MAC_SUCCESS == channel_set_status)
        {
            conf_status = MAC_SUCCESS;

            set_tal_pib_internal(phyCurrentPage,
                                 (void *)&(start_parms.ChannelPage));

            set_tal_pib_internal(macPANId,
                                 (void *)&(start_parms.PANId));

#ifdef BEACON_SUPPORT
            /*
             * Store current beacon order in order to be able to detect
             * switching from nonbeacon to beacon network.
             */
            uint8_t cur_beacon_order = tal_pib_BeaconOrder;

            set_tal_pib_internal(macBeaconOrder,
                                 (void *)&(start_parms.BeaconOrder));
            set_tal_pib_internal(macSuperframeOrder,
                                 (void *)&(start_parms.SuperframeOrder));

            /*
             * New symbol times for beacon time (in sysbols) and inactive time are
             * calculated according to the new superframe configuration.
             */
            if (start_parms.BeaconOrder < NON_BEACON_NWK)
            {
                set_tal_pib_internal(macBattLifeExt,
                                     (void *)&(start_parms.BatteryLifeExtension));
            }

            if ((NON_BEACON_NWK < cur_beacon_order) &&
                (start_parms.BeaconOrder == NON_BEACON_NWK))
            {
                /*
                 * This is a transition from a beacon enabled network to
                 * a nonbeacon enabled network.
                 * In this case the broadcast data queue will never be served.
                 *
                 * Therefore the broadcast queue needs to be emptied.
                 * The standard does not define what to do now.
                 * The current implementation will try to send all pending broadcast
                 * data frames immediately, thus giving the receiving nodes a chance
                 * receive them.
                 */
                while (broadcast_q.size > 0)
                {
                    mac_tx_pending_bc_data();
                }
            }

            if ((NON_BEACON_NWK == cur_beacon_order) &&
                (start_parms.BeaconOrder < NON_BEACON_NWK))
            {
                /*
                 * This is a transition from a nonbeacon enabled network to
                 * a beacon enabled network, hence the beacon timer will be
                 * started.
                 */
                mac_start_beacon_timer();
            }
#endif  /* BEACON_SUPPORT */
        }
    }

    gen_mlme_start_conf((uint8_t *)buf, conf_status);

    /* Set radio to sleep if allowed */
    mac_sleep_trans();
} /* mac_coord_realignment_command_tx_success() */


#endif /* (MAC_START_REQUEST_CONFIRM == 1) || defined(DOXYGEN) */

/* EOF */
