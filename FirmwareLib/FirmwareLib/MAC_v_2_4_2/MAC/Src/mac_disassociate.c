/**
 * @file mac_disassociate.c
 *
 * @brief Implements the MLME-DISASSOCIATION functionality
 *
 * $Id: mac_disassociate.c 19513 2009-12-11 18:51:54Z sschneid $
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

#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)

/* === Macros =============================================================== */

/*
 * Disassociation payload length
 */
#define DISASSOC_PAYLOAD_LEN             (2)

/* === Globals ============================================================= */


/* === Prototypes ========================================================== */

static void build_disassoc_cmd_frame(uint8_t *buffer,
                                     mlme_disassociate_req_t *disassoc_req,
                                     bool tx_indirect);
static bool mac_awake_disassociate(buffer_t *m);

static void mac_gen_mlme_disassociate_conf(buffer_t *buf,
                                    uint8_t status,
                                    uint8_t dev_addr_mode,
                                    uint16_t dev_panid,
                                    address_field_t *dev_addr);

/* === Implementation ====================================================== */

/**
 * @brief Initiates MLME disassociate confirm message
 *
 * This function creates and appends a MLME disassociate confirm message
 * into the  internal event queue.
 *
 * @param buf Buffer for sending MLME disassociate confirm message to NHLE
 * @param status Status of disassociation
 * @param dev_addr_mode Addressing mode of the device that has either requested
 *                      disassociation or been instructed to disassociate by
 *                      its coordinator.
 * @param dev_panid PAN identifier of the device that has either requested
 *                  disassociation or been instructed to disassociate by its
 *                  coordinator.
 * @param dev_addr Address of the device that has either requested
 *                 disassociation or been instructed to disassociate by its
 *                 coordinator.
 */
static void mac_gen_mlme_disassociate_conf(buffer_t *buf,
                                           uint8_t status,
                                           uint8_t dev_addr_mode,
                                           uint16_t dev_panid,
                                           address_field_t *dev_addr)
{
    mlme_disassociate_conf_t *d_conf;

    d_conf = (mlme_disassociate_conf_t *)BMM_BUFFER_POINTER(buf);

    d_conf->cmdcode = MLME_DISASSOCIATE_CONFIRM;
    d_conf->status = status;
    d_conf->DeviceAddrMode = dev_addr_mode;
    d_conf->DevicePANId = dev_panid;

    if (FCF_SHORT_ADDR == dev_addr_mode)
    {
        /* Clean-up of 64 bit address before writing 16 bit value. */
        d_conf->DeviceAddress = 0;
        /* A short device address is requested. */
        ADDR_COPY_DST_SRC_16(d_conf->DeviceAddress, dev_addr->short_address);
    }
    else
    {
        /* A long device address is requested. */
        ADDR_COPY_DST_SRC_64(d_conf->DeviceAddress, dev_addr->long_address);
    }

    qmm_queue_append(&mac_nhle_q, (buffer_t *)buf);
}



/**
 * @brief Handles the MLME disassociate request command from the NWK layer
 *
 * The MLME-DISASSOCIATE.request primitive is generated by the next
 * higher layer of an associated device and issued to its MLME to
 * request disassociation from the PAN. It is also generated by the
 * next higher layer of the coordinator and issued to its MLME to
 * instruct an associated device to leave the PAN.
 *
 * @param m Pointer to the MLME-DISASSOCIATION.Request message passed by NHLE
 */
