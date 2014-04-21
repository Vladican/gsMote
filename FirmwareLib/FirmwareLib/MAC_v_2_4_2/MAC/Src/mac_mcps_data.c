/**
 * @file mac_mcps_data.c
 *
 * @brief Handles MCPS related primitives and frames
 *
 * This file handles MCPS-DATA requests from the upper layer,
 * generates data frames and initiates its transmission, and
 * processes received data frames.
 *
 * $Id: mac_mcps_data.c 20067 2010-01-28 12:35:18Z sschneid $
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
#if (MAC_START_REQUEST_CONFIRM == 1)
#ifdef BEACON_SUPPORT
#include "broadcast_structures.h"
#endif  /* BEACON_SUPPORT */
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */
#include "stack_config.h"
#include "mac_internal.h"
#include "mac.h"
#include "mac_build_config.h"

/* === Macros =============================================================== */


/* === Globals ============================================================= */


/* === Prototypes ========================================================== */

static bool build_frame_info(mcps_data_req_t *pmdr,
                             uint8_t *buffer,
                             bool indirect);
#if (MAC_INDIRECT_DATA_FFD == 1)
static void mac_buffer_decrement_persistence(void);
static void mac_persistence_buf_expired(buffer_t *msg);
static void mac_t_persistence_cb(void *callback_parameter);
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

/* MAC-internal Buffer functions */
#if (MAC_PURGE_REQUEST_CONFIRM == 1)
static uint8_t check_msdu_handle_cb(void *buf, void *handle);
static bool mac_buffer_purge(uint8_t msdu_handle);
#endif /* (MAC_PURGE_REQUEST_CONFIRM == 1) */

/* === Implementation ====================================================== */

/*
 * @brief Initiates mcps data confirm message
 *
 * This function creates the mcps data confirm structure,
 * and appends it into internal event queue.
 *
 * @param buf Buffer for mcps data confirmation.
 * @param status Data transmission status.
 * @param handle MSDU handle.
 * @param timestamp Time in symbols at which the data were transmitted.
 */
void mac_gen_mcps_data_conf(buffer_t *buf, uint8_t status, uint8_t handle, uint32_t timestamp)
{
    mcps_data_conf_t *mdc = (mcps_data_conf_t *)BMM_BUFFER_POINTER(buf);

    mdc->cmdcode = MCPS_DATA_CONFIRM;
    mdc->msduHandle = handle;
    mdc->status = status;
    mdc->Timestamp = timestamp;

    qmm_queue_append(&mac_nhle_q, buf);
}



/**
 * @brief Builds the data frame for transmission
 *
 * This function builds the data frame for transmission.
 * The NWK layer has supplied the parameters.
 * The frame_info_t data type is constructed and filled in.
 * Also the FCF is constructed based on the parameters passed.
 *
 * @param msg Pointer to the MCPS-DATA.request parameter
 */
