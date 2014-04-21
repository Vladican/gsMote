/**
 * @file mac_data_req.c
 *
 * @brief Implements data request related functions
 *
 * This file implements generation, transmission, reception of data
 * request frames and transmits indirect data frame.
 *
 * $Id: mac_data_req.c 18827 2009-10-23 12:40:53Z sschneid $
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
#include "mac_build_config.h"

#if (MAC_INDIRECT_DATA_BASIC == 1)

/* === Macros =============================================================== */

/*
 * Data request payload length
 */
#define DATA_REQ_PAYLOAD_LEN            (1)

/* === Globals ============================================================= */


/* === Prototypes ========================================================== */

#if (MAC_INDIRECT_DATA_FFD == 1)
static buffer_t *build_null_data_frame(void);
static uint8_t find_long_buffer_cb(void *buf, void *long_addr);
static uint8_t find_short_buffer_cb(void *buf, void *short_addr);
#endif  /*  (MAC_INDIRECT_DATA_FFD == 1)*/

/* === Implementation ====================================================== */

/**
 * @brief Build and transmits data request command frame
 *
 * This function builds and tranmits a data request command frame.
 *
 *
 * @param expl_poll Data request due to explicit MLME poll request
 * @param force_own_long_addr Forces the usage of the Extended Address as
 *                            Source Address. This a allows for implicitly
 *                            poll for pending data at the coordinator if
 *                            the Extended Address was used in the Beacon frame.
 * @param expl_dest_addr_mode Mode of subsequent destination address to be used
 *                            explicitly (0/2/3).
 *                            0: No explicit destination address attached,
 *                               use either macCoordShortAddress or
 *                               macCoordExtendedAddress
 *                            2: Use explicitly attached address in parameter
 *                               expl_dest_addr as destination address as
 *                               short address
 *                            3: Use explicitly attached address in parameter
 *                               expl_dest_addr as destination address as
 *                               extended address
 * @param expl_dest_addr Explicitly attached destination address for data
 *                       request frame. This is to be treated as either not
 *                       present, short or extended address, depending on
 *                       parameter expl_dest_addr_mode.
 * @param expl_dest_pan_id Explicitly attached destination PAN-Id (Coordinator
 *                         PAN-Id) for data request frame.
 *                         This is to be treated only as present, depending on
 *                         parameter expl_dest_addr_mode.
 *
 * @return True if data request command frame was created and sent to
 *         the TAL successfully, false otherwise.
 */
