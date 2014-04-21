/**
 * @file tal_tx.c
 *
 * @brief This file handles the frame transmission within the TAL.
 *
 * $Id: tal_tx.c 17948 2009-09-15 08:45:11Z sschneid $
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
#include <string.h>
#include <stdbool.h>
#include "pal.h"
#include "return_val.h"
#include "tal.h"
#include "ieee_const.h"
#include "tal_pib.h"
#include "tal_irq_handler.h"
#include "tal_constants.h"
#include "tal_tx.h"
#include "stack_config.h"
#include "bmm.h"
#include "qmm.h"
#include "tal_rx.h"
#include "tal_internal.h"
#include "at86rf212.h"
#ifdef BEACON_SUPPORT
#include "tal_slotted_csma.h"
#endif  /* BEACON_SUPPORT */
#include "mac_build_config.h"

/* === TYPES =============================================================== */


/* === MACROS ============================================================== */

/*
 * Command Frame Identifier for Association Request
 */
#define ASSOCIATION_REQUEST             (0x01)

/* === GLOBALS ============================================================= */

static trx_trac_status_t trx_trac_status;

#if ((MAC_START_REQUEST_CONFIRM == 1) && (defined BEACON_SUPPORT))
/**
 * Pointer to 15.4 frame structure for beacon frames.
 */
static uint8_t *beacon_frame;
#endif /* ((MAC_START_REQUEST_CONFIRM == 1) && (defined BEACON_SUPPORT)) */

/* === PROTOTYPES ========================================================== */

static uint8_t * frame_create(frame_info_t *frame_info);


/* === IMPLEMENTATION ====================================================== */

/**
 * @brief Requests to TAL to transmit frame
 *
 * This function is called by the MAC to deliver a frame to the TAL
 * to be transmitted by the transceiver.
 *
 * @param mac_frame_info Pointer to the frame_info_t structure updated by
 *                       the MAC layer
 * @param csma_mode Indicates mode of csma-ca to be performed for this frame
 * @param perform_frame_retry Indicates whether to retries are to be performed for
 *                            this frame
 *
 * @return MAC_SUCCESS  if the TAL has accepted the data from the MAC for frame
 *                 transmission
 *         TAL_BUSY if the TAL is busy servicing the previous MAC request
 */
retval_t tal_tx_frame(frame_info_t *mac_frame_info,
                      csma_mode_t csma_mode,
                      bool perform_frame_retry)
{
    if (tal_state != TAL_IDLE)
    {
        return TAL_BUSY;
    }

    /*
     * Store the pointer to the provided frame structure.
     * This is needed for the callback function.
     */
    mac_frame_ptr = mac_frame_info;

#ifdef TEST_HARNESS
    if (1 == tal_pib_PrivateCCAFailure)
    {
        /*
         * Pretend CCA failure for CCA test purposes.
         * Setting the corresponding TAL states will initiate a
         * tal_tx_frame_done_cb() callback with CCA failure.
         */

        tal_state = TAL_TX_DONE;
        trx_trac_status = TRAC_CHANNEL_ACCESS_FAILURE;
        return MAC_SUCCESS;
    }
#endif

    /* Create the frame to be downloaded to the transceiver. */
    tal_frame_to_tx = frame_create(mac_frame_info);

    /*
     * In case the frame is too large, return immediately indicating
     * invalid status.
     */
    if (tal_frame_to_tx == NULL)
    {
        return MAC_INVALID_PARAMETER;
    }

#ifdef BEACON_SUPPORT
    // check if beacon mode is used
    if (csma_mode == CSMA_SLOTTED)
    {
        slotted_csma_start(perform_frame_retry);
    }
    else
    {
        send_frame(tal_frame_to_tx, csma_mode, perform_frame_retry);
    }
#else   /* No BEACON_SUPPORT */
    send_frame(tal_frame_to_tx, csma_mode, perform_frame_retry);
#endif  /* BEACON_SUPPORT / No BEACON_SUPPORT */

    return MAC_SUCCESS;
}


