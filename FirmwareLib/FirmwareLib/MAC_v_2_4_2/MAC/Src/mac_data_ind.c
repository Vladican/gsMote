/**
 * @file mac_data_ind.c
 *
 * @brief Implements incoming frame handling in the MAC
 *
 * This file handles parsing of incoming frames from the PHY. The frames are
 * parsed in two passes, whereas the first pass is initiated by the PHY and
 * handles all time critical task necessary to initiate ACk transmission.
 * The second pass handles the further filtering of the frames and calls the
 * corresponding actions within the MAC. As long as the second pass of a valid
 * frame is not finished, no further frame can be processed.
 *
 * $Id: mac_data_ind.c 20214 2010-02-08 12:37:34Z sschneid $
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
#endif  /* (MAC_INDIRECT_DATA_FFD == 1) */
#include "stack_config.h"
#include "mac_internal.h"
#include "mac.h"
#include "mac_config.h"
#include "mac_build_config.h"

/* === Macros =============================================================== */

/*
 * Mask of the GTS descriptor counter
 */
#define GTS_DESCRIPTOR_COUNTER_MASK     (0x07)

/*
 * Extract the PAN Coordinator bit from the Superframe Spec.
 */
#define GET_PAN_COORDINATOR(spec)      (((spec) & 0x4000) >> 14)

#if (MAC_PAN_ID_CONFLICT_NON_PC == 1)
/*
 * PAN-Id conflict notification payload length
 */
#define PAN_ID_CONFLICT_PAYLOAD_LEN     (1)
#endif  /* (MAC_PAN_ID_CONFLICT_NON_PC == 1) */

/* === Globals ============================================================= */

/*
 * Temporary storage for LQI received
 */
static uint8_t ppdu_link_quality;

/* === Prototypes ========================================================== */

static void init_parse_buffer(void);
static bool parse_payload(frame_info_t *frameptr);
static bool process_data_ind_not_transient(uint8_t *msg, frame_info_t *f_tr);
#if (MAC_SCAN_SUPPORT == 1)
static bool process_data_ind_scanning(uint8_t *msg);
#endif /* (MAC_SCAN_SUPPORT == 1) */

#ifdef PROMISCUOUS_MODE
static void prom_mode_rx_frame(buffer_t *b_ptr, frame_info_t *f_ptr);
#endif  /* PROMISCUOUS_MODE */

#if (MAC_PAN_ID_CONFLICT_AS_PC == 1)
static void check_for_pan_id_conflict_as_pc(bool in_scan);
#endif  /* (MAC_PAN_ID_CONFLICT_AS_PC == 1) */

#if (MAC_PAN_ID_CONFLICT_NON_PC == 1)
static void check_for_pan_id_conflict_non_pc(bool in_scan);
static bool tx_pan_id_conf_notif(void);
#endif  /* (MAC_PAN_ID_CONFLICT_NON_PC == 1) */

/* === Implementation ====================================================== */

/**
 * @brief Depending on received frame the appropriate function is called
 *
 * @param msg Pointer to the buffer.
 */