void mcps_data_request(uint8_t *msg)
{
    mcps_data_req_t mdr;

    memcpy(&mdr, BMM_BUFFER_POINTER((buffer_t *)msg), sizeof(mcps_data_req_t));

    if (!mdr.TxOptions & WPAN_TXOPT_INDIRECT)
    {
        /*
         * Data Requests for a coordinator using direct transmission are
         * accepted in all non-transient states (no polling and no scanning
         * is ongoing).
         */
        if ((MAC_POLL_IDLE != mac_poll_state) ||
            (MAC_SCAN_IDLE != mac_scan_state)
           )
        {
            mac_gen_mcps_data_conf((buffer_t *)msg,
                                   (uint8_t)MAC_CHANNEL_ACCESS_FAILURE,
                                   mdr.msduHandle,
                                   0);
            return;
        }
    }

#ifndef REDUCED_PARAM_CHECK
    /* Check whether somebody requests an ACK of broadcast frames */
    if ((mdr.TxOptions & WPAN_TXOPT_ACK) &&
        (FCF_SHORT_ADDR == mdr.DstAddrMode) &&
        (BROADCAST == mdr.DstAddr))
    {
        mac_gen_mcps_data_conf((buffer_t *)msg,
                           (uint8_t)MAC_INVALID_PARAMETER,
                           mdr.msduHandle,
                           0);
        return;
    }

    /* Check whether both Src and Dst Address are not present */
    if ((FCF_NO_ADDR == mdr.SrcAddrMode) &&
        (FCF_NO_ADDR == mdr.DstAddrMode))
    {
        mac_gen_mcps_data_conf((buffer_t *)msg,
                               (uint8_t)MAC_INVALID_ADDRESS,
                               mdr.msduHandle,
                               0);
        return;
    }

    /* Check whether Src or Dst Address indicate reserved values */
    if ((FCF_RESERVED_ADDR == mdr.SrcAddrMode) ||
        (FCF_RESERVED_ADDR == mdr.DstAddrMode))
    {
        mac_gen_mcps_data_conf((buffer_t *)msg,
                               (uint8_t)MAC_INVALID_PARAMETER,
                               mdr.msduHandle,
                               0);
        return;
    }
#endif  /* REDUCED_PARAM_CHECK */

    /*
     * Broadcast transmission in a beacon-enabled network intiated by a
     * PAN Coordinator or Coordinator is put into the broadcast queue..
     */
#if (MAC_START_REQUEST_CONFIRM == 1)
#ifdef BEACON_SUPPORT
    if (
        ((MAC_PAN_COORD_STARTED == mac_state) || (MAC_COORDINATOR == mac_state)) &&
        (tal_pib_BeaconOrder < NON_BEACON_NWK) &&
        (FCF_SHORT_ADDR == mdr.DstAddrMode) &&
        (BROADCAST == mdr.DstAddr)
       )
    {
        retval_t queue_status;

        broadcast_t *broadcast_trans_frame =
            (broadcast_t *)BMM_BUFFER_POINTER((buffer_t *)msg);

        broadcast_trans_frame->data = (frame_info_t *)
            (BMM_BUFFER_POINTER((buffer_t *)msg) + sizeof(broadcast_t));

        /* Store the message type */
        broadcast_trans_frame->data->msg_type = MCPS_MESSAGE;
        /* Add the msduHandle in the broadcast buffer */
        broadcast_trans_frame->msduHandle = mdr.msduHandle;

        /* Build the frame */
        if (!build_frame_info(&mdr, (uint8_t *)broadcast_trans_frame, true))
        {
            /* The frame is too long. */
            mac_gen_mcps_data_conf((buffer_t *)msg,
                                   (uint8_t)MAC_INVALID_PARAMETER,
                                   mdr.msduHandle,
                                   0);
            return;
        }

        /* Append the MCPS data request into the broadcast queue */
        queue_status = qmm_queue_append(&broadcast_q, (buffer_t *)msg);

        if (QUEUE_FULL == queue_status)
        {
            mac_gen_mcps_data_conf((buffer_t *)msg,
                                   (uint8_t)MAC_CHANNEL_ACCESS_FAILURE,
                                   mdr.msduHandle,
                                   0);
        }
        return;
    }
#endif  /* BEACON_SUPPORT */
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */

    /*
     * Indirect transmission is only allowed if we are
     * a PAN coordinator or coordinator.
     */
#if (MAC_INDIRECT_DATA_FFD == 1)
    if (
        (mdr.TxOptions & WPAN_TXOPT_INDIRECT) &&
        ((MAC_PAN_COORD_STARTED == mac_state) || (MAC_COORDINATOR == mac_state))
       )
    {
        retval_t queue_status;
        indirect_data_t *indirect_trans_frame =
            (indirect_data_t *)BMM_BUFFER_POINTER((buffer_t *)msg);

        indirect_trans_frame->in_transit = false;
        indirect_trans_frame->data = (frame_info_t *)
            (BMM_BUFFER_POINTER((buffer_t *)msg) + sizeof(indirect_data_t));

        /* Store the message type */
        indirect_trans_frame->data->msg_type = MCPS_MESSAGE;
        /* Add the msduHandle in the indirect data buffer */
        indirect_trans_frame->msduHandle = mdr.msduHandle;

        /* Build the frame */
        if (!build_frame_info(&mdr, (uint8_t *)indirect_trans_frame, true))
        {
            /* The frame is too long. */
            mac_gen_mcps_data_conf((buffer_t *)msg,
                                   (uint8_t)MAC_INVALID_PARAMETER,
                                   mdr.msduHandle,
                                   0);
            return;
        }

#ifdef TEST_HARNESS
        if (mac_pib_privateTransactionOverflow >= 1)
        {
            /*
             * Private PIB is set to indicate no free indirect
             * buffer for new transaction.
             */
            queue_status = QUEUE_FULL;
        }
        else
#endif /* TEST_HARNESS */
        {
            /* Append the MCPS data request into the indirect data queue */
            queue_status = qmm_queue_append(&indirect_data_q, (buffer_t *)msg);
        }

        if (QUEUE_FULL == queue_status)
        {
            mac_gen_mcps_data_conf((buffer_t *)msg,
                                   (uint8_t)MAC_TRANSACTION_OVERFLOW,
                                   mdr.msduHandle,
                                   0);
            return;
        }

        /*
         * If an FFD does have pending data,
         * the MAC persistence timer needs to be started.
         */
        add_persistence_time(msg);
        mac_check_persistence_timer();
    }
    else
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

    /*
     * We are NOT indirect, so we need to transmit using
     * CSMA_CA in the CAP (for beacon enabled) or immediately (for
     * a non-beacon enabled).
     */
    {
        retval_t tal_tx_status;

        frame_info_t *transmit_frame =
            (frame_info_t *)BMM_BUFFER_POINTER((buffer_t *)msg);

        if (!build_frame_info(&mdr, (uint8_t *)transmit_frame, false))
        {
            /* The frame is too long. */
            mac_gen_mcps_data_conf((buffer_t *)msg,
                                   (uint8_t)MAC_INVALID_PARAMETER,
                                   mdr.msduHandle,
                                   0);
            return;
        }

        mac_msdu_handle = mdr.msduHandle;
        transmit_frame->msg_type = MCPS_MESSAGE;

        if (RADIO_SLEEPING == mac_radio_sleep_state)
        {
            mac_trx_wakeup();
        }

        transmit_frame->buffer_header = (buffer_t *)msg;

        /* Transmission should be done with CSMA-CA and with frame retries. */
#ifdef BEACON_SUPPORT
        csma_mode_t cur_csma_mode;

        if (NON_BEACON_NWK == tal_pib_BeaconOrder)
        {
            /* In Nonbeacon network the frame is sent with unslotted CSMA-CA. */
            cur_csma_mode = CSMA_UNSLOTTED;
        }
        else
        {
            /* In Beacon network the frame is sent with slotted CSMA-CA. */
            cur_csma_mode = CSMA_SLOTTED;
        }

        tal_tx_status = tal_tx_frame(transmit_frame, cur_csma_mode, true);
#else   /* No BEACON_SUPPORT */
        /* In Nonbeacon build the frame is sent with unslotted CSMA-CA. */
// LSR
        //tal_tx_status = tal_tx_frame(transmit_frame, CSMA_UNSLOTTED, true);
		if((mdr.TxOptions & WPAN_TXOPT_OFF) != 0)
		{
			tal_tx_status = tal_tx_frame(transmit_frame, NO_CSMA_NO_IFS, false);
			// This disables CSMA (the false parameter sets retries to 7 which disables the CSMA mechanism).		
		}
		else
		{
			tal_tx_status = tal_tx_frame(transmit_frame, CSMA_UNSLOTTED, true);
		}
// ^
#endif  /* BEACON_SUPPORT / No BEACON_SUPPORT */

        if (MAC_SUCCESS == tal_tx_status)
        {
            MAKE_MAC_BUSY();
        }
        else
        {
#if (DEBUG > 1)
            ASSERT("Direct data tx failed" == 0);
#endif
            /* Transmission to TAL failed, generate confirmation message. */
            mac_gen_mcps_data_conf((buffer_t *)msg,
                                   (uint8_t)MAC_CHANNEL_ACCESS_FAILURE,
                                   mdr.msduHandle,
                                   0);

            /* Set radio to sleep if allowed */
            mac_sleep_trans();
        }
    }
} /* mcps_data_request() */