void mlme_disassociate_request(uint8_t *m)
{
    mlme_disassociate_req_t disassoc_req;

    uint8_t *buffer = BMM_BUFFER_POINTER((buffer_t *)m);
    uint64_t temp_dev_addr = 0; // Initialize entire address

    /* Store the disassociation request received from NHLE. */
    memcpy((uint8_t *)&disassoc_req, buffer, sizeof(mlme_disassociate_req_t));
    ADDR_COPY_DST_SRC_64(temp_dev_addr, disassoc_req.DeviceAddress);

#if (MAC_DISASSOCIATION_FFD_SUPPORT == 1)
    retval_t queue_status;
    /* Build the request from the coordinator perspective. */

    /*
     * MAC-2003:
     * If the DeviceAddress parameter is not equal to macCoordExtendedAddress
     * and this primitive was received by the MLME of a coordinator, the
     * command will be sent using indirect transmission, i.e., the command
     * frame is added to the list of pending transactions stored on the
     * coordinator and extracted at the discretion of the concerned device
     * using the method described in 7.5.6.3.Otherwise, the MLME issues the
     * MLME-DISASSOCIATE.confirm primitive with a status of MAC_INVALID_PARAMETER.
     */
    /*
     * If the DevicePANId parameter is not equal to macPANId, the MLME issues
     * the MLME-DISASSOCIATE.confirm primitive with a status of
     * MAC_INVALID_PARAMETER.
     * If the DevicePANId parameter is equal to macPANId, the MLME evaluates
     * the primitive address fields.
     *
     * If the DeviceAddrMode parameter is equal to 0x02 and the DeviceAddress
     * parameter is equal to macCoordShortAddress or if the DeviceAddrMode
     * parameter is equal to 0x03 and the DeviceAddress parameter is equal
     * to macCoordExtendedAddress, the TxIndirect parameter is ignored, and
     * the MLME sends a disassociation notification command (see 7.3.3) to
     * its coordinator in the CAP for a beacon-enabled PAN or immediately
     * for a nonbeacon-enabled PAN.
     *
     * If the DeviceAddrMode parameter is equal to 0x02 and the DeviceAddress
     * parameter is not equal to macCoordShortAddress or if the DeviceAddrMode
     * parameter is equal to 0x03 and the DeviceAddress parameter is not equal
     * to macCoordExtendedAddress, and if this primitive was received by the
     * MLME of a coordinator with the TxIndirect parameter set to TRUE, the
     * disassociation notification command will be sent using indirect
     * transmission, i.e., the command frame is added to the list of pending
     * transactions stored on the coordinator and extracted at the discretion
     * of the device concerned using the method described in 7.5.6.3.
     *
     * If the DeviceAddrMode parameter is equal to 0x02 and the DeviceAddress
     * parameter is not equal to macCoordShortAddress or if the DeviceAddrMode
     * parameter is equal to 0x03 and the DeviceAddress parameter is not equal
     * to macCoordExtendedAddress, and if this primitive was received by the
     * MLME of a coordinator with the TxIndirect parameter set to FALSE, the
     * MLME sends a disassociation notification command to the device in the
     * CAP for a beacon-enabled PAN or immediately for a nonbeacon-enabled PAN.
     *
     * Otherwise, the MLME issues the MLME-DISASSOCIATE.confirm primitive with
     * a status of MAC_INVALID_PARAMETER and does not generate a disassociation
     * notification command.
     */
#ifndef REDUCED_PARAM_CHECK
    if (disassoc_req.DevicePANId != tal_pib_PANId)
    {
            mac_gen_mlme_disassociate_conf((buffer_t *)m,
                                           MAC_INVALID_PARAMETER,
                                           disassoc_req.DeviceAddrMode,
                                           disassoc_req.DevicePANId,
                                           (address_field_t *)&temp_dev_addr);
            return;
    }
#endif  /* REDUCED_PARAM_CHECK */

    if (
        ((MAC_PAN_COORD_STARTED == mac_state) || (MAC_COORDINATOR == mac_state)) &&
        (disassoc_req.TxIndirect)
       )
    {
        indirect_data_t *indirect_frame;

        indirect_frame = (indirect_data_t *)buffer;

        indirect_frame->data = (frame_info_t *)((buffer) +
            sizeof(indirect_data_t));

        /*
         * The current device is a coordinator, and if the DeviceAddress
         * is not ours, then send via indirect transmission.
         */
        if (
            ((disassoc_req.DeviceAddrMode == WPAN_ADDRMODE_SHORT) &&
             (disassoc_req.DeviceAddress != macCoordShortAddress)) ||
            ((disassoc_req.DeviceAddrMode == WPAN_ADDRMODE_LONG) &&
            (disassoc_req.DeviceAddress != mac_pib_macCoordExtendedAddress))
           )
        {
            build_disassoc_cmd_frame(buffer, &disassoc_req, true);

#ifdef TEST_HARNESS
            if (mac_pib_privateTransactionOverflow >= 1)
            {   /*
                 * Private PIB is set to indicate no free indirect
                 * buffer for new transaction.
                 */
                queue_status = QUEUE_FULL;
            }
            else
#endif /* TEST_HARNESS */
            {
                queue_status = qmm_queue_append(&indirect_data_q, (buffer_t *)m);
            }

            /* Append the data into indirect queue. */
            if (QUEUE_FULL == queue_status)
            {
                /*
                 * If there is no capacity to store the transaction, the MLME
                 * will discard the MSDU and issue the MLME-DISASSOCIATE.
                 * confirm primitive with a status of TRANSACTION_OVERFLOW.
                 */
                mac_gen_mlme_disassociate_conf((buffer_t *)m,
                                               MAC_TRANSACTION_OVERFLOW,
                                               disassoc_req.DeviceAddrMode,
                                               disassoc_req.DevicePANId,
                                               (address_field_t *)&temp_dev_addr);
                return;
            }

            /*
             * If an FFD does have pending data,
             * the MAC persistence timer needs to be started.
             */
            add_persistence_time(m);
            mac_check_persistence_timer();

            indirect_frame->in_transit = false;
        }
#ifndef REDUCED_PARAM_CHECK
        else
        {
            mac_gen_mlme_disassociate_conf((buffer_t *)m,
                                           MAC_INVALID_PARAMETER,
                                           disassoc_req.DeviceAddrMode,
                                           disassoc_req.DevicePANId,
                                           (address_field_t *)&temp_dev_addr);
            return;
        }
#endif  /* REDUCED_PARAM_CHECK */
    }
    else    /* Device */
#endif /* (MAC_DISASSOCIATION_FFD_SUPPORT == 1) */
    {
        bool transmission_status;

        build_disassoc_cmd_frame(buffer, &disassoc_req, false);

        /* Is the radio in sleep state? */
        if (RADIO_SLEEPING == mac_radio_sleep_state)
        {
            /* Wake up the radio first */
            mac_trx_wakeup();
        }
        transmission_status = mac_awake_disassociate((buffer_t *)m);

        if (!transmission_status)
        {
            /* Create the MLME DISASSOCIATION confirmation message */
            mac_gen_mlme_disassociate_conf((buffer_t *)m,
                                           MAC_CHANNEL_ACCESS_FAILURE,
                                           disassoc_req.DeviceAddrMode,
                                           disassoc_req.DevicePANId,
                                           (address_field_t *)&temp_dev_addr);

            /* Set radio to sleep if allowed */
            mac_sleep_trans();
        }
    }
} /* mlme_disassociate_request() */



