/**
 * @file mac_process_tal_tx_frame_status.c
 *
 * @brief Processes the TAL tx frame status received on the frame transmission.
 *
 * $Id: mac_process_tal_tx_frame_status.c 20200 2010-02-08 09:31:43Z sschneid $
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
 * Position of the ack in the transmitted frame
 */
#define ACK_POSITION                    (5)

/* === Globals ============================================================= */


/* === Prototypes ========================================================== */

static void mac_process_tal_tx_status(bool has_pending_bit, frame_info_t *frame);
static void select_response(retval_t tx_status, frame_info_t *frame);

#if (MAC_INDIRECT_DATA_FFD == 1)
static void remove_frame_from_indirect_q(frame_info_t *f_ptr);
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

/* === Implementation ====================================================== */

/*
 * @brief Process tal_tx_frame_done_cb status
 *
 * This function is called, if an ACK is requested in the last transmitted frame.
 * According to the frame type that has previously been sent, the
 * corresponding actions are taken and the MAC returns to its standard state.
 *
 * @param has_pending_bit Indicates the pending bit was set in the ACK
 * frame which was received by TAL for the last transmitted frame
 * @param frame Pointer to the transmitted frame
 */
static void mac_process_tal_tx_status(bool has_pending_bit, frame_info_t *frame)
{
    switch (frame->msg_type)
    {
        case MCPS_MESSAGE:
#if (MAC_INDIRECT_DATA_FFD == 1)
            if (mac_indirect_flag)
            {
                remove_frame_from_indirect_q(frame);
            }
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

            /* Create the MCPS DATA confirmation message */
            {
                buffer_t *mcps_buf = frame->buffer_header;

                mac_gen_mcps_data_conf((buffer_t *)mcps_buf,
                                       (uint8_t)MAC_SUCCESS,
                                       mac_msdu_handle,
                                       frame->time_stamp);
            }

            /* Set radio to sleep if allowed */
            mac_sleep_trans();
            break;


#if (MAC_INDIRECT_DATA_FFD == 1)
        case NULL_FRAME:
            /* Free the buffer allocated for the Null data frame */
            bmm_buffer_free(frame->buffer_header);

            /* Set radio to sleep if allowed */
            mac_sleep_trans();
            break;
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */


#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)
        case DISASSOCIATIONNOTIFICATION:
#if (MAC_DISASSOCIATION_FFD_SUPPORT == 1)
            if (mac_indirect_flag)
            {
                remove_frame_from_indirect_q(frame);
            }
#endif  /* (MAC_DISASSOCIATION_FFD_SUPPORT == 1) */

            /* Create the MLME DISASSOCIATION confirmation message */
            {
                buffer_t *disassoc_buf = frame->buffer_header;

                /*
                 * Prepare disassociation confirm message after transmission of
                 * the disassociation notification frame.
                 */
                mac_prep_disassoc_conf((buffer_t *)disassoc_buf,
                                       MAC_SUCCESS,
                                       frame);
            }

            /*
             * Only an associated device should go to idle on transmission of a
             * disassociation frame.
             */
            if (MAC_ASSOCIATED == mac_state)
            {
                /*
                 * Entering sleep mode is already done implicitly in
                 * mac_idle_trans().
                 */
                mac_idle_trans();
            }

            /* Set radio to sleep if allowed */
            mac_sleep_trans();
            break;
#endif  /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) */


#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1)
        case ASSOCIATIONREQUEST:
#ifdef TEST_HARNESS
            /*
             * For purely testing purposes the coordinator may be set
             * into a state where it will never respond with an
             * Assoc Response frame
             */
            if (mac_pib_privateNoDataAfterAssocReq >= 1)
            {
                bmm_buffer_free(frame->buffer_header);

                break;
            }
#endif /* TEST_HARNESS */

            mac_poll_state = MAC_AWAIT_ASSOC_RESPONSE;

            {
                uint8_t status;
                uint32_t response_timer;

                /* Start the response wait timer */
                response_timer = mac_pib_macResponseWaitTime;
                response_timer = TAL_CONVERT_SYMBOLS_TO_US(response_timer);

                status = pal_timer_start(T_Poll_Wait_Time,
                                         response_timer, TIMEOUT_RELATIVE,
                                         (FUNC_PTR)mac_t_response_wait_cb,
                                         NULL);

#if (DEBUG > 0)
                ASSERT(MAC_SUCCESS == status);
#endif
                if (MAC_SUCCESS != status)
                {
                    mac_t_response_wait_cb(NULL);
                }
            }
            break;
#endif /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */

#if (MAC_INDIRECT_DATA_BASIC == 1)
        case DATAREQUEST:           /* Explicit poll caused by MLME-POLL.request */
        case DATAREQUEST_IMPL_POLL: /* Implicit poll without MLME-POLL.request */
            {
                /* Free the data_request buffer */
                bmm_buffer_free(frame->buffer_header);

                /*
                 * In case we are in the middle of an association procedure,
                 * we nothing here, but keep in the same state.
                 * Also the poll timer is NOT canceled here, because it acts
                 * as a timer for the entire association procedure.
                 */
                if (MAC_AWAIT_ASSOC_RESPONSE != mac_poll_state)
                {
                    if (DATAREQUEST == frame->msg_type)
                    {
                         /* Explicit poll caused by MLME-POLL.request */
                        if (!has_pending_bit)
                        {
                            /* Reuse the poll request buffer for poll confirmation */
                            mlme_poll_conf_t *mpc = (mlme_poll_conf_t *)BMM_BUFFER_POINTER
                                                    ((buffer_t *)mac_conf_buf_ptr);

                            mpc->cmdcode = MLME_POLL_CONFIRM;
                            mpc->status = MAC_NO_DATA;
                            qmm_queue_append(&mac_nhle_q,
                                             (buffer_t *)mac_conf_buf_ptr);

                            /* Set radio to sleep if allowed */
                            mac_sleep_trans();
                            return;
                        }

                        /* Wait for data reception */
                        mac_poll_state = MAC_POLL_EXPLICIT;
                    }
                    else
                    {
                        /* Implicit poll without request */
                        mac_poll_state = MAC_POLL_IMPLICIT;
                    }

                    {
                        uint8_t status;
                        uint32_t response_timer;
                        /*
                         * Start T_Poll_Wait_Time only if we are not in the
                         * middle of an association.
                         */
                        response_timer = mac_pib_macMaxFrameTotalWaitTime;
                        response_timer = TAL_CONVERT_SYMBOLS_TO_US(response_timer);

                        status = pal_timer_start(T_Poll_Wait_Time,
                                                 response_timer,
                                                 TIMEOUT_RELATIVE,
                                                 (FUNC_PTR)mac_t_poll_wait_time_cb,
                                                 NULL);

                        /*
                         * If we are waiting for a pending frame,
                         * the MAC needs to remain busy.
                         */
                        MAKE_MAC_BUSY();

                        if (MAC_SUCCESS != status)
                        {
                            mac_t_poll_wait_time_cb(NULL);
                        }
                    }
                }   /* (MAC_AWAIT_ASSOC_RESPONSE != mac_poll_state) */
            }
            break;
#endif /* (MAC_INDIRECT_DATA_BASIC == 1) */

#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)
        case ASSOCIATIONRESPONSE:
            {
                /* Association resonse frames are always sent indirectly. */
                remove_frame_from_indirect_q(frame);

                mac_mlme_comm_status(FCF_LONG_ADDR,
                                     &tal_pib_IeeeAddress,
                                     FCF_LONG_ADDR,
                                     &(frame->dest_address),
                                     MAC_SUCCESS,
                                     frame->buffer_header);
            }
            break;
#endif  /* (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) */

#if (MAC_ORPHAN_INDICATION_RESPONSE == 1)
        case ORPHANREALIGNMENT:
            {
                mac_mlme_comm_status(FCF_LONG_ADDR,
                                     &tal_pib_IeeeAddress,
                                     FCF_LONG_ADDR,
                                     &(frame->dest_address),
                                     MAC_SUCCESS,
                                     frame->buffer_header);
                }
            break;
#endif  /* (MAC_ORPHAN_INDICATION_RESPONSE == 1) */

#if (MAC_PAN_ID_CONFLICT_NON_PC == 1)
        case PANIDCONFLICTNOTIFICAION:
            /* Free the buffer allocated for the Pan-Id conflict notification frame */
            bmm_buffer_free(frame->buffer_header);

            /* Generate a sync loss to the higher layer. */
            mac_sync_loss(MAC_PAN_ID_CONFLICT);

            /* Set radio to sleep if allowed */
            mac_sleep_trans();
            break;
#endif  /* (MAC_PAN_ID_CONFLICT_NON_PC == 1) */

        default:
#if (DEBUG > 0)
            ASSERT("Unknown message type" == 0);
#endif
            return;
    }

    has_pending_bit = has_pending_bit; /* Keep compiler happy. */
} /* mac_process_tal_tx_status() */