/**
 * @brief Processes data frames
 *
 * This function processes the data frames received and sends
 * mcps_data_indication to the NHLE.
 *
 * @param data_ind Pointer to receive buffer of the data frame
 */
void mac_process_data_frame(buffer_t *data_ind)
{
    mcps_data_ind_t *mdi =
        (mcps_data_ind_t *)BMM_BUFFER_POINTER(data_ind);

    if (mac_parse_data.payload_length == 0)
    {
        /*
         * A null frame is neither indicated to the higher layer
         * nor checked for for frame pending bit set, since
         * null data frames with frame pending bit set are nonsense.
         */
        /* Since no indication is generated, the frame buffer is released. */
        bmm_buffer_free(data_ind);

        /* Set radio to sleep if allowed */
        mac_sleep_trans();
    }
    else
    {
        /* Build the MLME_Data_indication parameters. */
        mdi->DSN = mac_parse_data.sequence_number;
        mdi->Timestamp = mac_parse_data.timestamp;

        /* Source address info */
        mdi->SrcAddrMode = mac_parse_data.src_addr_mode;

        if (FCF_LONG_ADDR == mdi->SrcAddrMode ||
            FCF_SHORT_ADDR == mdi->SrcAddrMode)
        {
            mdi->SrcPANId = mac_parse_data.src_panid;
            ADDR_COPY_DST_SRC_64(mdi->SrcAddr, mac_parse_data.src_addr.long_address);
        }
        else
        {
            /*
             * Even if the Source address mode is zero, and the source address
             * informationis ís not present, the values are cleared to prevent
             * the providing of trash information.
             */
            mdi->SrcPANId = 0;
            mdi->SrcAddr = 0;
        }


        /* Start of duplicate detection. */
        if ((mdi->DSN == mac_last_dsn) &&
            (mdi->SrcAddr == mac_last_src_addr)
           )
        {
            /*
             * This is a duplicated frame.
             * It will not be indicated to the next higher layer,
             * but nevetheless the frame pending bit needs to be
             * checked and acted upon.
             */
            /* Since no indication is generated, the frame buffer is released. */
            bmm_buffer_free(data_ind);
        }
        else
        {
            /* Generate data indication to next higher layer. */

            /* Store required information for perform subsequent
             * duplicate detections.
             */
            mac_last_dsn = mdi->DSN;
            mac_last_src_addr = mdi->SrcAddr;

            /* Destination address info */
            mdi->DstAddrMode = mac_parse_data.dest_addr_mode;

            if (FCF_LONG_ADDR == mdi->DstAddrMode ||
                FCF_SHORT_ADDR == mdi->DstAddrMode)
            {
                mdi->DstPANId = mac_parse_data.dest_panid;
                ADDR_COPY_DST_SRC_64(mdi->DstAddr, mac_parse_data.dest_addr.long_address);
            }
            else
            {
                /*
                 * Even if the Destination address mode is zero, and the destination
                 * address information is ís not present, the values are cleared to
                 * prevent the providing of trash information.
                 */
                mdi->DstPANId = 0;
                mdi->DstAddr = 0;
            }

            mdi->mpduLinkQuality = mac_parse_data.ppduLinkQuality;
            mdi->data[0] = mdi->msduLength = mac_parse_data.payload_length;

            /* Copy the data */
            memcpy(&mdi->data[1],
                   &mac_parse_data.payload_data.data.payload[0],
                   mdi->msduLength);

            mdi->cmdcode = MCPS_DATA_INDICATION;

            /* Append MCPS data indication to MAC-NHLE queue */
            qmm_queue_append(&mac_nhle_q, data_ind);

        }   /* End of duplicate detection. */


        /* Continue with checking the frame pending bit in the received
         * data frame.
         */
#if (MAC_INDIRECT_DATA_BASIC == 1)
        if (mac_parse_data.fcf & FCF_FRAME_PENDING)
        {
#if (MAC_START_REQUEST_CONFIRM == 1)
            /* An node that is not PAN coordinator may poll for pending data. */
            if (MAC_PAN_COORD_STARTED != mac_state)
#endif  /* (MAC_START_REQUEST_CONFIRM == 1) */
            {
                 address_field_t src_addr;

                /* Build command frame due to implicit poll request */
                /*
                 * No explicit destination address attached, so use current
                 * values of PIB attributes macCoordShortAddress or
                 * macCoordExtendedAddress.
                 */
                /*
                 * This implicit poll (i.e. corresponding data request
                 * frame) is to be sent to the same node that we have received
                 * this data frame. Therefore the source address information
                 * from this data frame needs to be extracted, and used for the
                 * data request frame appropriately.
                 * Use this as destination address expclitily and
                 * feed this to the function mac_build_and_tx_data_req
                 */
                if (FCF_SHORT_ADDR == mac_parse_data.src_addr_mode)
                {
                    ADDR_COPY_DST_SRC_16(src_addr.short_address, mac_parse_data.src_addr.short_address);
                    mac_build_and_tx_data_req(false,
                                              false,
                                              FCF_SHORT_ADDR,
                                              (address_field_t *)&(src_addr),
                                              mac_parse_data.src_panid);
                }
                else if (FCF_LONG_ADDR == mac_parse_data.src_addr_mode)
                {
                    ADDR_COPY_DST_SRC_64(src_addr.long_address, mac_parse_data.src_addr.long_address);
                    mac_build_and_tx_data_req(false,
                                              false,
                                              FCF_LONG_ADDR,
                                              (address_field_t *)&(src_addr),
                                              mac_parse_data.src_panid);
                }
                else
                {
                    mac_build_and_tx_data_req(false, false, 0, NULL, 0);
                }
            }
        }
        else
#endif /* (MAC_INDIRECT_DATA_BASIC == 1) */
        {
            /* Frame pending but was not set, so no further action required. */
            /* Set radio to sleep if allowed */
            mac_sleep_trans();
        }   /* if (mac_parse_data.fcf & FCF_FRAME_PENDING) */
    }   /* (mac_parse_data.payload_length == 0) */
} /* mac_process_data_frame() */