/*
 * @brief Continues handling MLME-DISASSOCIATE.request once the radio is awake
 *
 * This function sends the disassociation request information to TAL. On
 * sending the disassociation request all information about its parent device
 * is cleared.
 *
 * @param m Pointer to the buffer to be sent to TAL.
 *
 * @return True if the frame is transmitted successfully, false otherwise.
 */
static bool mac_awake_disassociate(buffer_t *m)
{
    frame_info_t *transmit_frame;
    transmit_frame = (frame_info_t *)BMM_BUFFER_POINTER(m);
    transmit_frame->buffer_header = m;

    /*
     * Send the disassociation information to the TAL.
     * Start CSMA-CA using backoff and retry (direct transmission).
     */
    retval_t tal_tx_status;

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
    tal_tx_status = tal_tx_frame(transmit_frame, CSMA_UNSLOTTED, true);
#endif  /* BEACON_SUPPORT / No BEACON_SUPPORT */

    if (MAC_SUCCESS == tal_tx_status)
    {
        MAKE_MAC_BUSY();
    }
    else
    {
        return false;
    }
    return true;
}



/*
 * @brief Internal function to build the disassociation command frame
 *
 * This function builds a disassociation command frame.
 *
 * @param buffer Frame buffer to fill in the disassociation information
 *               required by the TAL to create a disassociation request frame.
 * @param disassoc_req Specifies MLME-Disassociation.request sent by NHLE.
 * @param tx_indirect Set to true if frame is to be sent indiretly and false otherwise.
 */