void mac_process_tal_data_ind(uint8_t *msg)
{
    frame_info_t *frameptr = (frame_info_t *)BMM_BUFFER_POINTER((buffer_t *)msg);
    bool processed_tal_data_indication = false;

#ifdef PROMISCUOUS_MODE
    if (tal_pib_PromiscuousMode)
    {
        /*
         * In promiscuous mode all received frames are forwarded to the
         * higher layer or application my means of MCPS_DATA.indication
         * primitives.
         */
        prom_mode_rx_frame((buffer_t *)msg, frameptr);
        return;
    }
#endif  /* PROMISCUOUS_MODE */

    if (!parse_payload(frameptr))
    {
        /* Frame parsing failed */
        bmm_buffer_free((buffer_t *)msg);
        return;
    }

    /* Check if the MAC is busy processing the previous requests */
    if (mac_busy)
    {
        /*
         * If MAC has to process an incoming frame that requires a response
         * (i.e. beacon request and data request) then process this operation
         * once the MAC has become free. Put the request received back into the
         * MAC internal event queue.
         */
        if (FCF_FRAMETYPE_MAC_CMD == mac_parse_data.frame_type)
        {
            if (DATAREQUEST == mac_parse_data.mac_command ||
                BEACONREQUEST == mac_parse_data.mac_command)
            {
                bmm_buffer_free((buffer_t *)msg);
                return;
            }
        }
    }

    switch (mac_poll_state)
    {
        case MAC_POLL_IDLE:
            /*
             * We are in no transient state.
             * Now are either in a non-transient MAC state or scanning.
             */
            if (MAC_SCAN_IDLE == mac_scan_state)
            {
                /*
                 * Continue with handling the "real" non-transient MAC states now.
                 */
                processed_tal_data_indication = process_data_ind_not_transient(msg, frameptr);
            }
#if (MAC_SCAN_SUPPORT == 1)
            else
            {
                /* Scanning is ongoing. */
                processed_tal_data_indication = process_data_ind_scanning(msg);
            }
#endif /* (MAC_SCAN_SUPPORT == 1) */
            break;

#if (MAC_INDIRECT_DATA_BASIC == 1)
            /*
             * This is the 'wait for data' state after either
             * explicit poll or implicit poll.
             */
        case MAC_POLL_EXPLICIT:
        case MAC_POLL_IMPLICIT:
            /*
             * Function mac_process_data_response() resets the
             * MAC poll state.
             */
            mac_process_data_response();

            switch (mac_parse_data.frame_type)
            {
                case FCF_FRAMETYPE_MAC_CMD:
                {
                    switch (mac_parse_data.mac_command)
                    {

#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)
                        case ASSOCIATIONREQUEST:
                           mac_process_associate_request((buffer_t *)msg);
                           processed_tal_data_indication = true;
                           break;
#endif /* (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) */

#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)
                        case DISASSOCIATIONNOTIFICATION:
                            {
                                mac_process_disassociate_notification((buffer_t *)msg);

                                processed_tal_data_indication = true;

                                /*
                                 * Device needs to scan for networks again,
                                 * go into idle mode and reset variables
                                 */
                                mac_idle_trans();
                            }
                            break;
#endif /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) */

                        case DATAREQUEST:
#if (MAC_INDIRECT_DATA_FFD == 1)
                            if (indirect_data_q.size > 0)
                            {
                                mac_process_data_request((buffer_t *)msg);
                                processed_tal_data_indication = true;
                            }
                            else
                            {
                                mac_handle_tx_null_data_frame();
                            }
#endif  /*  (MAC_INDIRECT_DATA_FFD == 1) */
                            break;

                        case PANIDCONFLICTNOTIFICAION:
                            break;

#if (MAC_ORPHAN_INDICATION_RESPONSE == 1)
                        case ORPHANNOTIFICATION:
                            mac_process_orphan_notification((buffer_t *)msg);
                            processed_tal_data_indication = true;
                            break;
#endif /* (MAC_ORPHAN_INDICATION_RESPONSE == 1) */

#if (MAC_START_REQUEST_CONFIRM == 1)
                        case BEACONREQUEST:
                            if (MAC_COORDINATOR == mac_state)
                            {
                                /*
                                 * Only a Coordinator can both poll and
                                 * AND answer beacon request frames.
                                 * PAN Coordinators do not poll;
                                 * End devices do not answer beacon requests.
                                 */
                                mac_process_beacon_request((buffer_t *)msg);
                                processed_tal_data_indication = true;
                            }
                            break;
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */

#if (MAC_SYNC_LOSS_INDICATION == 1)
                        case COORDINATORREALIGNMENT:
                            /*
                             * Received coordinator realignment frame for
                             * entire PAN.
                             */
                            mac_process_coord_realign((buffer_t *)msg);
                            processed_tal_data_indication = true;
                            break;
#endif  /* (MAC_SYNC_LOSS_INDICATION == 1) */

                        default:
#if (DEBUG > 1)
                            ASSERT("Unsupported MAC frame in state MAC_EXPLICIT_POLL or MAC_POLL_IMPLICIT" == 0);
#endif
                            break;
                    }
                }
                break; /* case FCF_FRAMETYPE_MAC_CMD: */

                case FCF_FRAMETYPE_DATA:
                    mac_process_data_frame((buffer_t *)msg);
                    processed_tal_data_indication = true;
                    break;

                default:
                    break;
            }
            break;
            /* MAC_POLL_EXPLICIT, MAC_POLL_IMPLICIT */
#endif /* (MAC_INDIRECT_DATA_BASIC == 1) */


#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1)
        case MAC_AWAIT_ASSOC_RESPONSE:
            /*
             * We are either expecting an association reponse frame
             * or a null data frame.
             */
            if ((FCF_FRAMETYPE_MAC_CMD == mac_parse_data.frame_type) &&
                (ASSOCIATIONRESPONSE == mac_parse_data.mac_command)
               )
            {
                /* This is the expected association response frame. */
                pal_timer_stop(T_Poll_Wait_Time);

#if (DEBUG > 1)
                if (pal_is_timer_running(T_Poll_Wait_Time))
                {
                    ASSERT("T_Poll_Wait_Time tmr during association running" == 0);
                }
#endif
                mac_process_associate_response((buffer_t *)msg);
                processed_tal_data_indication = true;
            }
            else if (FCF_FRAMETYPE_DATA == mac_parse_data.frame_type)
            {
                mac_process_data_frame((buffer_t *)msg);
                processed_tal_data_indication = true;
            }
            break;
            /*  MAC_AWAIT_ASSOC_RESPONSE */
#endif /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */

        default:
#if (DEBUG > 1)
            ASSERT("Received frame in unsupported MAC poll state" == 0);
#endif
            break;
    }

    /* If message is not processed */
    if (!processed_tal_data_indication)
    {
        bmm_buffer_free((buffer_t *)msg);
    }
} /* mac_process_tal_data_ind() */



#if (MAC_SCAN_SUPPORT == 1)
/**
 * @brief Continues processing a data indication from the TAL for
 *        scanning states of the MAC (mac_scan_state == MAC_SCAN_IDLE).
 *
 * @param msg Pointer to the buffer.
 *
 * @return bool True if frame has been processed, or false otherwise.
 */