/*
 * @brief Builds MCPS data frame
 *
 * This function builds the data frame.
 *
 * @param pmdr Request parameters
 * @param buffer Pointer to transmission frame
 * @param indirect Transmission is direct or indirect
 *
 * @return False if frame length is too long, true otherwise.
 */
static bool build_frame_info(mcps_data_req_t *pmdr,
                             uint8_t *buffer,
                             bool indirect)
{
    bool intrabit = false;
    uint8_t data_frame_len;
    frame_info_t *transmit_frame;

#if (MAC_INDIRECT_DATA_FFD == 1)
    if (indirect)
    {
        transmit_frame = ((indirect_data_t *)buffer)->data;
    }
    else
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */
    {
        transmit_frame = (frame_info_t *)buffer;

        /* Keep compiler happy. */
        indirect = indirect;
    }

    /* Construct FCF first. */
#ifdef TEST_HARNESS
    /*
     * When performing tests this PIB attribute defaults to 1
     * (i.e. a standard data frame). If not set to 1, it is used as a
     * (supposedly illegal) frame type to fill into the frame type
     * field of the data frame's FCF. In effect, valid (illegal)
     * values range from 4 through 7.
     */
    if (mac_pib_privateIllegalFrameType != 1)
    {
        transmit_frame->frame_ctrl =
            FCF_SET_FRAMETYPE(mac_pib_privateIllegalFrameType);
    }
    else
#endif /* TEST_HARNESS */
    {
        transmit_frame->frame_ctrl = FCF_SET_FRAMETYPE(FCF_FRAMETYPE_DATA);
    }

    if (pmdr->TxOptions & WPAN_TXOPT_ACK)
    {
        transmit_frame->frame_ctrl |= FCF_ACK_REQUEST;
    }

    /*
     * 802.15.4-2006 section 7.1.1.1.3:
     *
     * If the msduLength parameter is greater than aMaxMACSafePayloadSize,
     * the MAC sublayer will set the Frame Version subfield of the
     * Frame Control field to one.
     */
    if (pmdr->msduLength > aMaxMACSafePayloadSize)
    {
        transmit_frame->frame_ctrl |= FCF_FRAME_VERSION_2006;
    }

    /*
     * In 802.15.4-2006 the PAN-Id is not present in the MCPS-DATA.request
     * primitive.
     */
    if ((tal_pib_PANId == pmdr->DstPANId) &&
        (FCF_NO_ADDR != pmdr->SrcAddrMode) &&
        (FCF_NO_ADDR != pmdr->DstAddrMode))
    {
        /* Set intra-PAN bit. */
        intrabit = true;
        transmit_frame->frame_ctrl |= FCF_PAN_ID_COMPRESSION;
    }

    /* Set FCFs address mode */
    transmit_frame->frame_ctrl |= FCF_SET_SOURCE_ADDR_MODE(pmdr->SrcAddrMode);
    transmit_frame->frame_ctrl |= FCF_SET_DEST_ADDR_MODE(pmdr->DstAddrMode);

    /* Populate the DSN. */
    transmit_frame->seq_num = mac_pib_macDSN++;

    /*
     * Start counting the Data frame length, in order to check if the frame
     * is not too long.
     */
    data_frame_len = 3; /* FCF + Seq No. */

    /* Build the addressing fields. Destination first. */
    if (FCF_NO_ADDR != pmdr->DstAddrMode)
    {
        transmit_frame->dest_panid = pmdr->DstPANId;
        data_frame_len += 2;
        transmit_frame->dest_address = pmdr->DstAddr;
        if (FCF_SHORT_ADDR == pmdr->DstAddrMode)
        {
            data_frame_len += 2;
        }
        else
        {
            data_frame_len += 8;
        }
    }

    /* Source PANId, if not intra-PAN && source address present */
    if (!intrabit && (FCF_NO_ADDR != pmdr->SrcAddrMode))
    {
        /*
         * In 802.15.4-2006 the PAN-Id is not present in the MCPS-DATA.request
         * primitive.
         */
        transmit_frame->src_panid = tal_pib_PANId;
        data_frame_len += 2;
    }

    /*
     * In 802.15.4-2006 the PAN-Id is not present in the MCPS-DATA.request
     * primitive.
     */
    if (FCF_SHORT_ADDR == pmdr->SrcAddrMode)
    {
        transmit_frame->src_address = tal_pib_ShortAddress;
        data_frame_len += 2;
    }
    else if (FCF_LONG_ADDR == pmdr->SrcAddrMode)
    {
        transmit_frame->src_address = tal_pib_IeeeAddress;
        data_frame_len += 8;
    }

    transmit_frame->payload_length = pmdr->msduLength;
    data_frame_len += pmdr->msduLength + 2; /* Add 2 octets for FCS. */

    /*
     * Payload pointer points to data, which was already copied
     * into buffer
     */
    transmit_frame->payload = buffer +
                              LARGE_BUFFER_SIZE -
                              FCF_SIZE - transmit_frame->payload_length;

    if (data_frame_len > aMaxPHYPacketSize)
    {
        return false;
    }

    return true;
} /* build_frame_info() */