/*
 * @brief Selects a response based on the type of the transmitted frame
 *
 * This function selects a response based on the type of the transmitted frame.
 * This is based on the SDL's select_response procedure.
 *
 * @param tx_status Status of transmission
 * @param frame Pointer to the transmitted frame
 */
static void select_response(retval_t tx_status, frame_info_t *frame)
{
    switch (frame->msg_type)
    {
#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1)
        case ASSOCIATIONREQUEST:
            {
                buffer_t *assoc_buf = frame->buffer_header;

                /* This short address will only be used if the association
                 * attempt was unsuccessful. 7.1.3.4.1 mandates it to be
                 * 0xFFFF in that case. On a successful association, the
                 * actual value will be filled in based on the response to the
                 * data request obtained from the coordinator.
                 */
                mac_gen_mlme_associate_conf((buffer_t *)assoc_buf, tx_status, BROADCAST);
            }
            break;
#endif /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */


#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)
        case ASSOCIATIONRESPONSE:
            {
                /* Association resonse frames are always sent indirectly. */
                /*
                 * The removal of this frame from the indirect queue needs
                 * to be done BEFORE the subsequent call of function
                 * mac_mlme_comm_status(), since variable frame is reused
                 * for the comm status indication buffer and would invalid
                 * afterwards.
                 */
                remove_frame_from_indirect_q(frame);

                mac_mlme_comm_status(FCF_LONG_ADDR,
                                     &tal_pib_IeeeAddress,
                                     FCF_LONG_ADDR,
                                     &(frame->dest_address),
                                     tx_status,
                                     frame->buffer_header);
                }
            break;
#endif  /* (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) */


#if (MAC_ORPHAN_INDICATION_RESPONSE == 1)
        case ORPHANREALIGNMENT:
            {
                /* Coord realignment was sent as result of an orphan scan */
                mac_mlme_comm_status(FCF_LONG_ADDR,
                                     &tal_pib_IeeeAddress,
                                     FCF_LONG_ADDR,
                                     &(frame->dest_address),
                                     tx_status,
                                     frame->buffer_header);
            }
            break;
#endif  /* (MAC_ORPHAN_INDICATION_RESPONSE == 1) */


#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)
        case DISASSOCIATIONNOTIFICATION:
            {
#if (MAC_DISASSOCIATION_FFD_SUPPORT == 1)
                if (mac_indirect_flag)
                {
                    /* The frame was sent indirectly. */
                    remove_frame_from_indirect_q(frame);
                }
#endif  /* (MAC_DISASSOCIATION_FFD_SUPPORT == 1) */

                buffer_t *disassoc_buf = frame->buffer_header;

                /*
                 * Prepare disassociation confirm message after transmission of
                 * the disassociation notification frame.
                 */
                mac_prep_disassoc_conf((buffer_t *)disassoc_buf,
                                       tx_status,
                                       frame);
            }
            break;
#endif  /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) */

#if (MAC_INDIRECT_DATA_BASIC == 1)
        case DATAREQUEST:
            /* Free the data request buffer */
            bmm_buffer_free(frame->buffer_header);

            /* Create the Poll Confirmation message */
            {
                mlme_poll_conf_t *mpc;

                /* Reuse the poll request buffer for poll confirmation */
                mpc = (mlme_poll_conf_t *)BMM_BUFFER_POINTER((buffer_t *)mac_conf_buf_ptr);

                mpc->cmdcode = MLME_POLL_CONFIRM;
                mpc->status = tx_status;

                qmm_queue_append(&mac_nhle_q, (buffer_t *)mac_conf_buf_ptr);
            }
            break;
#endif /* (MAC_INDIRECT_DATA_BASIC == 1) */

        case MCPS_MESSAGE:
            {
#if (MAC_INDIRECT_DATA_FFD == 1)
                if (mac_indirect_flag)
                {
                    /* The frame was sent indirectly. */
                    remove_frame_from_indirect_q(frame);
                }
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

                buffer_t *mcps_buf = frame->buffer_header;

                /* Create the MCPS DATA confirmation message for device */
                mac_gen_mcps_data_conf((buffer_t *)mcps_buf,
                                       (uint8_t)tx_status,
                                       mac_msdu_handle,
                                       frame->time_stamp);
            }
            break;

#if (MAC_INDIRECT_DATA_BASIC == 1)
        case DATAREQUEST_IMPL_POLL:
            /* Free the data request buffer */
            bmm_buffer_free(frame->buffer_header);
            break;

        case NULL_FRAME:
            bmm_buffer_free(frame->buffer_header);
            break;
#endif /* (MAC_INDIRECT_DATA_BASIC == 1) */

#if (MAC_START_REQUEST_CONFIRM == 1)
        case COORDINATORREALIGNMENT:
            /*
             * The coordinator realignment command has been sent out
             * successfully. Hence the MAC should be updated with the new
             * parameters given in the MLME.START_request with
             * coordinator realignment command
             */
            mac_coord_realignment_command_tx_success(tx_status,
                                                     frame->buffer_header);
            break;
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */

#if (MAC_START_REQUEST_CONFIRM == 1)
        case BEACON_MESSAGE:
#ifndef BEACON_SUPPORT
            /*
             * The code for freeing up the buffer is only included for a
             * build without beacon support, since in this case the static
             * beacon buffer is NOT used, but a standard buffer is used
             * instead, which needs to be freed up.
             */
            bmm_buffer_free(frame->buffer_header);
#endif  /* No BEACON_SUPPORT */
            break;
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */

#if (MAC_PAN_ID_CONFLICT_NON_PC == 1)
        case PANIDCONFLICTNOTIFICAION:
            /* Free the buffer allocated for the Pan-Id conflict notification frame */
            bmm_buffer_free(frame->buffer_header);

            /* Set radio to sleep if allowed */
            mac_sleep_trans();
            break;
#endif  /* (MAC_PAN_ID_CONFLICT_NON_PC == 1) */

        default:
#if (DEBUG > 0)
            ASSERT("Unexpected msg type in select_response()" == 0);
#endif
            break;
    }

    /* Set radio to sleep if allowed */
    mac_sleep_trans();
} /* select_response() */