static bool process_data_ind_scanning(uint8_t *msg)
{
    bool processed_in_scanning = false;
    /*
     * We are in a scanning process now (mac_scan_state is not MAC_SCAN_IDLE),
     * so continue with the specific scanning states.
     */
    switch (mac_scan_state)
    {
#if (MAC_SCAN_ED_REQUEST_CONFIRM == 1)
        /* Energy Detect scan */
        case MAC_SCAN_ED:
            /*
             * Ignore all frames received while performing ED measurement,
             * or while performing CCA.
             */
            msg = msg;  /* Keep compiler happy. */
            break;
#endif /* (MAC_SCAN_ED_REQUEST_CONFIRM == 1) */


#if ((MAC_SCAN_PASSIVE_REQUEST_CONFIRM == 1) || (MAC_SCAN_ACTIVE_REQUEST_CONFIRM == 1))
        /* Active scan or passive scan */
        case MAC_SCAN_ACTIVE:
        case MAC_SCAN_PASSIVE:
            if (FCF_FRAMETYPE_BEACON == mac_parse_data.frame_type)
            {
#if (MAC_PAN_ID_CONFLICT_AS_PC == 1)
                /* PAN-Id conflict detection as PAN-Coordinator. */
                if (MAC_PAN_COORD_STARTED == mac_state)
                {
                    /* Node is currently scanning. */
                    check_for_pan_id_conflict_as_pc(true);
                }
#endif  /* (MAC_PAN_ID_CONFLICT_AS_PC == 1) */
#if (MAC_PAN_ID_CONFLICT_NON_PC == 1)
                if (mac_pib_macAssociatedPANCoord &&
                    ((MAC_ASSOCIATED == mac_state) || (MAC_COORDINATOR == mac_state))
                   )
                {
                    check_for_pan_id_conflict_non_pc(true);
                }
#endif  /* (MAC_PAN_ID_CONFLICT_NON_PC == 1) */
                mac_process_beacon_frame((buffer_t *)msg);
                processed_in_scanning = true;
            }
            break;
#endif /* ((MAC_SCAN_PASSIVE_REQUEST_CONFIRM == 1) || (MAC_SCAN_ACTIVE_REQUEST_CONFIRM == 1)) */


#if (MAC_SCAN_ORPHAN_REQUEST_CONFIRM == 1)
        /* Orphan scan */
        case MAC_SCAN_ORPHAN:
            if (FCF_FRAMETYPE_MAC_CMD == mac_parse_data.frame_type &&
                COORDINATORREALIGNMENT == mac_parse_data.mac_command)
            {
                /*
                 * Received coordinator realignment frame in the middle of
                 * an orphan scan.
                 */
                pal_timer_stop(T_Scan_Duration);

                mac_process_orphan_realign((buffer_t *)msg);
                processed_in_scanning = true;
            }
            break;
#endif /* (MAC_SCAN_ORPHAN_REQUEST_CONFIRM == 1) */


        default:
        {
#if (DEBUG > 1)
            ASSERT("Unexpected TAL data indication while checking mac_scan_state" == 0);
#endif
        }
        break;
    }

    return (processed_in_scanning);
}
#endif /* (MAC_SCAN_SUPPORT == 1) */



/**
 * @brief Continues processing a data indication from the TAL for
 *        non-polling and non-scanning states of the MAC
 *        (mac_poll_state == MAC_POLL_IDLE, mac_scan_state == MAC_SCAN_IDLE).
 *
 * @param msg Pointer to the buffer.
 * @param f_ptr Pointer to the frame_info_t structure.
 *
 * @return bool True if frame has been processed, or false otherwise.
 */