#if (MAC_INDIRECT_DATA_FFD == 1)
/*
 * @brief Start the Persistence timer for indirect data
 *
 * This function starts the persistence timer for handling of indirect
 * data.
 */
void mac_start_persistence_timer(void)
{
    retval_t status = FAILURE;
    /* Interval of indirect data persistence timer */
    uint32_t persistence_int_us;

#ifdef BEACON_SUPPORT
    /*
     * This is a beacon build.
     */
    uint8_t bo_for_persistence_tmr;

    if (tal_pib_BeaconOrder == NON_BEACON_NWK)
    {
        /*
         * The timeout interval for the indirect data persistence timer is
         * based on the define below and is the same as for a nonbeacon build.
         */
        bo_for_persistence_tmr = BO_USED_FOR_MAC_PERS_TIME;
    }
    else
    {
        /*
         * The timeout interval for the indirect data persistence timer is
         * based on the current beacon order.
         */
        bo_for_persistence_tmr = tal_pib_BeaconOrder;
    }

    persistence_int_us =
        TAL_CONVERT_SYMBOLS_TO_US(TAL_GET_BEACON_INTERVAL_TIME(bo_for_persistence_tmr));
#else   /* No BEACON_SUPPORT */
    /*
     * This is a nonbeacon build. The timeout interval for the indirect data
     * persistence timer is based on the define below.
     */
    persistence_int_us =
        TAL_CONVERT_SYMBOLS_TO_US(TAL_GET_BEACON_INTERVAL_TIME(BO_USED_FOR_MAC_PERS_TIME));
#endif  /* BEACON_SUPPORT / No BEACON_SUPPORT */

    /* Start the indirect data persistence timer now. */
    status = pal_timer_start(T_Data_Persistence,
                             persistence_int_us,
                             TIMEOUT_RELATIVE,
                             (FUNC_PTR)mac_t_persistence_cb,
                             NULL);

    if (MAC_SUCCESS != status)
    {
        /* Got to the persistence timer callback function immediately. */
        mac_t_persistence_cb(NULL);
#if (DEBUG > 0)
        ASSERT("Indirect data persistence timer start failed" == 0);
#endif
    }
}
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */



#if (MAC_INDIRECT_DATA_FFD == 1)
/*
 * @brief Handles timeout of indirect data persistence timer
 *
 * This function is a callback function of the timer started for checking
 * the mac persistence time of indirect data in the queue.
 *
 * @param callback_parameter Callback parameter
 */
static void mac_t_persistence_cb(void *callback_parameter)
{
     /* Decrement the persistence time for indirect data. */
     mac_buffer_decrement_persistence();

    if (indirect_data_q.size > 0)
    {
        /* Restart persistence timer. */
        mac_start_persistence_timer();
    }

    callback_parameter = callback_parameter; /* Keep compiler happy. */
}
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */



#if (MAC_INDIRECT_DATA_FFD == 1)
/*
 * @brief Decrements the persistence time of indirect data in indirect queue
 *
 * Decrements the persistance time of each indirect data in the indirect
 * queue. If the persistance time of any indirect data reduces to zero, the
 * confirmation for that indirect data is sent with the status as
 * transaction expired.
 */
static void mac_buffer_decrement_persistence(void)
{
    uint8_t index;

    /*
     * The entire indirect data queue is searched and the persistance time of
     * each indirect data is decremented. If the persistance time of any
     * indirect data becomes zero, then the corresponding status of that
     * indirect data element (mac_indirect_array[index].active) is made false.
     */
    for (index = 0; index < INDIRECT_DATA_QUEUE_CAPACITY; index++)
    {
        if (mac_indirect_array[index].active)
        {
            mac_indirect_array[index].persistence_time--;

            /*
             * If persistence time has expired (reduced to 0),
             * confirmation is called with transaction as status expired.
             */
            if (0 == mac_indirect_array[index].persistence_time)
            {
                mac_indirect_array[index].active = false;
                mac_persistence_buf_expired((buffer_t *)mac_indirect_array[index].buf_ptr);

                mac_indirect_array[index].buf_ptr = NULL;
            }
        }
    }

    rearrange_persistence_array();
}
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */



#if (MAC_INDIRECT_DATA_FFD == 1)
/*
 * @brief Generates notification for expired transaction
 *
 * This function generates the confirmation for those indirect data buffers
 * whose persistence time has reduced to zero.
 *
 * @param msg Pointer to buffer of indirect data whose persistance time
 * has reduced to zero
 */
static void mac_persistence_buf_expired(buffer_t *msg)
{
    search_t find_buf;
    indirect_data_t *frame = (indirect_data_t *)BMM_BUFFER_POINTER(msg);

    frame_info_t *trans_frame = frame->data;

    find_buf.criteria_func = find_buffer_cb;
    find_buf.handle = (void *)msg;
    qmm_queue_remove(&indirect_data_q, &find_buf);

    switch (trans_frame->msg_type)
    {
#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)
        case ASSOCIATIONRESPONSE:
            {
                mac_mlme_comm_status(FCF_LONG_ADDR,
                                     &tal_pib_IeeeAddress,
                                     FCF_LONG_ADDR,
                                     &(trans_frame->dest_address),
                                     MAC_TRANSACTION_EXPIRED, msg);
            }
            break;
#endif  /* (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) */


#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)
        case DISASSOCIATIONNOTIFICATION:
            /*
             * Prepare disassociation confirm message after transmission of
             * the disassociation notification frame.
             */
            mac_prep_disassoc_conf((buffer_t *)msg,
                                   MAC_TRANSACTION_EXPIRED,
                                   trans_frame);
            break;
#endif  /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) */


        case MCPS_MESSAGE:
            {
                mac_gen_mcps_data_conf((buffer_t *)msg,
                                       (uint8_t)MAC_TRANSACTION_EXPIRED,
                                        frame->msduHandle,
                                        0);
            }
            break;

        default:
            ASSERT("Unknown msg type" == 0);
            /* Nothing to be done here. */
            break;
    }
}
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */



#if ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_FFD == 1))
/*
 * @brief Purges a buffer corresponding to a MSDU handle
 *
 * This function tries to purge a given msdu by finding its msdu handle.
 * If the handle is found, that buffer is freed up for further use.
 * This routine will typically be called from the mlme_purge_request routine.
 *
 * @param msdu_handle MSDU handle
 *
 * @return True if the MSDU handle is found in the indirect queue
 *         and removed successfully, false otherwise.
 */
static bool mac_buffer_purge(uint8_t msdu_handle)
{
    uint8_t *buf_ptr;
    search_t find_buf;
    uint8_t handle = msdu_handle;

    /*
     * Callback function  for searching the data having MSDU handle
     * given by purge request
     */
    find_buf.criteria_func = check_msdu_handle_cb;
    /* Update the MSDU handle to be searched */
    find_buf.handle = &handle;

    /* Remove from indirect queue if the short address matches */
    buf_ptr = (uint8_t *)qmm_queue_remove(&indirect_data_q, &find_buf);

    if (NULL != buf_ptr)
    {
        uint8_t index;

        for (index = 0; index < INDIRECT_DATA_QUEUE_CAPACITY; index++)
        {
            if (buf_ptr == mac_indirect_array[index].buf_ptr)
            {
                mac_indirect_array[index].active = false;
                mac_indirect_array[index].buf_ptr = NULL;
                rearrange_persistence_array();
                break;
            }
        }

        /* Free the buffer allocated, after purging */
        bmm_buffer_free((buffer_t *)buf_ptr);

        return true;
    }

    /* No data available in the indirect queue with MSDU handle provided */
    return false;
}



/**
 * @brief Processes a MCPS-PURGE.request primitive
 *
 * This functions processes a MCPS-PURGE.request from the NHLE.
 * The MCPS-PURGE.request primitive allows the next higher layer
 * to purge an MSDU from the transaction queue.
 * On receipt of the MCPS-PURGE.request primitive, the MAC sublayer
 * attempts to find in its transaction queue the MSDU indicated by the
 * msduHandle parameter. If an MSDU matching the given handle is found,
 * the MSDU is discarded from the transaction queue, and the MAC
 * sublayer issues the MCPSPURGE. confirm primitive with a status of
 * MAC_SUCCESS. If an MSDU matching the given handle is not found, the MAC
 * sublayer issues the MCPS-PURGE.confirm primitive with a status of
 * INVALID_HANDLE.
 *
 * @param msg Pointer to the MCPS-PURGE.request parameter
 */
void mcps_purge_request(uint8_t *msg)
{
    mcps_purge_req_t *mpr =
        (mcps_purge_req_t *)BMM_BUFFER_POINTER(((buffer_t *)msg));

    mcps_purge_conf_t *mpc = (mcps_purge_conf_t *)mpr;

    uint8_t purge_handle = mpr->msduHandle;

    /* Update the purge confirm structure */
    mpc->cmdcode = MCPS_PURGE_CONFIRM;
    mpc->msduHandle = purge_handle;

    if (mac_buffer_purge(purge_handle))
    {
        mpc->status = MAC_SUCCESS;
    }
    else
    {
        mpc->status = MAC_INVALID_HANDLE;
    }

    qmm_queue_append(&mac_nhle_q, (buffer_t *)msg);
}



/*
 * @brief Checks whether MSDU handle matches
 *
 * @param buf Pointer to indirect data buffer
 * @param handle MSDU handle to be searched
 *
 * @return 1 if MSDU handle matches with the indirect data, 0 otherwise
 */
static uint8_t check_msdu_handle_cb(void *buf, void *handle)
{
    indirect_data_t *indirect_frame = (indirect_data_t *)buf;
    uint8_t msdu;

    msdu = *((uint8_t *)handle);

    /* Compare the MSDU handle */
    if (indirect_frame->msduHandle == msdu)
    {
        return 1;
    }
    return 0;
}
#endif /* ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_FFD == 1)) */
/* EOF */