bool mac_build_and_tx_data_req(bool expl_poll,
                               bool force_own_long_addr,
                               uint8_t expl_dest_addr_mode,
                               address_field_t *expl_dest_addr,
                               uint16_t expl_dest_pan_id)
{
    retval_t tal_tx_status;
    buffer_t *data_req_buffer;

    data_req_buffer = bmm_buffer_alloc(LARGE_BUFFER_SIZE);

    if (NULL == data_req_buffer)
    {
        return false;
    }

    frame_info_t *data_req_frame =
        (frame_info_t *)BMM_BUFFER_POINTER(data_req_buffer);

    /*
     * Long address needs to be used if a short address is not present
     * or if we are forced to use the long address.
     *
     * This is used for example in cases where the coordinator indicates
     * pending data for us using our extended address.
     *
     * This is also used for transmitting a data request frame
     * during association, since here we always need to use our
     * extended address.
     */
    if ((BROADCAST == tal_pib_ShortAddress) ||
        (MAC_NO_SHORT_ADDR_VALUE == tal_pib_ShortAddress) ||
        force_own_long_addr)
    {
        data_req_frame->frame_ctrl =
            FCF_SET_FRAMETYPE(FCF_FRAMETYPE_MAC_CMD) |
            FCF_SET_SOURCE_ADDR_MODE(FCF_LONG_ADDR);

        /* Build the Source address. */
        data_req_frame->src_address = tal_pib_IeeeAddress;
    }
    else
    {
        data_req_frame->frame_ctrl =
            FCF_SET_FRAMETYPE(FCF_FRAMETYPE_MAC_CMD) |
            FCF_SET_SOURCE_ADDR_MODE(FCF_SHORT_ADDR);

        /* Build the Source address. */
        data_req_frame->src_address = tal_pib_ShortAddress;
    }

    data_req_frame->frame_ctrl |= FCF_ACK_REQUEST;

    /*
     * In IEEE 802.15.4 the PAN ID Compression bit may always be set.
     * See page 154:
     * If the data request command is being sent in response to the receipt
     * of a beacon frame indicating that data are pending for that device,
     * the Destination Addressing Mode subfield of the Frame Control field
     * may be set to zero ..."
     * In order to keep the implementation simple the address info is also in
     * this case 2 or 3, i.e. the destination address info is present.
     * This in return means that the PAN ID Compression bit is always set for
     * data request frames, except the expl_dest_pan_id parameter is different from
     * our own PAN-Id PIB attribute.
     */
     if ((expl_dest_addr_mode != FCF_NO_ADDR) &&
         (expl_dest_pan_id != tal_pib_PANId)        )
     {
        /*
         * There is an expclicit destination address present AND
         * the destination PAN-Id is different from our own PAN-ID,
         * so include the source PAN-id into the frame.
         */
        data_req_frame->src_panid = tal_pib_PANId;
        data_req_frame->dest_panid = expl_dest_pan_id;
     }
     else
     {
        data_req_frame->frame_ctrl |= FCF_PAN_ID_COMPRESSION;
        data_req_frame->dest_panid = tal_pib_PANId;
     }


    /*
     * The source PAN Id is not present since the PAN ID
     * Compression bit is set.
     */

    /* The destination address information is present. */
    if (FCF_SHORT_ADDR == expl_dest_addr_mode)
    {
        /* An explicit short destination address is requested. */
        data_req_frame->frame_ctrl |= FCF_SET_DEST_ADDR_MODE(FCF_SHORT_ADDR);
        ADDR_COPY_DST_SRC_16(data_req_frame->dest_address, expl_dest_addr->short_address);
    }
    else if (FCF_LONG_ADDR == expl_dest_addr_mode)
    {
        /* An explicit long destination address is requested. */
        data_req_frame->frame_ctrl |= FCF_SET_DEST_ADDR_MODE(FCF_LONG_ADDR);
        ADDR_COPY_DST_SRC_64(data_req_frame->dest_address, expl_dest_addr->long_address);
    }
    else
    {
        /* No explicit destination address is requested. */
        if (MAC_NO_SHORT_ADDR_VALUE != mac_pib_macCoordShortAddress)
        {
            /*
             * If current value of short address for coordinator PIB is
             * NOT 0xFFFE, the current value of the short address for
             * coordinator shall be used as desination address.
             */
            data_req_frame->frame_ctrl |= FCF_SET_DEST_ADDR_MODE(FCF_SHORT_ADDR);
            data_req_frame->dest_address = mac_pib_macCoordShortAddress;
        }
        else
        {
            /*
             * If current value of short address for coordinator PIB is 0xFFFE,
             * the current value of the extended address for coordinator
             * shall be used as desination address.
             */
            data_req_frame->frame_ctrl |= FCF_SET_DEST_ADDR_MODE(FCF_LONG_ADDR);
            data_req_frame->dest_address = mac_pib_macCoordExtendedAddress;
        }
    }

    /* Build the sequence number. */
    data_req_frame->seq_num = mac_pib_macDSN++;

    /* Update the payload length. */
    data_req_frame->payload_length = DATA_REQ_PAYLOAD_LEN;

    /* Get the payload pointer. */
    data_req_frame->payload = (uint8_t *)data_req_frame +
                              LARGE_BUFFER_SIZE - FCF_SIZE -
                              data_req_frame->payload_length;

    /* Build the command frame id. */
    data_req_frame->payload[0] = DATAREQUEST;

    /*
     * If this data request cmd frame was initiated by a device due to implicit
     * poll, set msgtype to DATAREQUEST_IMPL_POLL.
     * If this data request cmd frame was initiated by a MLME poll request,
     * set msgtype to DATAREQUEST.
     */
    if (expl_poll)
    {
        data_req_frame->msg_type = DATAREQUEST;
    }
    else
    {
        data_req_frame->msg_type = DATAREQUEST_IMPL_POLL;
    }

    /*
     * The buffer header is stored as a part of frame_info_t structure before the
     * frame is given to the TAL. After the transmission of the frame, reuse
     * the buffer using this pointer.
     */
    data_req_frame->buffer_header = data_req_buffer;

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

    tal_tx_status = tal_tx_frame(data_req_frame, cur_csma_mode, true);
#else   /* No BEACON_SUPPORT */
    /* In Nonbeacon build the frame is sent with unslotted CSMA-CA. */
    tal_tx_status = tal_tx_frame(data_req_frame, CSMA_UNSLOTTED, true);
#endif  /* BEACON_SUPPORT */

    if (MAC_SUCCESS == tal_tx_status)
    {
        MAKE_MAC_BUSY();
        return true;
    }
    else
    {
        /* TAL is busy, hence the data request could not be transmitted */
        bmm_buffer_free(data_req_buffer);

#if (DEBUG > 1)
        ASSERT("Data req failed" == 0);
#endif
        return false;
    }
} /* mac_build_and_tx_data_req() */