/**
 * @brief Implements the handling of the transmission end.
 *
 * This function handles the callback for the transmission end.
 */
void tx_done_handling(void)
{
    /* Calculate negative offset of timestamp */
    uint16_t offset;

    tal_state = TAL_IDLE;

    /* Calcuated the tx time */
    offset =
          TAL_CONVERT_SYMBOLS_TO_US((PHY_OVERHEAD + LENGTH_FIELD_LEN) * SYMBOLS_PER_OCTET)
        + TAL_PSDU_US_PER_OCTET(*tal_frame_to_tx + FCS_LEN)
        + IRQ_PROCESSING_DLY_US;
    if ((mac_frame_ptr->frame_ctrl & FCF_ACK_REQUEST) == FCF_ACK_REQUEST)
    {
        /* Tx timestamp needs to be reduced by ACK duration etc. */
        offset +=
              TAL_CONVERT_SYMBOLS_TO_US((PHY_OVERHEAD + LENGTH_FIELD_LEN) * SYMBOLS_PER_OCTET)
            + TAL_PSDU_US_PER_OCTET(ACK_PAYLOAD_LEN + FCS_LEN);

#ifdef HIGH_DATA_RATE_SUPPORT
        if (tal_pib_CurrentPage == 0)
        {
            offset += TAL_CONVERT_SYMBOLS_TO_US(aTurnaroundTime);

        }
        else
        {
            offset += 32;
        }
#else
        offset += TAL_CONVERT_SYMBOLS_TO_US(aTurnaroundTime);
#endif  /* #ifdef HIGH_DATA_RATE_SUPPORT */
    }
    mac_frame_ptr->time_stamp -= offset;

    switch (trx_trac_status)
    {
        case TRAC_SUCCESS:
            tal_tx_frame_done_cb(MAC_SUCCESS, mac_frame_ptr);
            break;

        case TRAC_SUCCESS_DATA_PENDING:
            tal_tx_frame_done_cb(TAL_FRAME_PENDING, mac_frame_ptr);
            break;

        case TRAC_CHANNEL_ACCESS_FAILURE:
            tal_tx_frame_done_cb(MAC_CHANNEL_ACCESS_FAILURE, mac_frame_ptr);
            break;

        case TRAC_NO_ACK:
            tal_tx_frame_done_cb(MAC_NO_ACK, mac_frame_ptr);
            break;

        case TRAC_INVALID:
            tal_tx_frame_done_cb(FAILURE, mac_frame_ptr);
            break;

        default:
            ASSERT("Unexpected tal_tx_state" == 0);
            tal_tx_frame_done_cb(FAILURE, mac_frame_ptr);
            break;
    }
} /* tx_done_handling() */


/**
 * @brief Creates MAC frame
 *
 * This function is called to create a MAC frame using the information
 * passed to tal_tx_frame().
 *
 * @param tal_frame_info Pointer to the frame_info_t structure updated by
 *                       the MAC layer
 *
 * @return Pointer to the created frame header if the frame length is not
 *         larger than aMaxPHYPacketSize, NULL otherwise
 */