static bool process_data_ind_not_transient(uint8_t *msg, frame_info_t *f_tr)
{
    bool processed_in_not_transient = false;
    /*
     * We are in MAC_POLL_IDLE and MAC_SCAN_IDLE now,
     * so continue with the real MAC states.
     */
    switch (mac_state)
    {
#if (MAC_START_REQUEST_CONFIRM == 1)
        case MAC_PAN_COORD_STARTED:
        {
            switch (mac_parse_data.frame_type)
            {
                case FCF_FRAMETYPE_MAC_CMD:
                {
                    switch (mac_parse_data.mac_command)
                    {
#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)
                        case ASSOCIATIONREQUEST:
                            mac_process_associate_request((buffer_t *)msg);
                            processed_in_not_transient = true;
                            break;
#endif /* (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) */

#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)
                        case DISASSOCIATIONNOTIFICATION:
                            mac_process_disassociate_notification((buffer_t *)msg);
                            processed_in_not_transient = true;
                            break;
#endif /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) */

#if (MAC_INDIRECT_DATA_FFD == 1)
                        case DATAREQUEST:
                            if (indirect_data_q.size > 0)
                            {
                                mac_process_data_request((buffer_t *)msg);
                                processed_in_not_transient = true;
                            }
                            else
                            {
                                mac_handle_tx_null_data_frame();
                            }
                            break;
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

#if (MAC_ORPHAN_INDICATION_RESPONSE == 1)
                        case ORPHANNOTIFICATION:
                            mac_process_orphan_notification((buffer_t *)msg);
                            processed_in_not_transient = true;
                            break;
#endif /* (MAC_ORPHAN_INDICATION_RESPONSE == 1) */

                        case BEACONREQUEST:
                            mac_process_beacon_request((buffer_t *)msg);
                            processed_in_not_transient = true;
                            break;

#if (MAC_PAN_ID_CONFLICT_AS_PC == 1)
                        case PANIDCONFLICTNOTIFICAION:
                            mac_sync_loss(MAC_PAN_ID_CONFLICT);
                            break;
#endif  /* (MAC_PAN_ID_CONFLICT_AS_PC == 1) */

                        default:
                            break;
                    }
                }
                break;

                case FCF_FRAMETYPE_DATA:
                    mac_process_data_frame((buffer_t *)msg);
                    processed_in_not_transient = true;
                    break;

#if (MAC_PAN_ID_CONFLICT_AS_PC == 1)
                case FCF_FRAMETYPE_BEACON:
                    /* PAN-Id conflict detection as PAN-Coordinator. */
                    /* Node is not scanning. */
                    check_for_pan_id_conflict_as_pc(false);
                    break;
#endif  /* (MAC_PAN_ID_CONFLICT_AS_PC == 1) */

                default:
                    break;
             }
        }
        break;
        /* MAC_PAN_COORD_STARTED */
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */

        case MAC_IDLE:
#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1)
        case MAC_ASSOCIATED:
#endif /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */
#if (MAC_START_REQUEST_CONFIRM == 1)
        case MAC_COORDINATOR:
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */
        {
            /* Is it a Beacon from our parent? */
            switch (mac_parse_data.frame_type)
            {
#if (MAC_SYNC_REQUEST == 1)
                case FCF_FRAMETYPE_BEACON:
                {
                    uint32_t beacon_tx_time_symb;
#if (DEBUG > 1)
                    retval_t set_status;
#endif

                    /* Check for PAN-Id conflict being NOT a PAN Corodinator. */
#if (MAC_PAN_ID_CONFLICT_NON_PC == 1)
                    if (mac_pib_macAssociatedPANCoord && (MAC_IDLE != mac_state))
                    {
                        check_for_pan_id_conflict_non_pc(false);
                    }
#endif  /* (MAC_PAN_ID_CONFLICT_NON_PC == 1) */

                    /* Check if the beacon is received from my parent. */
                    if ((mac_parse_data.src_panid == tal_pib_PANId) &&
                        (((mac_parse_data.src_addr_mode == FCF_SHORT_ADDR) &&
                          (mac_parse_data.src_addr.short_address ==
                                mac_pib_macCoordShortAddress)) ||
                         ((mac_parse_data.src_addr_mode == FCF_LONG_ADDR) &&
                          (mac_parse_data.src_addr.long_address ==
                                mac_pib_macCoordExtendedAddress))))
                    {
                        beacon_tx_time_symb = TAL_CONVERT_US_TO_SYMBOLS(f_tr->time_stamp);

#if (DEBUG > 1)
                        set_status =
#endif
                            set_tal_pib_internal(macBeaconTxTime, (void *)&beacon_tx_time_symb);
#if (DEBUG > 1)
                        ASSERT(MAC_SUCCESS == set_status);
#endif
                        if ((MAC_SYNC_TRACKING_BEACON == mac_sync_state) ||
                            (MAC_SYNC_BEFORE_ASSOC == mac_sync_state)
                           )
                        {
                            uint32_t nxt_bcn_tm;
                            uint32_t beacon_int_symb;

                            /* Process a received beacon. */
                            mac_process_beacon_frame((buffer_t *)msg);

                            /* Initialize beacon tracking timer. */
                            {
                                retval_t tmr_start_res = FAILURE;

#ifdef BEACON_SUPPORT
                                if (tal_pib_BeaconOrder < NON_BEACON_NWK)
                                {
                                    beacon_int_symb =
                                        TAL_GET_BEACON_INTERVAL_TIME(tal_pib_BeaconOrder);
                                }
                                else
#endif /* BEACON_SUPPORT */
                                {
                                    beacon_int_symb =
                                        TAL_GET_BEACON_INTERVAL_TIME(BO_USED_FOR_MAC_PERS_TIME);
                                }

                                pal_timer_stop(T_Beacon_Tracking_Period);

#if (DEBUG > 1)
                                if (pal_is_timer_running(T_Beacon_Tracking_Period))
                                {
                                    ASSERT("Bcn tmr running" == 0);
                                }
#endif

                                do
                                {
                                    /*
                                     * Calculate the time for next beacon
                                     * transmission
                                     */
                                    beacon_tx_time_symb = tal_add_time_symbols(beacon_tx_time_symb,
                                                                               beacon_int_symb);
                                    /*
                                     * Take into account the time taken by
                                     * the radio to wakeup from sleep state
                                     */
                                    nxt_bcn_tm = tal_sub_time_symbols(beacon_tx_time_symb,
                                                                      TAL_RADIO_WAKEUP_TIME_SYM << (tal_pib_BeaconOrder + 2));

                                    tmr_start_res =
                                        pal_timer_start(T_Beacon_Tracking_Period,
                                                        TAL_CONVERT_SYMBOLS_TO_US(nxt_bcn_tm),
                                                        TIMEOUT_ABSOLUTE,
                                                        (FUNC_PTR)mac_t_tracking_beacons_cb,
                                                        NULL);
#if (DEBUG > 1)
                                    ASSERT(MAC_SUCCESS == tmr_start_res);
#endif
                                }
                                while (MAC_SUCCESS != tmr_start_res);
                            }

                            /*
                             * Initialize superframe timer if required only
                             * for devices because Superframe timer is already running for
                             * coordinator.
                             */
                            /* TODO */
                            /*
                            if (MAC_COORDINATOR != mac_state)
                            {
                                if (tal_pib_SuperFrameOrder < tal_pib_BeaconOrder)
                                {
                                        pal_timer_start(T_Superframe,
                                                        TAL_CONVERT_SYMBOLS_TO_US(
                                                            TAL_GET_SUPERFRAME_DURATION_TIME(
                                                                tal_pib_SuperFrameOrder)),
                                                        TIMEOUT_RELATIVE,
                                                        (FUNC_PTR)mac_t_start_inactive_device_cb,
                                                        NULL);
                                }
                            }
                            */

                            /* Initialize missed beacon timer. */
                            mac_start_missed_beacon_timer();

                            /* A device that is neither scanning nor polling shall go to sleep now. */
                            if (
                                (MAC_COORDINATOR != mac_state) &&
                                (MAC_SCAN_IDLE == mac_scan_state) &&
                                (MAC_POLL_IDLE == mac_poll_state)
                               )
                            {
                                /*
                                 * If the last received beacon frame from our parent
                                 * has indicated pending broadbast data, we need to
                                 * stay awake, until the broadcast data has been received.
                                 */
                                if (!mac_bc_data_indicated)
                                {
                                    /* Set radio to sleep if allowed */
                                    mac_sleep_trans();
                                }
                            }
                        }
                        else if (MAC_SYNC_ONCE == mac_sync_state)
                        {
                            mac_process_beacon_frame((buffer_t *)msg);

                            /* Do this after processing the beacon. */
                            mac_sync_state = MAC_SYNC_NEVER;
                        }
                        else
                        {
                            /* Process the beacon frame */
                            bmm_buffer_free((buffer_t *)msg);
                        }

                        processed_in_not_transient = true;
                    }
                    else
                    {
                       /* No action taken, buffer will be freed. */
                    }
                }
                break;
#else   /* (MAC_SYNC_REQUEST == 0) */
                case FCF_FRAMETYPE_BEACON:
                {
                    /* Check for PAN-Id conflict being NOT a PAN Corodinator. */
#if ((MAC_PAN_ID_CONFLICT_NON_PC == 1) && (MAC_ASSOCIATION_REQUEST_CONFIRM == 1))
                    if (mac_pib_macAssociatedPANCoord && (MAC_IDLE != mac_state))
                    {
                        check_for_pan_id_conflict_non_pc(false);
                    }
#endif  /* ((MAC_PAN_ID_CONFLICT_NON_PC == 1) && (MAC_ASSOCIATION_REQUEST_CONFIRM == 1)) */
                }
                break;
#endif /* (MAC_SYNC_REQUEST == 0/1) */



                case FCF_FRAMETYPE_MAC_CMD:
                    switch (mac_parse_data.mac_command)
                    {
#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)
                        case DISASSOCIATIONNOTIFICATION:
                            mac_process_disassociate_notification((buffer_t *)msg);
                            processed_in_not_transient = true;

                            if (MAC_ASSOCIATED == mac_state)
                            {
                                /*
                                 * Device needs to scan for networks again,
                                 * go into idle mode and reset variables
                                 */
                                mac_idle_trans();
                            }
                            break;
#endif /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) */

#if (MAC_SYNC_LOSS_INDICATION == 1)
                        case COORDINATORREALIGNMENT:
                            /*
                             * Received coordinator realignment frame from
                             * coordinator while NOT performing orphan scan.
                             */
                            mac_process_coord_realign((buffer_t *)msg);
                            processed_in_not_transient = true;
                            break;
#endif /* (MAC_SYNC_LOSS_INDICATION == 1) */

#if (MAC_START_REQUEST_CONFIRM == 1)
                        case BEACONREQUEST:
                            if (MAC_COORDINATOR == mac_state)
                            {
                                /*
                                 * Only Coordinators (no End devices) answer
                                 * Beacon requests.
                                 */
                                mac_process_beacon_request((buffer_t *)msg);
                                processed_in_not_transient = true;
                            }
                            break;
#endif  /* (MAC_START_REQUEST_CONFIRM == 1) */

#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)
                        case ASSOCIATIONREQUEST:
                            mac_process_associate_request((buffer_t *)msg);
                            processed_in_not_transient = true;
                            break;
#endif /* (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) */

#if (MAC_INDIRECT_DATA_FFD == 1)
                        case DATAREQUEST:
                            if (indirect_data_q.size > 0)
                            {
                                mac_process_data_request((buffer_t *)msg);
                                processed_in_not_transient = true;
                            }
                            else
                            {
                                mac_handle_tx_null_data_frame();
                            }
                            break;
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

#if (MAC_ORPHAN_INDICATION_RESPONSE == 1)
                        case ORPHANNOTIFICATION:
                            mac_process_orphan_notification((buffer_t *)msg);
                            processed_in_not_transient = true;
                            break;
#endif /* (MAC_ORPHAN_INDICATION_RESPONSE == 1) */
                        default:
                            break;
                    }
                    break;

                case FCF_FRAMETYPE_DATA:
                    mac_process_data_frame((buffer_t *)msg);
                    processed_in_not_transient = true;
                    break;

                default:
                    break;
            }
        }
        break;
        /* MAC_IDLE, MAC_ASSOCIATED, MAC_COORDINATOR */

        default:
        {
#if (DEBUG > 1)
            ASSERT("Unexpected TAL data indication" == 0);
#endif
        }
        break;
    }

    f_tr = f_tr;    /* Keep compiler happy. */

    return (processed_in_not_transient);
}