#if (MAC_INDIRECT_DATA_FFD == 1)
/*
 * @brief Constructs a null data frame
 *
 * @return Pointer to the created null data frame, NULL otherwise.
 */
static buffer_t *build_null_data_frame(void)
{
    buffer_t *buf_ptr;
    bool use_long_addr;
    frame_info_t *transmit_frame;

    buf_ptr = bmm_buffer_alloc(LARGE_BUFFER_SIZE);

    if (NULL == buf_ptr)
    {
        return NULL;
    }

    transmit_frame = (frame_info_t *)BMM_BUFFER_POINTER(buf_ptr);

    use_long_addr = (FCF_LONG_ADDR == mac_parse_data.src_addr_mode);

    /* Build the FCF. */
    transmit_frame->frame_ctrl = FCF_SET_FRAMETYPE(FCF_FRAMETYPE_DATA);

    if (use_long_addr)
    {
        transmit_frame->frame_ctrl |= FCF_SET_DEST_ADDR_MODE(FCF_LONG_ADDR);
        /* Destination address is set from source address of received frame. */
        ADDR_COPY_DST_SRC_64(transmit_frame->dest_address, mac_parse_data.src_addr.long_address);
    }
    else
    {
        transmit_frame->frame_ctrl |= FCF_SET_DEST_ADDR_MODE(FCF_SHORT_ADDR);
        /* Destination address is set from source address of received frame. */
        ADDR_COPY_DST_SRC_16(transmit_frame->dest_address, mac_parse_data.src_addr.short_address);
    }

    /* Sequence number. */
    transmit_frame->seq_num = mac_pib_macDSN++;

    /* Destination PANId is set from source PANId of received frame. */
    transmit_frame->dest_panid = mac_parse_data.src_panid;

    /* End MHR */

    /* No src addressing information. */
    /* No data payload, this is a null packet.*/

    transmit_frame->msg_type = NULL_FRAME;

    /* Null Data frame has no payload. */
    transmit_frame->payload_length = 0;

    /*
     * Although the payload is not present for a Null Data frame, the payload pointer
     * is still updated, as the TAL requires this while building the frame.
     */
    transmit_frame->payload = (uint8_t *)transmit_frame +
                              LARGE_BUFFER_SIZE - FCF_SIZE -
                              transmit_frame->payload_length;

    return buf_ptr;
} /* build_null_data_frame() */



/**
 * @brief Processes a received data request command frame
 *
 * This function processes a received data request command frame
 * at the coordinator, searches for pending indirect data frames
 * for the originator and initiates the frame transmission of the
 * data frame with CSMA-CA.
 *
 * @param msg Frame reception buffer pointer
 */