static void build_disassoc_cmd_frame(uint8_t *buffer,
                                     mlme_disassociate_req_t *disassoc_req,
                                     bool tx_indirect)
{
    uint16_t fcf;
    frame_info_t *transmit_frame;

#if (MAC_DISASSOCIATION_FFD_SUPPORT == 1)
    if (tx_indirect)
    {
        transmit_frame = ((indirect_data_t *)buffer)->data;
        transmit_frame->dest_address = disassoc_req->DeviceAddress;
    }
    else
#endif  /* (MAC_DISASSOCIATION_FFD_SUPPORT == 1) */
    {
        tx_indirect = tx_indirect;    /* Keep compiler happy. */
        transmit_frame = (frame_info_t *)buffer;

        transmit_frame->dest_address = disassoc_req->DeviceAddress;
    }

    /* Construct FCF. */
    /* 802.15.4-2006 sets the PAN-Id compression) bit. */
    fcf = FCF_SET_FRAMETYPE(FCF_FRAMETYPE_MAC_CMD) |
          FCF_SET_DEST_ADDR_MODE(disassoc_req->DeviceAddrMode) |
          FCF_SET_SOURCE_ADDR_MODE(FCF_LONG_ADDR) |
          FCF_PAN_ID_COMPRESSION |
          FCF_ACK_REQUEST;

    /* Build the Frame Control Field */
    transmit_frame->frame_ctrl = fcf;

    /* Populate the DSN. */
    transmit_frame->seq_num = mac_pib_macDSN++;

    /* Set destination PAN identifier. */
    transmit_frame->dest_panid = tal_pib_PANId;

    transmit_frame->payload_length = DISASSOC_PAYLOAD_LEN;

    transmit_frame->payload = buffer +
                              LARGE_BUFFER_SIZE -
                              FCF_SIZE - transmit_frame->payload_length;

    /*
     * Set the source addres of the device sending the command.
     */
    transmit_frame->src_address = tal_pib_IeeeAddress;

    /* Set up the command frame identifier. */
    transmit_frame->payload[0] = transmit_frame->msg_type = DISASSOCIATIONNOTIFICATION;

    /* Set up the disassociation reason code. */
    transmit_frame->payload[1] = disassoc_req->DisassociateReason;
} /* build_disassoc_cmd_frame() */



/**
 * @brief Process a disassociation notification command
 *
 * This functions processes a received disassociation notification
 * command frame.
 * Actual data are taken from the incoming frame in mac_parse_buffer.
 *
 * @param msg Frame buffer to be filled in
 */
void mac_process_disassociate_notification(buffer_t *msg)
{
    mlme_disassociate_ind_t *dai =
        (mlme_disassociate_ind_t *)BMM_BUFFER_POINTER(((buffer_t *)msg));

    /* Set up the header portion of the mlme_disassociate_ind_t. */
    dai->cmdcode = MLME_DISASSOCIATE_INDICATION;

    /* Build the indication parameters. */

    /*
     * Set the DeviceAddress first. The device address is the address
     * of the device requesting the disassociaton which is always
     * contained in the source address.
     */
    ADDR_COPY_DST_SRC_64(dai->DeviceAddress, mac_parse_data.src_addr.long_address);

    dai->DisassociateReason = mac_parse_data.payload_data.disassoc_req_data.disassoc_reason;

    qmm_queue_append(&mac_nhle_q, (buffer_t *)msg);

    /*
     * Once a device is disassociated from a coordinator, the coordinator's
     * address info should be cleared.
     */
    mac_pib_macCoordExtendedAddress = CLEAR_ADDR_64;

    /* The default short address is 0xFFFF. */
    mac_pib_macCoordShortAddress = INVALID_SHORT_ADDRESS;
}



/**
 * @brief Prepares a disassociation confirm message with device address information
 *
 * This functions prepares a disassociation confirm message in case the device
 * address information needs to be extracted.
 *
 * @param buf Buffer for sending MLME disassociate confirm message to NHLE
 * @param status Status of disassociation
 * @param frame_ptr Pointer to frame_info_t structure that may contain the
 *                  required device address information
 */