/*
 * @brief Parse an MPDU
 *
 * This function parses an MPDU which got from data_indication,
 * and leaves the parse result in mac_parse_data.
 *
 * @param frameptr Pointer to frame received from TAL
 *
 * @return bool True if frame OK, or false if frame is invalid.
 */
static bool parse_payload(frame_info_t *frameptr)
{
    uint8_t     payload_index;
    uint8_t     temp_byte;
    uint16_t    fcf;
    uint8_t     *framep;

    init_parse_buffer();

    mac_parse_data.fcf = fcf = frameptr->frame_ctrl;

    if (fcf & FCF_SECURITY_ENABLED)
    {
        return false;
    }

    mac_parse_data.frame_type = FCF_GET_FRAMETYPE(fcf);

    mac_parse_data.dest_addr_mode = FCF_GET_DEST_ADDR_MODE(fcf);
    mac_parse_data.src_addr_mode = FCF_GET_SOURCE_ADDR_MODE(fcf);

    mac_parse_data.dest_panid = frameptr->dest_panid;
    mac_parse_data.src_panid = frameptr->src_panid;

    ADDR_COPY_DST_SRC_64(mac_parse_data.dest_addr.long_address, frameptr->dest_address);
    ADDR_COPY_DST_SRC_64(mac_parse_data.src_addr.long_address, frameptr->src_address);

    mac_parse_data.sequence_number = frameptr->seq_num;

    mac_parse_data.payload_length = frameptr->payload_length;

    if (FCF_FRAMETYPE_MAC_CMD == mac_parse_data.frame_type)
    {
        mac_parse_data.mac_command = frameptr->payload[0];
    }

    payload_index = 0;
    mac_parse_data.ppduLinkQuality = ppdu_link_quality;

    mac_parse_data.timestamp = frameptr->time_stamp;

    framep = frameptr->payload;

    switch (mac_parse_data.frame_type)
    {
        case FCF_FRAMETYPE_BEACON:
            /* Get the Superframe specification */
            memcpy(&mac_parse_data.payload_data.beacon_data.superframe_spec,
                   &framep[payload_index],
                   sizeof(uint16_t));
            payload_index += sizeof(uint16_t);

            /* Get the GTS specification */
            mac_parse_data.payload_data.beacon_data.gts_spec = framep[payload_index++];

            /*
             * If the GTS specification descriptor count is > 0, then
             * increase the index by the correct GTS field octet number
             * GTS directions and GTS address list will not be parsed
             */
            temp_byte = (mac_parse_data.payload_data.beacon_data.gts_spec &
                         GTS_DESCRIPTOR_COUNTER_MASK);
            if (temp_byte > 0)
            {
                /* 1 octet GTS diresctions + GTS address list */
                payload_index += 1 + temp_byte;
            }

            /* Get the Pending address specification */
            mac_parse_data.payload_data.beacon_data.pending_addr_spec =
                framep[payload_index++];

            /*
             * If the Pending address specification indicates that the number of
             * short or long addresses is > 0, then get the short and/or
             * long addresses
             */
            temp_byte = NUM_SHORT_PEND_ADDR(mac_parse_data.payload_data.beacon_data.
                                                pending_addr_spec);

            if (temp_byte > 0)
            {
                memcpy(mac_parse_data.payload_data.beacon_data.
                       pending_addr_list_short,
                       &framep[payload_index],
                       (temp_byte * sizeof(uint16_t)));
                payload_index += (temp_byte * sizeof(uint16_t));
            }

            temp_byte = NUM_LONG_PEND_ADDR(mac_parse_data.payload_data.beacon_data.
                                                pending_addr_spec);

            if (temp_byte > 0)
            {
                memcpy(mac_parse_data.payload_data.beacon_data.
                       pending_addr_list_long,
                       &framep[payload_index],
                       (temp_byte * sizeof(uint64_t)));

                payload_index += (temp_byte * sizeof(uint64_t));
            }

            /* Is there a beacon payload ? */
            if (frameptr->payload_length > payload_index)
            {
                mac_parse_data.payload_length =
                    frameptr->payload_length - payload_index;

                memcpy(mac_parse_data.payload_data.beacon_data.payload,
                       &framep[payload_index],
                       mac_parse_data.payload_length);
            }
            else
            {
                mac_parse_data.payload_length = 0;
            }
            break;

        case FCF_FRAMETYPE_DATA:
            if (frameptr->payload_length)
            {
                mac_parse_data.payload_length = frameptr->payload_length;

                /*
                 * In case the device got a frame with a corrupted payload
                 * length
                 */
                if (mac_parse_data.payload_length >= aMaxMACPayloadSize)
                {
                    mac_parse_data.payload_length = aMaxMACPayloadSize;
                }

                memcpy(mac_parse_data.payload_data.data.payload,
                       &framep[payload_index],
                       mac_parse_data.payload_length);
            }
            else
            {
                mac_parse_data.payload_length = 0;
            }
            break;

        case FCF_FRAMETYPE_MAC_CMD:
            /*
             * Leave the SDL path here.
             *
             * SDL would normally complete parsing the command, then
             * fall back to the caller, which would eventually notice
             * that an ACK needs to be sent (76(154), p. 328). Upon
             * that, it would walk a lot of state transitions until it
             * could eventually transmit the ACK frame. This is very
             * likely to happen way too late to be in time for an ACK
             * frame, and it's somewhat funny why that part of SDL
             * would actually go all that way down as all other ACK
             * frames are pushed out onto the queue really quickly,
             * without taking care to await all the response message
             * ping-pong first.
             */
            payload_index = 1;

            switch (mac_parse_data.mac_command)
            {
#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)
            case ASSOCIATIONREQUEST:
                mac_parse_data.payload_data.assoc_req_data.capability_info =
                    framep[payload_index++];
                break;
#endif /* (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) */

#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1)
            case ASSOCIATIONRESPONSE:
                memcpy(&mac_parse_data.payload_data.assoc_response_data.short_addr,
                       &framep[payload_index],
                       sizeof(uint16_t));
                payload_index += sizeof(uint16_t);
                mac_parse_data.payload_data.assoc_response_data.assoc_status =
                    framep[payload_index];
                break;
#endif /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */

#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)
            case DISASSOCIATIONNOTIFICATION:
                mac_parse_data.payload_data.disassoc_req_data.disassoc_reason =
                    framep[payload_index++];
                break;
#endif /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) */

            case COORDINATORREALIGNMENT:
                memcpy(&mac_parse_data.payload_data.coord_realign_data.pan_id,
                       &framep[payload_index],
                       sizeof(uint16_t));
                payload_index += sizeof(uint16_t);

                memcpy(&mac_parse_data.payload_data.coord_realign_data.coord_short_addr,
                       &framep[payload_index],
                       sizeof(uint16_t));
                payload_index += sizeof(uint16_t);

                mac_parse_data.payload_data.coord_realign_data.logical_channel =
                    framep[payload_index++];

                memcpy(&mac_parse_data.payload_data.coord_realign_data.short_addr,
                       &framep[payload_index],
                       sizeof(uint16_t));
                payload_index += sizeof(uint16_t);

                /*
                 * If frame version subfield indicates a 802.15.4-2006 compatible frame,
                 * the channel page is appended as additional information element.
                 */
                if (fcf & FCF_FRAME_VERSION_2006)
                {
                    mac_parse_data.payload_data.coord_realign_data.channel_page =
                        framep[payload_index++];
                }
                break;

#if (MAC_ORPHAN_INDICATION_RESPONSE == 1)
            case ORPHANNOTIFICATION:
#endif /* (MAC_ORPHAN_INDICATION_RESPONSE == 1) */
            case DATAREQUEST:
#if (MAC_START_REQUEST_CONFIRM == 1)
            case BEACONREQUEST:
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */
#if (MAC_PAN_ID_CONFLICT_AS_PC == 1)
            case PANIDCONFLICTNOTIFICAION:
#endif  /* (MAC_PAN_ID_CONFLICT_AS_PC == 1) */
                break;

            default:
#if (DEBUG > 1)
                ASSERT("Unsupported MAC command in parse_MPDU" == 0);
#endif
                return false;
            }
            break;

        default:
            return false;
    }
    return true;
} /* parse_payload() */