void mac_process_data_request(buffer_t *msg)
{
    buffer_t *buf_ptr;
    search_t find_buf;
    frame_info_t *transmit_frame;
    indirect_data_t *indirect_frame;
    retval_t tal_tx_status;

    mac_indirect_flag = false;

    /* Free the buffer of the received frame. */
    bmm_buffer_free(msg);

    /* Ignore data request if we are not PAN coordinator or coordinator. */
    if ((MAC_IDLE == mac_state) ||
        (MAC_ASSOCIATED == mac_state)
       )
    {
#if (DEBUG > 1)
        ASSERT("Neither PAN coordinator nor coordinator" == 0);
#endif
        return;
    }

    /* Check the addressing mode */
    switch (mac_parse_data.src_addr_mode)
    {
        case FCF_SHORT_ADDR:
            /*
             * Look for pending data in the indirect queue for
             * this short address.
             */

            /*
             * Assign the function pointer for searching the
             * data having address of the requested device.
             */
            find_buf.criteria_func = find_short_buffer_cb;

            /* Update the short address to be searched. */
            find_buf.handle = &mac_parse_data.src_addr.short_address;
            break;

        case FCF_LONG_ADDR:
            /*
             * Look for pending data in the indirect queue for
             * this long address.
             */

            /*
             * Assign the function pointer for searching the
             * data having address of the requested device.
             */
            find_buf.criteria_func = find_long_buffer_cb;

            /* Update the long address to be searched. */
            find_buf.handle = &mac_parse_data.src_addr.long_address;
            break;


        default:
#if (DEBUG > 1)
            ASSERT("Unexpected addressing mode" == 0);
#endif
            return;
    }

    /*
     * Read from the indirect queue. The removal of items from this queue
     * will be done after successful transmission of the frame.
     */
    buf_ptr = qmm_queue_read(&indirect_data_q, &find_buf);

    if (NULL == buf_ptr)
    {
        mac_handle_tx_null_data_frame();
        return;
    }
    else
    {
        /* Indirect data found and to be sent. */
        indirect_frame = (indirect_data_t *)BMM_BUFFER_POINTER(buf_ptr);
        transmit_frame = indirect_frame->data;

        /*
         * We need to check whether the source PAN-Id of the previously
         * received data request frame is identical to the destination PAN-Id
         * of the pending frame. If not the frame shall not be transmitted,
         * but a Null Data frame instead.
         */
        if (mac_parse_data.src_panid != transmit_frame->dest_panid)
        {
            mac_handle_tx_null_data_frame();
            return;
        }
        else
        {
            /* Store the MSDU Handle for confirmation. */
            mac_msdu_handle = indirect_frame->msduHandle;

            indirect_frame->in_transit = true;

            /*
             * Go through the indirect data queue to find out the frame pending for
             * the device which has requested for the data.
             */
            switch (mac_parse_data.src_addr_mode)
            {
                uint8_t i;
                frame_info_t *data_ptr;

                case FCF_SHORT_ADDR:
                    for (i = 0; i < INDIRECT_DATA_QUEUE_CAPACITY; i++)
                    {
                        if (mac_indirect_array[i].active)
                        {
                            indirect_frame =
                                (indirect_data_t *)
                                    (BMM_BUFFER_POINTER((buffer_t *)mac_indirect_array[i].buf_ptr));

                            data_ptr = indirect_frame->data;

                            /*
                             * Check if the frame is already ready for
                             * transmission.
                             */
                            if (!indirect_frame->in_transit)
                            {
                                /*
                                 * Compare the dest_address(short) of the
                                 * indirect data frame with the source address of
                                 * data request frame.
                                 */
                                if (mac_parse_data.src_addr.short_address ==
                                    (uint16_t)data_ptr->dest_address)
                                {
                                    /*
                                     * Update the frame pending bit in the frame
                                     * control field of the transmitting frame.
                                     */
                                    transmit_frame->frame_ctrl |= FCF_FRAME_PENDING;
                                    break;
                                }
                            }
                        }
                    }
                    break;

                case FCF_LONG_ADDR:
                    for (i = 0; i < INDIRECT_DATA_QUEUE_CAPACITY; i++)
                    {
                        if (mac_indirect_array[i].active)
                        {
                            indirect_frame =
                                (indirect_data_t *) \
                                    (BMM_BUFFER_POINTER((buffer_t *)mac_indirect_array[i].buf_ptr));

                            data_ptr = indirect_frame->data;

                            /*
                             * Check if the frame is already ready
                             * for transmission.
                             */
                            if (!indirect_frame->in_transit)
                            {
                                /*
                                 * Compare the dest_address (extended) of the
                                 * indirect data frame with the source address of
                                 * the data request frame.
                                 */
                                if (mac_parse_data.src_addr.long_address ==
                                    data_ptr->dest_address)
                                {
                                    /*
                                     * Update the frame pending bit in the frame control
                                     * field of the transmitting frame.
                                     */
                                    transmit_frame->frame_ctrl |= FCF_FRAME_PENDING;
                                    break;
                                }
                            }
                        }
                    }
                    break;

                default:
                    break;
            }
        }

        indirect_frame = (indirect_data_t *)BMM_BUFFER_POINTER(buf_ptr);
        indirect_frame->in_transit = false;

        transmit_frame->buffer_header = buf_ptr;

        /*
         * Transmission should be done with CSMA-CA or
         * quickly after the ACK of the data request command.
         * Here it's done quickly after the ACK w/o CSMA.
         */
        tal_tx_status = tal_tx_frame(transmit_frame, NO_CSMA_WITH_IFS, false);

        if (MAC_SUCCESS == tal_tx_status)
        {
            MAKE_MAC_BUSY();
            mac_indirect_flag = true;
        }
        else
        {
#if (DEBUG > 1)
            ASSERT("Indirect data tx failed" == 0);
#endif
        }
    }
} /* mac_process_data_request() */



