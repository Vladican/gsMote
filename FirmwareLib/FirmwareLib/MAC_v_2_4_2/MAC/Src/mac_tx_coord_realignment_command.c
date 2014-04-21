/**
 * @file mac_tx_coord_realignment_command.c
 *
 * @brief Implements the coordinator realignment command.
 *
 * $Id: mac_tx_coord_realignment_command.c 18827 2009-10-23 12:40:53Z sschneid $
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
#if (MAC_START_REQUEST_CONFIRM == 1)
#ifdef BEACON_SUPPORT
#include "broadcast_structures.h"
#endif  /* BEACON_SUPPORT */
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */
#include "stack_config.h"
#include "mac_internal.h"
#include "mac.h"

/* === Macros =============================================================== */

/*
 * Coordinator realignment payload length
 */
/*
 * In 802.15.4-2006 the channel page may be added, if the new channel page is
 * different than the original value. In order to simplify the code, it
 * is always added.
 */
#define COORD_REALIGN_PAYLOAD_LEN       (9)

/* === Globals ============================================================== */


/* === Prototypes =========================================================== */


/* === Implementation ======================================================= */

#if ((MAC_ORPHAN_INDICATION_RESPONSE == 1) || (MAC_START_REQUEST_CONFIRM == 1))
/**
 * @brief Sends a coordinator realignment command frame
 *
 * This function is called either in response to the reception of an orphan
 * notification command from a device (cmd_type = ORPHANREALIGNMENT),
 * or gratuitously whenever the PAN parameters are about to be changed
 * (cmd_type = COORDINATORREALIGNMENT). In the first case, the
 * paramater mor contains a pointer to the respective
 * MLME_ORPHAN.response message, while in the latter case this
 * parameter is unused, and can be passed as NULL.
 *
 * @param cmd_type Determines directed or broadcast mode
 * @param buf Pointer to the buffer, using which coord_realignment_command
 *            to be sent
 * @param new_panid Contains the new PAN-ID in case there is a network
 *                  realignment
 * @param new_channel Contains the new channel in case there is a network
 *                    realignment
 * @param new_page Contains the new channel page in case there is a network
 *                 realignment
 *
 * @return True if coord_realignment_command is sent successfully,
 *         false otherwise
 */