/*
 * @brief Initializes the parse buffer
 *
 * This function initializes the parse buffer
 */
static void init_parse_buffer(void)
{
#if (DEBUG > 1)
    memset(&mac_parse_data, 0, sizeof(mac_parse_data));
    mac_parse_data.frame_type = 0xFF;   /* FF hex is an invalid frame type */
    mac_parse_data.dest_addr_mode = 0xFF;
    mac_parse_data.src_addr_mode = 0xFF;
    mac_parse_data.mac_command = 0xFF;
#else  /* DEBUG <= 1, i.e. optimize */
    mac_parse_data.payload_length = 0;
    mac_parse_data.src_panid = 0;
    mac_parse_data.dest_panid = 0;
#endif /* DEBUG > 1 */
}



/**
 * @brief Callback function called by TAL on recption of any frame.
 *
 * This function pushes an event into the TAL-MAC queue, indicating a
 * frame reception.
 *
 * @param frame Pointer to recived frame
 * @param lqi LQI of the recieved frame
 */
void tal_rx_frame_cb(frame_info_t *frame, uint8_t lqi)
{
    ppdu_link_quality = lqi;

    frame->msg_type = (frame_msgtype_t)TAL_DATA_INDICATION;

    if (NULL == frame->buffer_header)
    {
#if (DEBUG > 0)
        ASSERT("Null frame From TAL" == 0);
#endif
        return;
    }

    qmm_queue_append(&tal_mac_q, frame->buffer_header);
}