/*
 * @brief Creates and transmits a Null Data frame
 *
 * This function creates and transmits a Null Data frame in case the
 * coordinator does not have pending data to be transmitted.
 */
void mac_handle_tx_null_data_frame(void)
{
    frame_info_t *tx_frame;
    retval_t tal_tx_status;
    buffer_t *b_ptr;

    /*
     * No matching pending item in the queue,
     * so a Null Data frame is created.
     */
    b_ptr = build_null_data_frame();

    if (NULL != b_ptr)
    {
        tx_frame = (frame_info_t *)BMM_BUFFER_POINTER(b_ptr);

        tx_frame->buffer_header = b_ptr;

        /*
         * Transmission should be done with CSMA-CA or
         * quickly after the ACK of the data request command.
         * Here it's done quickly after the ACK w/o CSMA.
         */
        tal_tx_status = tal_tx_frame(tx_frame, NO_CSMA_WITH_IFS, false);

        if (MAC_SUCCESS == tal_tx_status)
        {
            MAKE_MAC_BUSY();
        }
        else
        {
#if (DEBUG > 1)
            ASSERT("Null frame tx failed" == 0);
#endif
            /*
             * Transmission to TAL failed, free up the buffer used to create
             * Null Data frame.
             */
            bmm_buffer_free(b_ptr);
        }
    }
    else
    {
        /*
         * Buffer is not available to create the Null Data frame, hence
         * abort the operation.
         */
        return;
    }
}



/*
 * @brief Checks for matching short address
 *
 * This callback function checks whether the passed short address
 * matches with the frame in the queue.
 *
 * @param buf Pointer to indirect data buffer
 * @param short_addr Short address to be searched
 *
 * @return 1 if short address passed matches with the destination
 * address of the indirect frame , 0 otherwise
 */
static uint8_t find_short_buffer_cb(void *buf, void *short_addr)
{
    frame_info_t *data;
    indirect_data_t *indirect_frame = (indirect_data_t *)buf;

    data = indirect_frame->data;

    /*
     * Compare indirect data frame's dest_address(short)
     * with short address passed.
     */
    if (*(uint16_t *)short_addr == (uint16_t)data->dest_address)
    {
        return 1;
    }

    return 0;
}



/*
 * @brief Checks for matching extended address
 *
 * This callback function checks whether the passed short address
 * matches with the frame in the queue.
 *
 * @param buf Pointer to indirect data buffer
 * @param long_addr Extended address to be searched
 *
 * @return 1 if extended address passed matches with the destination
 * address of the indirect frame , 0 otherwise
 */
static uint8_t find_long_buffer_cb(void *buf, void *long_addr)
{
    frame_info_t *data;
    indirect_data_t *indirect_frame = (indirect_data_t *)buf;

    data = indirect_frame->data;

    /*
     * Compare indirect data frame's dest_address(extended)
     * with the exended address passed.
     */
    if (*(uint64_t *)long_addr == data->dest_address)
    {
        return 1;
    }
    return 0;
}



/**
 * @brief Checks whether the indirect data frame address matches
 * with the address passed.
 *
 * @param buf Pointer to indirect data buffer
 * @param buffer Pointer to the buffer to be searched
 *
 * @return 1 if address matches, 0 otherwise
 */

uint8_t find_buffer_cb(void *buf, void *buffer)
{
    uint8_t *body = BMM_BUFFER_POINTER((buffer_t *)buffer);
    if (buf == body)
    {
        return 1;
    }
    return 0;
}
#endif  /* (MAC_INDIRECT_DATA_FFD == 1) */
#endif  /* (MAC_INDIRECT_DATA_BASIC == 1) */

/* EOF */