/**
 * @brief Callback function from TAL after the frame is transmitted
 *
 * This is a callback function from the TAL. It is used when an attempt
 * to transmit a frame is finished.
 *
 * @param status Status of transmission
 * @param frame Specifies pointer to the transmitted frame
 */
void tal_tx_frame_done_cb(retval_t status, frame_info_t *frame)
{
    /* Frame transmission completed, set dispatcher to not busy */
    MAKE_MAC_NOT_BUSY();

    /* If ack requested and ack received */
    if ((frame->frame_ctrl & (1UL << ACK_POSITION)) &&
        ((MAC_NO_ACK != status) && (MAC_CHANNEL_ACCESS_FAILURE != status)))
    {
        /* Frame Pending bit is set to true on received ack */
        if (TAL_FRAME_PENDING == status)
        {
            /* Frame pending */
            mac_process_tal_tx_status(true, frame);
        }
        else
        {
            mac_process_tal_tx_status(false, frame);
        }
    }
#if ((MAC_SCAN_ACTIVE_REQUEST_CONFIRM == 1) || (MAC_SCAN_ORPHAN_REQUEST_CONFIRM == 1))
    /* If ack requested and ack not received, or ack not requested */
    else if ((MAC_SCAN_ACTIVE == mac_scan_state) ||
             (MAC_SCAN_ORPHAN == mac_scan_state)
            )
    {
        mac_scan_send_complete(status);
    }
#endif /* ((MAC_SCAN_ACTIVE_REQUEST_CONFIRM == 1) || (MAC_SCAN_ORPHAN_REQUEST_CONFIRM == 1)) */
    else
    {
        select_response(status, frame);
    }
}



#if (MAC_INDIRECT_DATA_FFD == 1)
/**
 * @brief Helper function to remove transmitted indirect data from the queue
 *
 * @param f_ptr Pointer to frame_info_t structure of previously transmitted frame
 */
static void remove_frame_from_indirect_q(frame_info_t *f_ptr)
{
    uint8_t index;
    search_t find_buf;

    find_buf.criteria_func = find_buffer_cb;

    /* Update the address to be searched */
    find_buf.handle = (void *)f_ptr->buffer_header;

    qmm_queue_remove(&indirect_data_q, &find_buf);

    for (index = 0; index < INDIRECT_DATA_QUEUE_CAPACITY; index++)
    {
        /*
         * Remove the persistence timer for the data, which is
         * successfully transmitted
         */
        if ((uint8_t *)f_ptr->buffer_header ==
            mac_indirect_array[index].buf_ptr)
        {
            mac_indirect_array[index].active = false;
            mac_indirect_array[index].buf_ptr = NULL;
            rearrange_persistence_array();
            break;
        }

    }
    mac_indirect_flag = false;
}
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

/* EOF */