static uint8_t *frame_create(frame_info_t *tal_frame_info)
{
    uint8_t frame_length;
    uint16_t frame_format_mask;
    uint8_t *frame_header;

    /*
     * Start creating the frame header backwards starting from the payload.
     * Note: This approach does not require copying the payload again
     * into the frame buffer.
     */
    frame_header = tal_frame_info->payload;

    /* Start with the frame length set to the payload length. */
    frame_length = tal_frame_info->payload_length;

    /* Get the source addressing information. */
    frame_format_mask = tal_frame_info->frame_ctrl & SRC_ADDR_MODE_MASK;

    /*
     * As the frame creation is done backwards, the source address field
     * of the frame header is updated with the source address
     */
    if (SRC_EXT_ADDR_MODE == frame_format_mask)
    {
        frame_header -= EXT_ADDR_LEN;

        convert_64_bit_to_byte_array(tal_frame_info->src_address,
                                     frame_header);

        frame_length += EXT_ADDR_LEN;
    }
    else if (SRC_SHORT_ADDR_MODE == frame_format_mask)
    {
        frame_header -= SHORT_ADDR_LEN;

        convert_16_bit_to_byte_array(tal_frame_info->src_address,
                                     frame_header);

        frame_length += SHORT_ADDR_LEN;
    }

    if (frame_format_mask)
    {
        if ((tal_frame_info->frame_ctrl & FCF_PAN_ID_COMPRESSION) &&
            (tal_frame_info->frame_ctrl & DEST_ADDR_MODE_MASK)
           )
        {
            /*
             * If the Intra-PAN bit is set to 1
             * and both the destination and source addresses are present,
             * then do NOT include the source PAN ID.
             */
        }
        else
        {
            /* Add source PAN ID. */
            frame_header -= PAN_ID_LEN;

            convert_16_bit_to_byte_array(tal_frame_info->src_panid,
                                         frame_header);

            frame_length += PAN_ID_LEN;
        }
    }

    /* Get the destination addressing information. */
    frame_format_mask = tal_frame_info->frame_ctrl & DEST_ADDR_MODE_MASK;

    /*
     * The destination address shall be included in the MAC frame only,
     * if the destination addressing mode subfield of the
     * frame control field is nonzero.
     */
    if (DEST_EXT_ADDR_MODE == frame_format_mask)
    {
        frame_header -= EXT_ADDR_LEN;

        convert_64_bit_to_byte_array(tal_frame_info->dest_address,
                                     frame_header);

        frame_length += EXT_ADDR_LEN;
    }
    else if (DEST_SHORT_ADDR_MODE == frame_format_mask)
    {
        frame_header -= SHORT_ADDR_LEN;

        convert_16_bit_to_byte_array(tal_frame_info->dest_address,
                                     frame_header);

        frame_length += SHORT_ADDR_LEN;
    }

    /*
     * The destination PAN ID shall be included in the MAC frame only,
     * if the destination addressing mode subfield of the
     * frame control field is nonzero.
     */
    if (frame_format_mask)
    {
        frame_header -= PAN_ID_LEN;

        convert_16_bit_to_byte_array(tal_frame_info->dest_panid,
                                     frame_header);

        frame_length += PAN_ID_LEN;
    }

    /*
     * As the frame header creation is done backwards
     * and the next field to be updated is the sequence number,
     * update the sequence number into the frame header
     * by decrementing frame_header by one.
     */
    frame_header--;

    *frame_header = tal_frame_info->seq_num;

    /*
     * The frame length is incremented to includ the length of the
     * sequence number.
     */
    frame_length++;

    /*
     * The next field to be updated is the frame control field of the
     * frame header.
     */
    frame_header -= FCF_LEN;

    convert_16_bit_to_byte_array(tal_frame_info->frame_ctrl,
                                 frame_header);

    /* Include space for FCS and FCF field. */
    frame_length += FCF_LEN + FCS_LEN;

    /*
     * The PHY header contains the length of the frame to be transmitted.
     * This is appended to the created frame.
     */
    frame_header--;

    *frame_header = frame_length;

    /*
     * Frame length is not supposed to be longer than 127 octets
     * (aMaxPHYPacketSize), otherwise the transmission status is undefined.
     */
    if (frame_length > aMaxPHYPacketSize)
    {
        return NULL;
    }

    return (frame_header);
}  /* frame_create() */


/**
 * @brief Sends frame
 *
 * @param frame_tx Pointer to prepared frame
 * @param use_csma Flag indicating if CSMA is requested
 * @param tx_retries Flag indicating if transmission retries are requested
 *                   by the MAC layer
 */