#ifdef PROMISCUOUS_MODE
static void prom_mode_rx_frame(buffer_t *b_ptr, frame_info_t *f_ptr)
{
    mcps_data_ind_t *mdi =
        (mcps_data_ind_t *)BMM_BUFFER_POINTER(b_ptr);

    uint32_t cur_time_stamp = f_ptr->time_stamp;
    uint8_t cur_pl_len = f_ptr->payload_length;

    /*
     * In promiscous mode the MCPS_DATA.indication is used as container
     * for the received frame.
     */

    /*
     * Since both f_ptr and mdi point to the same data storage place,
     * we need to save all required data first.
     * The time stamp has already been saved into .
     * So lets save the payload now.
     */

    /*
     * Move the payload data to the proper place which already contains
     * MHR of original frame (This has already been done by the TAL).
     */
    memmove(&mdi->data[1], &f_ptr->payload[0], cur_pl_len);

    mdi->data[0] = mdi->msduLength = cur_pl_len;

    /* Build the MLME_Data_indication parameters */
    mdi->DSN = 0;
    mdi->Timestamp = cur_time_stamp;

    /* Source address mode is 0. */
    mdi->SrcAddrMode = FCF_NO_ADDR;
    mdi->SrcPANId = 0;
    mdi->SrcAddr = 0;

    /* Destination address mode is 0.*/
    mdi->DstAddrMode = FCF_NO_ADDR;
    mdi->DstPANId = 0;
    mdi->DstAddr = 0;

    mdi->mpduLinkQuality = ppdu_link_quality;
    mdi->cmdcode = MCPS_DATA_INDICATION;

    /* Append MCPS data indication to MAC-NHLE queue */
    qmm_queue_append(&mac_nhle_q, b_ptr);
}
#endif  /* PROMISCUOUS_MODE */



#if (MAC_PAN_ID_CONFLICT_AS_PC == 1)
/**
 * @brief PAN-Id conflict detection as PAN-Coordinator.
 *
 * This function handles the detection of PAN-Id Conflict detection
 * in case the node is a PAN Coordinator.
 *
 * @param in_scan Indicates whether node is currently scanning
 */
static void check_for_pan_id_conflict_as_pc(bool in_scan)
{
    /*
     * Check whether the received frame has the PAN Coordinator bit set
     * in the Superframe Specification field of the beacon frame, and
     * whether the received beacon frame has the same PAN-Id as the current
     * network.
     */
    if (GET_PAN_COORDINATOR(mac_parse_data.payload_data.beacon_data.superframe_spec))
    {
        if (
#if ((MAC_SCAN_ACTIVE_REQUEST_CONFIRM == 1) || (MAC_SCAN_PASSIVE_REQUEST_CONFIRM == 1))
            ((!in_scan) && (mac_parse_data.src_panid == tal_pib_PANId)) ||
            (mac_parse_data.src_panid == mac_scan_orig_panid)
#else
            (!in_scan) && (mac_parse_data.src_panid == tal_pib_PANId)
#endif /* ((MAC_SCAN_ACTIVE_REQUEST_CONFIRM == 1) || (MAC_SCAN_PASSIVE_REQUEST_CONFIRM == 1)) */
           )
        {
            mac_sync_loss(MAC_PAN_ID_CONFLICT);
        }
    }
}
#endif  /* (MAC_PAN_ID_CONFLICT_AS_PC == 1) */



#if (MAC_PAN_ID_CONFLICT_NON_PC == 1)
/**
 * @brief PAN-Id conflict detection NOT as PAN-Coordinator.
 *
 * This function handles the detection of PAN-Id Conflict detection
 * in case the node is NOT a PAN Coordinator.
 *
 * @param in_scan Indicates whether node is currently scanning
 */