void mac_prep_disassoc_conf(buffer_t *buf,
                            uint8_t status,
                            frame_info_t *frame_ptr)
{
#if (MAC_DISASSOCIATION_FFD_SUPPORT == 1)
    uint8_t dis_dest_addr_mode;
    uint64_t temp_dev_addr;
    ADDR_COPY_DST_SRC_64(temp_dev_addr, frame_ptr->dest_address);

    dis_dest_addr_mode = (((frame_ptr->frame_ctrl) >> FCF_DEST_ADDR_OFFSET) & 3);

    if (MAC_PAN_COORD_STARTED == mac_state)
    {
        /*
         * For PAN coordinator fill parameters of device that
         * we requested to disassociate into the disassociation confirm
         * message.
         * Since we have transmitted the disassociation notification frame
         * ourvelves, the destination address information is to be used here.
         */
        mac_gen_mlme_disassociate_conf((buffer_t *)buf,
                                       status,
                                       dis_dest_addr_mode,
                                       frame_ptr->dest_panid,
                                       (address_field_t *)&temp_dev_addr);
    }
    else if (MAC_COORDINATOR == mac_state)
    {
        /* We are a coordinator. */
        /*
         * There are 2 potential choices for a coordinator to get here:
         * 1) We have requested ourselves to disassociate with our own parent.
         * 2) We have requested one of our children (other coordinators or
         *    end devices) to leave the network.
         */
        /*
         * For case 1) check whether the destination address of the
         * disassociation notification frame is matching the address
         * of our parent.
         */
        if (
            /*
             * We had requested to disassociate from our parent using the
             * short address of the parent.
             */
            (
             (FCF_SHORT_ADDR == dis_dest_addr_mode) &&
             (frame_ptr->dest_address == mac_pib_macCoordShortAddress)
            ) ||
            /*
             * We had requested to disassociate from our parent using the
             * extended address of the parent.
             */
            (
             (FCF_LONG_ADDR == dis_dest_addr_mode) &&
             (frame_ptr->dest_address == mac_pib_macCoordExtendedAddress)
            )
           )
        {
            /*
             * We are acting as a child here, so we need to fill in our
             * own device parameter into the disassociation confirm message.
             */
            if ((BROADCAST == tal_pib_ShortAddress) ||
                (MAC_NO_SHORT_ADDR_VALUE == tal_pib_ShortAddress)
               )
            {
                /* We have no valid short address. */
                mac_gen_mlme_disassociate_conf((buffer_t *)buf,
                                               status,
                                               FCF_LONG_ADDR,
                                               tal_pib_PANId,
                                               (address_field_t *)&tal_pib_IeeeAddress);
            }
            else
            {
                /* We have a valid short address. */
                mac_gen_mlme_disassociate_conf((buffer_t *)buf,
                                               status,
                                               FCF_SHORT_ADDR,
                                               tal_pib_PANId,
                                               (address_field_t *)&tal_pib_ShortAddress);
            }
        }
        else
        {
            /*
             * We are acting as a parent here and have requested one of our
             * children to leave the network.
             * For coordinators that are disassociating their children.
             * fill parameters of device/child that we requested to
             * disassociate into the disassociation confirm message.
             * Since we have transmitted the disassociation notification frame
             * ourvelves, the destination address information is to be used here.
             */
            mac_gen_mlme_disassociate_conf((buffer_t *)buf,
                                           status,
                                           dis_dest_addr_mode,
                                           frame_ptr->dest_panid,
                                           (address_field_t *)&temp_dev_addr);
        }
    }
    else
#endif /* (MAC_DISASSOCIATION_FFD_SUPPORT == 1) */
    {
        /*
         * We are an end device, so we need to fill in our own device
         * parameter into the disassociation confirm message.
         */
        if ((BROADCAST == tal_pib_ShortAddress) ||
            (MAC_NO_SHORT_ADDR_VALUE == tal_pib_ShortAddress)
           )
        {
            /* We have no valid short address. */
            mac_gen_mlme_disassociate_conf((buffer_t *)buf,
                                           status,
                                           FCF_LONG_ADDR,
                                           tal_pib_PANId,
                                           (address_field_t *)&tal_pib_IeeeAddress);
        }
        else
        {
            /* We have a valid short address. */
            mac_gen_mlme_disassociate_conf((buffer_t *)buf,
                                           status,
                                           FCF_SHORT_ADDR,
                                           tal_pib_PANId,
                                           (address_field_t *)&tal_pib_ShortAddress);
        }
    }

    frame_ptr = frame_ptr;  /* Keep compiler happy. */
}

#endif  /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) */

/* EOF */