bool mac_tx_coord_realignment_command(frame_msgtype_t cmd_type,
                                      buffer_t *buf,
                                      uint16_t new_panid,
                                      uint8_t new_channel,
                                      uint8_t new_page)
{
    uint8_t index;
    retval_t tal_tx_status;

    mlme_orphan_resp_t *mor =
            (mlme_orphan_resp_t *)BMM_BUFFER_POINTER((buffer_t *)buf);

    mlme_orphan_resp_t orphan_resp;

    frame_info_t *coord_realignment_frame;

#ifdef BEACON_SUPPORT
    if ((NON_BEACON_NWK != tal_pib_BeaconOrder) &&
        (COORDINATORREALIGNMENT == cmd_type)
       )
    {
        broadcast_t *broadcast_trans_frame =
            (broadcast_t *)BMM_BUFFER_POINTER((buffer_t *)buf);

        broadcast_trans_frame->data = (frame_info_t *)
            (BMM_BUFFER_POINTER((buffer_t *)buf) + sizeof(broadcast_t));

        /* Add the msduHandle in the broadcast buffer */
        broadcast_trans_frame->msduHandle = 0xFF;
        coord_realignment_frame = broadcast_trans_frame->data;
    }
    else
    {
        coord_realignment_frame = (frame_info_t *)BMM_BUFFER_POINTER((buffer_t *)buf);
    }
#else
    coord_realignment_frame = (frame_info_t *)BMM_BUFFER_POINTER((buffer_t *)buf);
#endif  /* BEACON_SUPPORT */

    /* FCF */
    if (ORPHANREALIGNMENT == cmd_type)
    {
        /*
         * Orphan request is reused to send coordinator realignment
         * command frame and finally to send comm-status-indication
         */
        memcpy(&orphan_resp, mor, sizeof(mlme_orphan_resp_t));

        /*
         * Coordinator realignment in response to an orphan
         * notification command received from a device. This is always
         * sent to a 64-bit device address, and the device is
         * requested to acknowledge the reception.
         */
        coord_realignment_frame->frame_ctrl =
            FCF_SET_SOURCE_ADDR_MODE(FCF_LONG_ADDR) |
            FCF_SET_DEST_ADDR_MODE(FCF_LONG_ADDR) |
            FCF_SET_FRAMETYPE(FCF_FRAMETYPE_MAC_CMD) |
            FCF_ACK_REQUEST;

        coord_realignment_frame->dest_address = orphan_resp.OrphanAddress;
    }
    else
    {
        /*
         * Coordinator realignment gratuitously sent when the PAN
         * configuration changes. This is sent to the (16-bit)
         * broadcast address.
         */
        coord_realignment_frame->frame_ctrl =
            FCF_SET_SOURCE_ADDR_MODE(FCF_LONG_ADDR) |
            FCF_SET_DEST_ADDR_MODE(FCF_SHORT_ADDR) |
            FCF_SET_FRAMETYPE(FCF_FRAMETYPE_MAC_CMD);

        coord_realignment_frame->dest_address = BROADCAST;
    }

    /*
     * Since the channel page is always added at the end of the
     * coordinator realignment command frame, the frame version subfield
     * needs to indicate a 802.15.4-2006 compatible frame.
     */
    coord_realignment_frame->frame_ctrl |= FCF_FRAME_VERSION_2006;

    /* The parameters of the coordinator realignment are updated */

    coord_realignment_frame->seq_num = mac_pib_macDSN++;
    coord_realignment_frame->dest_panid = BROADCAST;

    coord_realignment_frame->src_panid = tal_pib_PANId;
    coord_realignment_frame->src_address = tal_pib_IeeeAddress;

    coord_realignment_frame->payload_length = COORD_REALIGN_PAYLOAD_LEN;
    coord_realignment_frame->payload = ((uint8_t *) coord_realignment_frame) +
                                       LARGE_BUFFER_SIZE -
                                       FCF_SIZE -
                                       coord_realignment_frame->payload_length;

    /*
     * An index variable is used to update the payload of the frame with new
     * paramters, starting from index 0.
     */
    index = 0;

    coord_realignment_frame->payload[index++] = COORDINATORREALIGNMENT;
    coord_realignment_frame->msg_type = cmd_type;

    /*
     * The payload of the frame has the parameters of the new PAN
     * configuration
     */

    coord_realignment_frame->payload[index++] = new_panid;
    coord_realignment_frame->payload[index++] = (new_panid >> 8);

    coord_realignment_frame->payload[index++] = tal_pib_ShortAddress;
    coord_realignment_frame->payload[index++] = (tal_pib_ShortAddress >> 8);

    coord_realignment_frame->payload[index++] = new_channel;

    /*
     * Insert the device's short address, or 0xFFFF if this is a
     * gratuitous realigment.
     */
    if (ORPHANREALIGNMENT == cmd_type)
    {
        coord_realignment_frame->payload[index++] = orphan_resp.ShortAddress;
        coord_realignment_frame->payload[index++] = (orphan_resp.ShortAddress >> 8);
    }
    else
    {
        coord_realignment_frame->payload[index++] = (uint8_t)BROADCAST;
        coord_realignment_frame->payload[index++] = (BROADCAST >> 8);
    }

    /* Add channel page no matter if it changes or not. */
    coord_realignment_frame->payload[index++] = new_page;

    coord_realignment_frame->buffer_header = buf;

    /* The frame is given to the TAL for transmission */
#ifdef BEACON_SUPPORT
    if (NON_BEACON_NWK == tal_pib_BeaconOrder)
    {
        /* In Nonbeacon network the frame is sent with unslotted CSMA-CA. */
        tal_tx_status = tal_tx_frame(coord_realignment_frame, CSMA_UNSLOTTED, true);
    }
    else
    {
        /* Beacon-enabled network */
        if (ORPHANREALIGNMENT == cmd_type)
        {
            /* In Beacon network the Orphan Realignment frame is sent with slotted CSMA-CA. */
            tal_tx_status = tal_tx_frame(coord_realignment_frame, CSMA_SLOTTED, true);
        }
        else
        {
            /*
             * Coordinator Realignment frame is sent to broadcast address,
             * so it needs to be put into the broadcast queue.
             */
            retval_t queue_status;

            /*
             * Append the Coordinator Realignment frame into the broadcast
             * queue.
             */
            queue_status = qmm_queue_append(&broadcast_q, buf);

            if (QUEUE_FULL == queue_status)
            {
                return false;
            }
            return true;
        }
    }
#else
    /* In build without beacon support the frame is sent with unslotted CSMA-CA. */
    tal_tx_status = tal_tx_frame(coord_realignment_frame, CSMA_UNSLOTTED, true);
#endif  /* BEACON_SUPPORT */

    if (MAC_SUCCESS == tal_tx_status)
    {
        /*
         * A positive confirmation is given since the TAL has successfuly accepted
         * the frame for transmission.
         */
        MAKE_MAC_BUSY();
        return true;
    }
    else
    {
#if (DEBUG > 1)
        ASSERT("Coord realign tx failed" == 0);
#endif
        /*
         * TAL was unable to transmit the frame, hence a negative confirmation
         * is returned.
         */
        return false;
    }
} /* mac_tx_coord_realignment_command() */
#endif /* ((MAC_ORPHAN_INDICATION_RESPONSE == 1) || (MAC_START_REQUEST_CONFIRM == 1)) */

/* EOF */