static void check_for_pan_id_conflict_non_pc(bool in_scan)
{
    /*
     * Check whether the received frame has the PAN Coordinator bit set
     * in the Superframe Specification field of the beacon frame.
     */
    if (GET_PAN_COORDINATOR(mac_parse_data.payload_data.beacon_data.superframe_spec))
    {
        /*
         * The received beacon frame is from a PAN Coordinator
         * (not necessarily ours).
         * Now check if the PAN-Id is ours.
         */
        if (
#if ((MAC_SCAN_ACTIVE_REQUEST_CONFIRM == 1) || (MAC_SCAN_PASSIVE_REQUEST_CONFIRM == 1))
            ((!in_scan) && (mac_parse_data.src_panid == tal_pib_PANId)) ||
            (mac_parse_data.src_panid == mac_scan_orig_panid)
#else
            (!in_scan) && (mac_parse_data.src_panid == tal_pib_PANId)
#endif /* ((MAC_SCAN_ACTIVE_REQUEST_CONFIRM == 1) || (MAC_SCAN_PASSIVE_REQUEST_CONFIRM == 1)) */
           )
        {
            /* This beacon frame has our own PAN-Id.
             * If the address of the source is different from our own
             * parent, a PAN-Id conflict has been detected.
             */
            if (
                (mac_parse_data.src_addr.short_address != mac_pib_macCoordShortAddress) &&
                (mac_parse_data.src_addr.long_address != mac_pib_macCoordExtendedAddress)
               )
            {
                tx_pan_id_conf_notif();
            }
        }
    }
}
#endif /* (MAC_PAN_ID_CONFLICT_NON_PC == 1) */



#if (MAC_PAN_ID_CONFLICT_NON_PC == 1)
/**
 * @brief Sends a PAN-Id conflict detection command frame
 *
 * This function is called in case a device (that is associated to a
 * PAN Coordinator) detects a PAN-Id conflict situation.
 *
 * @return True if the PAN-Id conflict notification is sent successfully,
 *         false otherwise
 */
static bool tx_pan_id_conf_notif(void)
{
    retval_t tal_tx_status;
    buffer_t *pan_id_conf_buffer;

    pan_id_conf_buffer = bmm_buffer_alloc(LARGE_BUFFER_SIZE);

    if (NULL == pan_id_conf_buffer)
    {
        return false;
    }

    frame_info_t *pan_id_conf_frame =
        (frame_info_t *)BMM_BUFFER_POINTER(pan_id_conf_buffer);

    pan_id_conf_frame->frame_ctrl =
        FCF_SET_FRAMETYPE(FCF_FRAMETYPE_MAC_CMD) |
        FCF_ACK_REQUEST |
        FCF_PAN_ID_COMPRESSION |
        FCF_SET_SOURCE_ADDR_MODE(FCF_LONG_ADDR) |
        FCF_SET_DEST_ADDR_MODE(FCF_LONG_ADDR);

        /* Build the Destination address. */
        pan_id_conf_frame->dest_panid = tal_pib_PANId;
        pan_id_conf_frame->dest_address = mac_pib_macCoordExtendedAddress;

        /* Build the Source address. */
        pan_id_conf_frame->src_address = tal_pib_IeeeAddress;

    /* Build the sequence number. */
    pan_id_conf_frame->seq_num = mac_pib_macDSN++;

    /* Update the payload length. */
    pan_id_conf_frame->payload_length = PAN_ID_CONFLICT_PAYLOAD_LEN;

    /* Get the payload pointer. */
    pan_id_conf_frame->payload = (uint8_t *)pan_id_conf_frame +
                                 LARGE_BUFFER_SIZE - FCF_SIZE -
                                 pan_id_conf_frame->payload_length;

    /* Build the command frame id. */
    pan_id_conf_frame->payload[0] = PANIDCONFLICTNOTIFICAION;
    pan_id_conf_frame->msg_type = PANIDCONFLICTNOTIFICAION;

    /*
     * The buffer header is stored as a part of frame_info_t structure before the
     * frame is given to the TAL. After the transmission of the frame, reuse
     * the buffer using this pointer.
     */
    pan_id_conf_frame->buffer_header = pan_id_conf_buffer;

    /* Transmission should be done with CSMA-CA and frame retries. */
#ifdef BEACON_SUPPORT
    /*
     * Now it gets tricky:
     * In Beacon network the frame is sent with slotted CSMA-CA only if:
     * 1) the node is associated, or
     * 2) the node is idle, but synced before association,
     * 3) the node is a Coordinator (we assume, that coordinators are always
     *    in sync with their parents).
     *
     * In all other cases, the frame has to be sent using unslotted CSMA-CA.
     */
    csma_mode_t cur_csma_mode;

    if (NON_BEACON_NWK != tal_pib_BeaconOrder)
    {
        if (
            ((MAC_IDLE == mac_state) && (MAC_SYNC_BEFORE_ASSOC == mac_sync_state)) ||
#if (MAC_START_REQUEST_CONFIRM == 1)
            (MAC_ASSOCIATED == mac_state) ||
            (MAC_COORDINATOR == mac_state)
#else
            (MAC_ASSOCIATED == mac_state)
#endif /* MAC_START_REQUEST_CONFIRM */
           )
        {
            cur_csma_mode = CSMA_SLOTTED;
        }
        else
        {
            cur_csma_mode = CSMA_UNSLOTTED;
        }
    }
    else
    {
        /* In Nonbeacon network the frame is sent with unslotted CSMA-CA. */
        cur_csma_mode = CSMA_UNSLOTTED;
    }

    tal_tx_status = tal_tx_frame(pan_id_conf_frame, cur_csma_mode, true);
#else   /* No BEACON_SUPPORT */
    /* In Nonbeacon build the frame is sent with unslotted CSMA-CA. */
    tal_tx_status = tal_tx_frame(pan_id_conf_frame, CSMA_UNSLOTTED, true);
#endif  /* BEACON_SUPPORT */

    if (MAC_SUCCESS == tal_tx_status)
    {
        MAKE_MAC_BUSY();
        return true;
    }
    else
    {
        /* TAL is busy, hence the data request could not be transmitted */
        bmm_buffer_free(pan_id_conf_buffer);

        return false;
    }
} /* tx_pan_id_conf_notif() */
#endif  /* (MAC_PAN_ID_CONFLICT_NON_PC == 1) */

/* EOF */