void send_frame(uint8_t *frame_tx, csma_mode_t csma_mode, bool tx_retries)
{
    tal_trx_status_t trx_status;

    // configure tx according to tx_retries
    if (tx_retries)
    {
        pal_trx_bit_write(SR_MAX_FRAME_RETRIES, tal_pib_MaxFrameRetries);
    }
    else
    {
        pal_trx_bit_write(SR_MAX_FRAME_RETRIES, 0);
    }

    // configure tx according to csma usage
    if ((csma_mode == NO_CSMA_NO_IFS) || (csma_mode == NO_CSMA_WITH_IFS))
    {
        if (tx_retries)
        {
            pal_trx_bit_write(SR_MAX_CSMA_RETRIES, tal_pib_MaxCSMABackoffs);
            pal_trx_reg_write(RG_CSMA_BE, 0x00);
        }
        else
        {
            pal_trx_bit_write(SR_MAX_CSMA_RETRIES, 7);
        }
    }
    else
    {
        pal_trx_reg_write(RG_CSMA_BE, ((tal_pib_MaxBE << 4) | tal_pib_MinBE));
        pal_trx_bit_write(SR_MAX_CSMA_RETRIES, tal_pib_MaxCSMABackoffs);
    }

    do
    {
        trx_status = set_trx_state(CMD_TX_ARET_ON);
    } while (trx_status != TX_ARET_ON);

    pal_trx_irq_disable(TRX_MAIN_IRQ_HDLR_IDX);
    tal_state = TAL_TX_AUTO;

    /* Handle interframe spacing */
    if (csma_mode == NO_CSMA_WITH_IFS)
    {
        if (last_frame_length > aMaxSIFSFrameSize)
        {
            pal_timer_delay(TAL_CONVERT_SYMBOLS_TO_US(macMinLIFSPeriod_def)
                            - IRQ_PROCESSING_DLY_US - PRE_TX_DURATION_US);
        }
        else
        {
            pal_timer_delay(TAL_CONVERT_SYMBOLS_TO_US(macMinSIFSPeriod_def)
                            - IRQ_PROCESSING_DLY_US - PRE_TX_DURATION_US);
        }
    }

    /* Toggle the SLP_TR pin triggering transmission. */
    PAL_SLP_TR_HIGH();
// LSR
	//PAL_WAIT_65_NS();
// ^
	PAL_SLP_TR_LOW();

    /*
     * Send the frame to the transceiver.
     * Note: The PhyHeader is the first byte of the frame to
     * be sent to the transceiver and this contains the frame
     * length.
     */
    pal_trx_frame_write(frame_tx, frame_tx[0]);

#ifndef NON_BLOCKING_SPI
    pal_trx_irq_enable(TRX_MAIN_IRQ_HDLR_IDX);
#endif
}


/**
 * @brief Handles interrupts issued due to end of transmission
 */
void handle_tx_end_irq(bool underrun_occured)
{
#if ((MAC_START_REQUEST_CONFIRM == 1) && (defined BEACON_SUPPORT))
    if (tal_beacon_transmission)
    {
        tal_beacon_transmission = false;

        if (tal_csma_state == BACKOFF_WAITING_FOR_BEACON)
        {
            // Slotted CSMA has been waiting for a beacon, now it can continue.
            tal_csma_state = CSMA_HANDLE_BEACON;
        }
    }
    else
#endif /* ((MAC_START_REQUEST_CONFIRM == 1) && (defined BEACON_SUPPORT)) */
    {
        /* Store tx timestamp to frame_info_t structure */
        pal_get_current_time(&mac_frame_ptr->time_stamp);

        /* Read trac status before enabling RX_AACK_ON. */
        if (underrun_occured)
        {
            trx_trac_status = TRAC_INVALID;
        }
        else
        {
            trx_trac_status = (trx_trac_status_t)pal_trx_bit_read(SR_TRAC_STATUS);
        }

#ifdef BEACON_SUPPORT
        if (tal_csma_state == FRAME_SENDING)    // Transmission was issued by slotted CSMA
        {
            tal_state = TAL_SLOTTED_CSMA;

            /* Map status message of transceiver to TAL constants. */
            switch (trx_trac_status)
            {
                case TRAC_SUCCESS_DATA_PENDING:
                    tal_csma_state = TX_DONE_FRAME_PENDING;
                     break;

                case TRAC_SUCCESS:
                    tal_csma_state = TX_DONE_SUCCESS;
                    break;

                case TRAC_CHANNEL_ACCESS_FAILURE:
                    tal_csma_state = CSMA_ACCESS_FAILURE;
                    break;

                case TRAC_NO_ACK:
                    tal_csma_state = TX_DONE_NO_ACK;
                    break;

                case TRAC_INVALID:  /* Handle this in the same way as default. */
                default:
                    ASSERT("not handled trac status" == 0);
                    tal_csma_state = CSMA_ACCESS_FAILURE;
                    break;
            }
        }
        else
#endif  /* BEACON_SUPPORT */
        // Trx has handled the entire transmission incl. CSMA
        {
            tal_state = TAL_TX_DONE;    // Further handling is done by tx_done_handling()
        }
    }

    /*
     * After transmission has finished, switch receiver on again.
     * Check if receive buffer is available.
     */
    if (NULL == tal_rx_buffer)
    {
        set_trx_state(CMD_PLL_ON);
        tal_rx_on_required = true;
    }
    else
    {
        set_trx_state(CMD_RX_AACK_ON);
    }
}


/**
 * @brief Prepares the beacon frame to be sent at the start of superframe
 *
 * This function prepares the beacon frame to be sent at the start of
 * the superframe
 *
 * @param mac_frame_info Pointer to the frame_info_t structure
 */
#if ((MAC_START_REQUEST_CONFIRM == 1) && (defined BEACON_SUPPORT))
void tal_prepare_beacon(frame_info_t *mac_frame_info)
{
    /*
     * Create the beacon frame and buffer the beacon frame to be sent later
     * when tal_tx_beacon() is called.
     */
    beacon_frame = frame_create(mac_frame_info);
}
#endif /* ((MAC_START_REQUEST_CONFIRM == 1) && (defined BEACON_SUPPORT)) */


/**
 * @brief Beacon frame transmission
 */
#if ((MAC_START_REQUEST_CONFIRM == 1) && (defined BEACON_SUPPORT))
void tal_tx_beacon(void)
{
    tal_trx_status_t trx_status;

    /*
     * Avoid that the beacon is transmitted while transmitting
     * a frame using slotted CSMA.
     */
    if (tal_state == TAL_TX_AUTO)
    {
        ASSERT("trying to transmit while slotted CSMA transmits or wait for an ACK" == 0);
        return;
    }

    /* Send the pre-created beacon frame to the transceiver. */
    do
    {
        trx_status = set_trx_state(CMD_PLL_ON);
#if (DEBUG > 1)
        if (trx_status != PLL_ON)
        {
            ASSERT("PLL_ON failed for beacon transmission" == 0);
        }
#endif
    } while (trx_status != PLL_ON);

    // @TODO wait for talbeaconTxTime

    pal_trx_irq_disable(TRX_MAIN_IRQ_HDLR_IDX);
    tal_beacon_transmission = true;

    /* Toggle the SLP_TR pin triggering transmission. */
    PAL_SLP_TR_HIGH();
    PAL_WAIT_65_NS();
    PAL_SLP_TR_LOW();

    /*
     * Send the frame to the transceiver.
     * Note: The PhyHeader is the first byte of the frame to
     * be sent to the transceiver and this contains the frame
     * length.
     */
    pal_trx_frame_write(beacon_frame, beacon_frame[0]-1);

#ifndef NON_BLOCKING_SPI
    pal_trx_irq_enable(TRX_MAIN_IRQ_HDLR_IDX);
#endif
}
#endif /* ((MAC_START_REQUEST_CONFIRM == 1) && (defined BEACON_SUPPORT)) */


/* EOF */

