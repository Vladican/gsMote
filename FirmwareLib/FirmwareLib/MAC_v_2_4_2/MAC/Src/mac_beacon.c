/**
 * @file mac_beacon.c
 *
 * @brief
 * Implements the building of beacon frames and initiates transmission via
 * CSMA-CA after reception of a beacon request frame in a nonbeacon-enabled PAN.
 *
 * $Id: mac_beacon.c 20067 2010-01-28 12:35:18Z sschneid $
 *
 * @author    Atmel Corporation: http://www.atmel.com
 * @author    Support email: avr@atmel.com
 */
/*
 * Copyright (c) 2009, Atmel Corporation All rights reserved.
 *
 * Licensed under Atmel's Limited License Agreement --> EULA.txt
 */

/* === Includes ============================================================= */

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

/*
 * Define the mask for extracting 16 bit short address from frame control.
 */
#define FCF_DESTINATION_SHORT_ADDRESS       (0x0800)

/*
 * Define the mask for extracting 64 bit extended address from frame control.
 */
#define FCF_DESTINATION_EXT_ADDRESS         (0x0C00)

/*
 * Size of CRC in octets.
 */
#define CRC_SIZE                            (2)

/*
 * Mask to extract destination address mode from frame control field.
 */
#define DEST_ADDR_MODE_MASK                 (0x0C00)

/*
 * Pending address specification length of a beacon frame.
 */
#define PENDING_ADDR_SPEC_SIZE              (1)

/*
 * Static buffer for beacon.
 */
#define BCN_BUFFER_SIZE                     (140)

/*
 * Time in (advance) symbols before beacon interval when beacon is prepared
 */
#define ADVNC_BCN_PREP_TIME                 (50)

/* === Globals ============================================================== */

#if (MAC_START_REQUEST_CONFIRM == 1)

#ifdef BEACON_SUPPORT
/*
 * Static buffer used for beacon transmission in a BEACON build.
 * In a build without beacon suppport, in order to save the static buffer,
 * a new buffer will be allocated to transmit the beacon frame.
 */
static uint8_t beacon_buffer[BCN_BUFFER_SIZE];
#endif  /* BEACON_SUPPORT */

#if (MAC_INDIRECT_DATA_FFD == 1)
/* Pointer used for adding pending addresses to the beacon frame. */
static uint8_t *beacon_ptr;

/* Variable to hold number the pending addresses. */
static uint8_t pending_address_count;
#endif  /* (MAC_INDIRECT_DATA_FFD == 1) */

#ifdef TEST_HARNESS
static uint8_t vpan_no;
#endif  /* TEST_HARNESS */

/* === Prototypes =========================================================== */

#if (MAC_INDIRECT_DATA_BASIC == 1)
static uint8_t add_pending_extended_address_cb(void *buf_ptr, void *handle);
static uint8_t add_pending_short_address_cb(void *buf_ptr, void *handle);
static uint8_t mac_buffer_add_pending(uint8_t *buf_ptr);
#endif

#ifdef BEACON_SUPPORT
static void mac_t_beacon_cb(void *callback_parameter);
static void mac_t_prepare_beacon_cb(void *callback_parameter);
/* TODO */
/*
static void mac_t_superframe_cb(void *callback_parameter);
 */
#endif  /* BEACON_SUPPORT */

/* === Implementation ======================================================= */

#if (MAC_INDIRECT_DATA_FFD == 1)
/*
 * @brief Populates the beacon frame with pending addresses
 *
 * This function populates the beacon frame with pending addresses by
 * traversing through the indirect queue.
 *
 * @param buf_ptr Pointer to the location in the beacon frame buffer where the
 * pending addresses are to be updated
 *
 * @return Number of bytes added in the beacon frame as a result of pending
 * address
 */
static uint8_t mac_buffer_add_pending(uint8_t *buf_ptr)
{
    search_t find_buf;
    uint8_t number_of_short_address = 0;

   /* Leave a space in the buffer for pending address specification. */
    beacon_ptr = buf_ptr + PENDING_ADDR_SPEC_SIZE;

    /* Initialize short address count. */
    pending_address_count = 0;

    /*
     * This callback function traverses through the indirect queue and
     * updates the beacon buffer with the pending short addresses.
     */
    find_buf.criteria_func = add_pending_short_address_cb;

    /*
     * At the end of this function call (qmm_queue_read), the beacon buffer
     * will be updated with the short address (if any) of the indirect
     * data (if any) present in the indirect queue.
     */
    qmm_queue_read(&indirect_data_q, &find_buf);

    /*
     * The count of short addresses added in the beacon frame is backed up
     * (as the same variable will be used to count the number of added
     * extended addresses) and the pending address specification in the
     * beacon frame is updated with the same.
     */
    number_of_short_address = *buf_ptr = pending_address_count;

    /* Initialize extended address count. */
    pending_address_count = 0;

    /*
     * This callback function traverses through the indirect queue and
     * updates the beacon buffer with the pending extended addresses.
     */
    find_buf.criteria_func = add_pending_extended_address_cb;

    /*
     * At the end of this function call (qmm_queue_read), the beacon buffer
     * will be updated with the extended address (if any) of the indirect
     * data (if any) present in the indirect queue.
     */
    qmm_queue_read(&indirect_data_q, &find_buf);

    /*
     * Pending address specification is updated with pending
     * extended address.
     */
    *buf_ptr |= (pending_address_count << 4);

    /* Total number of bytes used for pending address in beacon frame */
    pending_address_count = (number_of_short_address * sizeof(uint16_t)) +
                            (pending_address_count * sizeof(uint64_t)) +
                            PENDING_ADDR_SPEC_SIZE;

    return pending_address_count;
} /* mac_buffer_add_pending() */
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */



/**
 * @brief Builds and transmits the beacon frame
 *
 * This function is called to build a beacon frame. For beaconless network
 * this function also transmits the generated beacon frame.
 *
 * @param beacon_enabled Flag indicating the mode of beacon transmission
 * @param beacon_buffer_header For build without beacon support only:
 *                             Pointer to buffer of beacon frame to be
 *                             transmitted in nonbeacon network.
 **/
#ifdef BEACON_SUPPORT
void mac_build_and_tx_beacon(bool beacon_enabled)
#else   /* No BEACON_SUPPORT */
void mac_build_and_tx_beacon(bool beacon_enabled,
                             buffer_t *beacon_buffer_header)
#endif  /* BEACON_SUPPORT / No BEACON_SUPPORT */
{
    frame_info_t *transmit_frame;
    uint8_t end_of_bcn_payload;
    uint8_t i;
    uint8_t payload_length;
    uint8_t index;
    uint16_t superframe_spec = 0;
    uint16_t fcf;

#ifdef BEACON_SUPPORT
    /*
     * The frame is given to the TAL in the 'frame_info_t' format,
     * hence an instance of the frame_info_t is created.
     */
    transmit_frame = (frame_info_t *)beacon_buffer;
#else   /* No BEACON_SUPPORT */
    uint8_t *beacon_buffer = BMM_BUFFER_POINTER(beacon_buffer_header);

    /*
     * The frame is given to the TAL in the 'frame_info_t' format,
     * hence an instance of the frame_info_t is created.
     */
    transmit_frame = (frame_info_t *)beacon_buffer;

    /* Store buffer header to be able to release it later properly. */
    transmit_frame->buffer_header = beacon_buffer_header;
#endif  /* BEACON_SUPPORT / No BEACON_SUPPORT */

    /* The format of the FCF is decided based on the presence of a src address. */
    if (MAC_NO_SHORT_ADDR_VALUE == tal_pib_ShortAddress)
    {
        fcf = FCF_SET_SOURCE_ADDR_MODE((uint16_t)FCF_LONG_ADDR);
    }
    else
    {
        fcf = FCF_SET_SOURCE_ADDR_MODE((uint16_t)FCF_SHORT_ADDR);
    }

    fcf = fcf | FCF_SET_FRAMETYPE(FCF_FRAMETYPE_BEACON);

#if (MAC_START_REQUEST_CONFIRM == 1)
#ifdef BEACON_SUPPORT
    /*
     * In case
     * 1) the node is a PAN Coordinator or Coordinator, and
     * 2) this is beacon-enabled network, and
     * 3) there is a broadcast frame to be transmitted,
     * the frame pending bit in the frame control field of the beacon frame
     * to be transmitted needs to be set in order to indicate this to all
     * listening children nodes.
     */
    if (
        ((MAC_PAN_COORD_STARTED == mac_state) || (MAC_COORDINATOR == mac_state)) &&
        (tal_pib_BeaconOrder < NON_BEACON_NWK) &&
        (broadcast_q.size > 0)
       )
    {
        fcf |= FCF_FRAME_PENDING;
    }
#endif  /* BEACON_SUPPORT */
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */


    transmit_frame->frame_ctrl = fcf;

    /* Beacon sequence number field is updated. */
    transmit_frame->seq_num = mac_pib_macBSN;
    mac_pib_macBSN++;

    transmit_frame->src_panid = tal_pib_PANId;

#ifdef TEST_HARNESS
    if (mac_pib_privateVirtualPANs > 0)
    {
        /*
         * This changes the PAN-ID of subsequent beacon frames to simulate
         * virtual PANs for testing purposes.
         */
        transmit_frame->src_panid = transmit_frame->src_panid + vpan_no;
        vpan_no++;
        vpan_no = vpan_no % mac_pib_privateVirtualPANs;
    }
#endif /* TEST_HARNESS */

    /*
     * The address fields are updated. For a beacon frame, only the
     * source address information is updated.
     */
    if (MAC_NO_SHORT_ADDR_VALUE == tal_pib_ShortAddress)
    {
        transmit_frame->src_address = tal_pib_IeeeAddress;
    }
    else
    {
        transmit_frame->src_address = tal_pib_ShortAddress;
    }

    transmit_frame->msg_type = BEACON_MESSAGE;

    index = sizeof(frame_info_t);

    /* The superframe specification field is updated. */
#ifdef BEACON_SUPPORT
    superframe_spec = tal_pib_BeaconOrder;
    superframe_spec |= (tal_pib_SuperFrameOrder << 4);
    superframe_spec |= (mac_final_cap_slot << 8);

    if (tal_pib_BattLifeExt)
    {
        superframe_spec |= (1U << BATT_LIFE_EXT_BIT_POS);
    }
#else
    superframe_spec = NON_BEACON_NWK;
    superframe_spec |= (NON_BEACON_NWK << 4);
    superframe_spec |= (FINAL_CAP_SLOT_DEFAULT << 8);
#endif

    if (MAC_PAN_COORD_STARTED == mac_state)
    {
        superframe_spec |= (1U << PAN_COORD_BIT_POS);
    }

#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)
    if (mac_pib_macAssociationPermit)
    {
        superframe_spec |= (1U << ASSOC_PERMIT_BIT_POS);
    }
#endif

    /*
     * The beacon payload is updated with the superframe specification, gts
     * fields, pending addresses and the payload itself.
     */
    memcpy(&beacon_buffer[index], &superframe_spec, sizeof(uint16_t));
    index += sizeof(uint16_t);

    /* Build the (empty) GTS fields. */
    beacon_buffer[index++] = 0;

    /* Build the Pending address field. */
#if (MAC_INDIRECT_DATA_BASIC == 1)
    index += mac_buffer_add_pending(&beacon_buffer[index]);
#else
    /*
     * If indirect transmission is not enabled, the Pending Address Spec
     * field is always 0.
     */
    beacon_buffer[index++] = 0;
#endif

    /* Build the beacon payload if it exists. */
    if (mac_pib_macBeaconPayloadLength > 0)
    {
        memcpy(&beacon_buffer[index], mac_beacon_payload, mac_pib_macBeaconPayloadLength);
        index += mac_pib_macBeaconPayloadLength;
    }

    /* Get the end of the payload for reverse copying. */
    end_of_bcn_payload = index;

    payload_length = index - sizeof(frame_info_t);

    /*
     * The location from which the bytes of the beacon payload are copied in the
     * reverse order.
     */
    index = BCN_BUFFER_SIZE - CRC_SIZE;

    for (i = 0; i <= payload_length; i++)
    {
        beacon_buffer[index--] = beacon_buffer[end_of_bcn_payload--];
    }

    /* Compensate for the extra decrement done above. */
    index++;

    /*
     * The payload length and the payload pointer of the frame_info_t
     * structure are updated.
     */
    transmit_frame->payload_length = payload_length;

    transmit_frame->payload = &beacon_buffer[index];

#ifdef BEACON_SUPPORT
    if (beacon_enabled)
    {
        /*
         * The beacon in a beacon enabled network is cached and sent only
         * when tal_beacon_tx() is called by MAC.
         */
        tal_prepare_beacon(transmit_frame);
    }
    else
    {
        /* Buffer header not required in BEACON build. */
        transmit_frame->buffer_header = NULL;
#endif  /* BEACON_SUPPORT */

        /*
         * In a beaconless network the beacon is transmitted with
         * unslotted CSMA-CA.
         */
        tal_tx_frame(transmit_frame, CSMA_UNSLOTTED, false);

        MAKE_MAC_BUSY();

#ifdef BEACON_SUPPORT
    }
#endif  /* BEACON_SUPPORT */

#ifndef BEACON_SUPPORT
    beacon_enabled = beacon_enabled;    /* Keep compiler happy. */
#endif
} /* mac_build_and_tx_beacon() */



#if (MAC_INDIRECT_DATA_BASIC == 1)
/*
 * @brief Appends the pending short addresses into the beacon frame
 *
 * This function appends the pending short addresses to the beacon based
 * on frames currently in the indirect queue.
 *
 * @param buf_ptr Pointer to the indirect data in the indirect queue
 * @param handle Callback parameter
 *
 * @return 0 to traverse through the full indirect queue
 *
 */
static uint8_t add_pending_short_address_cb(void *buf_ptr, void *handle)
{
    uint16_t short_addr;
    indirect_data_t *indirect_frame = (indirect_data_t *)buf_ptr;

    /*
     * The frame_info_t strucure is obtained from indirect strucure. This is
     * required to get the FCF of the data.
     */
    frame_info_t *frame = indirect_frame->data;
    ADDR_COPY_DST_SRC_16(short_addr, frame->dest_address);

    /*
     * Only if the destination addressing mode is short address mode then the
     * indirect data is used to populate the beacon buffer with short
     * destination address.
     */
    if (FCF_DESTINATION_SHORT_ADDRESS == (frame->frame_ctrl & DEST_ADDR_MODE_MASK))
    {
        memcpy(beacon_ptr, &short_addr, sizeof(uint16_t));
        beacon_ptr += sizeof(uint16_t);
        pending_address_count++;
    }

    handle = handle;    /* Keep compiler happy. */

    return 0;
}
#endif /* (MAC_INDIRECT_DATA_BASIC == 1) */



#if (MAC_INDIRECT_DATA_BASIC == 1)
/*
 * @brief Appends the pending extended address into beacon
 *
 * This function appends pending extended addresses in the indirect queue
 * to the beacon.
 *
 * @param buf_ptr Pointer to the indirect data in the indirect queue
 * @param handle Callback parameter
 *
 * @return 0 to traverse through the full indirect queue
 *
 */
static uint8_t add_pending_extended_address_cb(void *buf_ptr, void *handle)
{
    uint64_t ext_addr;
    indirect_data_t *indirect_frame = (indirect_data_t *)buf_ptr;

    /*
     * The frame_info_t strucure is obtained from indirect structure. This is
     * required to get the FCF of the data.
     */
    frame_info_t *frame = indirect_frame->data;
    ADDR_COPY_DST_SRC_64(ext_addr, frame->dest_address);

    /*
     * Only if the destination addressing mode is extended address mode then the
     * indirect data is used to populate the beacon buffer with extended
     * destination address.
     */
    if (FCF_DESTINATION_EXT_ADDRESS == (frame->frame_ctrl & DEST_ADDR_MODE_MASK))
    {
        memcpy(beacon_ptr, &ext_addr, sizeof(uint64_t));
        beacon_ptr += sizeof(uint64_t);
        pending_address_count++;
    }

    handle = handle;    /* Keep compiler happy. */

    return 0;
}
#endif /* (MAC_INDIRECT_DATA_BASIC == 1) */



/**
 * @brief Processes a beacon request
 *
 *  This function is called in case a beacon request frame has been received by
 *  a coordinator. In a nonbeacon-enabled PAN the generation of a beacon frame
 *  using CSMA-CA is initiated. In a beacon-enabled PAN no extra beacon frame
 *  will be transmitted apart from the standard beacon frames.
 *
 * @param msg Pointer to the buffer in which the beaocn request was received
 */
void mac_process_beacon_request(buffer_t *msg)
{
#ifdef BEACON_SUPPORT
    /*
     * The buffer in which the beacon request was received is freed up.
     * This is only done in a BEACON build, since a static buffer is used
     * to transmit the beacon frame.
     */
    bmm_buffer_free((buffer_t *)msg);

    /*
     * If the network is a beacon enabled network then the beacons will not be
     * transmitted.
     */
    if (tal_pib_BeaconOrder == NON_BEACON_NWK)
    {
        /* The beacon is transmitted using CSMA-CA. */
        mac_build_and_tx_beacon(false);
    }
#else   /* No BEACON_SUPPORT */
    mac_build_and_tx_beacon(false, (buffer_t *)msg);
#endif  /* BEACON_SUPPORT / No BEACON_SUPPORT */
}



#ifdef BEACON_SUPPORT
/**
 * @brief Starts the beacon timer
 *
 * This function is called during MLME_START.request operation to start the
 * timers for beacon transmission in case the network is of type beacon enabled,
 * or to start the timer to count the transcation presistence time of indirect
 * data in case the network is nonbeacon enabled.
 *
 * The timers started for a beacon enabled and nonbeacon enabled networks are
 * different with different timeout values.
 *
 * In case of a beacon enabled network two timers are started. The first timer
 * prepares the beacon and second timer sends the beacon. In case of the
 * nonbeacon enabled network, only a single timer is started with timeout as
 * the beacon interval.
 *
 * For a beacon enabled network the first beacon of the network is prepared and
 * sent in this function and the subsequent beacons are sent in the callback
 * function of the timer.
 */
void mac_start_beacon_timer(void)
{
    uint32_t beacon_tx_time;
    uint32_t beacon_int_symbols;
    /*
     * This is a beacon enabled network.
     * The First beacon in a beacon enabled network is transmitted
     * directly without CSMA-CA.
     * This function call, to transmit the beacon frame without
     * CSMA-CA (direct), will cause it to store the beacon at the TAL.
     */
    mac_build_and_tx_beacon(true);

    /*
     * For the first beacon the current time is used as beacon transmission
     * time. For consecutive beacon transmissions, this beacon transmission
     * time is added to the beacon interval and then used.
     */
    pal_get_current_time(&beacon_tx_time);

    /*
     * Timer gives the time in microseconds, but the PIB value at the TAL is in
     * symbols. Hence a coversion is done before setting the PIB at the TAL.
     */
    beacon_tx_time = TAL_CONVERT_US_TO_SYMBOLS(beacon_tx_time);

    /* Now it gets tricky:
     * Since teh PIB attribute macBeaconTxTime is supposed to hold the time
     * of the last transmitted beacon frame, but we are going to transmit
     * our first beacon frame just now (in teh subsequentially called function
     * mac_t_beacon_cb(), we need to actually substract the time of one entire
     * beacon period from this time.
     * Otherwise we would leave out the second beacon in function
     * mac_t_beacon_cb().
     */
    beacon_int_symbols = TAL_GET_BEACON_INTERVAL_TIME(tal_pib_BeaconOrder);

    beacon_tx_time = tal_sub_time_symbols(beacon_tx_time, beacon_int_symbols);

    set_tal_pib_internal(macBeaconTxTime, (void *)&beacon_tx_time);

    /*
     * Now start the handling of all relevant beacon timers:
     * 1) T_Beacon: The regular beacon timer
     * 2) T_Beacon_Preparation: The Pre-beacon timer is used to give the node
     *    sufficient time prior to the actual beacon timer in order to prepare
     *    the next beacon frame or wake-up before the next beacon reception.
     * 3) T_Superframe: The timer when the inactive portion of the superframe
     *    starts; if there is not inactive portion, this timer is NOT running.
     *
     * This is simply done by calling the same function that will later be
     * called upon expiration of the main beacon timer and will handle alle
     * other timers as well to have a common time base for beacon related
     * timers.
     */
    /* This will also actually transmit the beacon. */
    mac_t_beacon_cb(NULL);
} /* mac_start_beacon_timer() */
#endif /* BEACON_SUPPORT */



#ifdef BEACON_SUPPORT
/*
 * @brief Callback function of the beacon preparation time timer
 *
 * This is the calback function of the timer started with timeout
 * value equal to ADVNC_BCN_PREP_TIME symbols less than beacon interval.
 *
 * In case of the beacon enabled network the beacon frame is created and
 * stored at the TAL. After preparing the beacon, the timer is restarted for
 * the next beacon interval less the beacon preparation time.
 *
 * @param callback_parameter Callback parameter
 */
static void mac_t_prepare_beacon_cb(void *callback_parameter)
{
    if (RADIO_SLEEPING == mac_radio_sleep_state)
    {
        /* Wake up radio first */
        mac_trx_wakeup();
    }

    /* For a beacon enabled network, the beacon is stored at the TAL. */
    mac_build_and_tx_beacon(true);

    callback_parameter = callback_parameter;  /* Keep compiler happy. */
} /* mac_t_prepare_beacon_cb() */
#endif  /* BEACON_SUPPORT */



#ifdef BEACON_SUPPORT
/*
 * @brief Transmits beacon frame after beacon interval
 *
 * This function is a callback function of the timer started with the timeout
 * value as the beacon interval time. The TAL function for transmitting
 * the beacon is called and the timer for sending the next beacon is started.
 *
 * @param callback_parameter Callback parameter
 */
static void mac_t_beacon_cb(void *callback_parameter)
{
    /*
     * This check here is done in order to stop beacon timing in case
     * the network has transitioned from a beacon-enabled network to
     * nonbeacon-enabled network.
     */
    if (tal_pib_BeaconOrder < NON_BEACON_NWK)
    {
        /*
         * In case the node is currently scanning, no beacon will be transmitted.
         */
        if (MAC_SCAN_IDLE == mac_scan_state)
        {
            /* This TAL function transmits the beacon fame. */
            tal_tx_beacon();
        }

        uint32_t beacon_int_us;
        uint32_t beacon_tx_time_symbols;
        uint32_t beacon_tx_time_us;
        uint32_t next_beacon_tx_time;
        retval_t status = FAILURE;

        /* 1) Start with main beacon timer. */
        beacon_int_us =
            TAL_CONVERT_SYMBOLS_TO_US(
                TAL_GET_BEACON_INTERVAL_TIME(tal_pib_BeaconOrder));

        /* This was the time when when transmitted the previous beacon frame. */
        beacon_tx_time_us = TAL_CONVERT_SYMBOLS_TO_US(tal_pib_BeaconTxTime);

        /*
         * This is supposed to be the time when we just had transmitted this
         * beacon frame (see calling of tal_tx_beacon() above).
         */
        next_beacon_tx_time = pal_add_time_us(beacon_tx_time_us,
                                              beacon_int_us);

        /*
         * In order to get the proper timeout value for the next beacon timer,
         * we have to add one more beacon interval time.
         * If the timer cannot be started, then we add more beacon intervals,
         * until we finally succeed.
         */
        while (MAC_SUCCESS != status)
        {
            next_beacon_tx_time = pal_add_time_us(next_beacon_tx_time, beacon_int_us);

            status = pal_timer_start(T_Beacon,
                                     next_beacon_tx_time,
                                     TIMEOUT_ABSOLUTE,
                                     (FUNC_PTR)mac_t_beacon_cb,
                                     NULL);
        }

        /*
         * Even if this may look odd, since we already had added a beacon
         * interval time to this variable before the while loop above,
         * we need to manually substract one beacon interval time,
         * in order to keep pace with the new value of macBeaconTxTime
         * just in case several calls of pal_add_time_us() had to be done
         * in the while loop before the timer could be started.
         */
        beacon_tx_time_us = pal_sub_time_us(next_beacon_tx_time,
                                            beacon_int_us);

        beacon_tx_time_symbols =
            TAL_CONVERT_US_TO_SYMBOLS(beacon_tx_time_us);

        /* The beacon transmission time is updated at TAL. */
        set_tal_pib_internal(macBeaconTxTime, (void *)&beacon_tx_time_symbols);

        {
            /* 2) Beacon preparation timer for building the new beacon frame. */
            uint32_t next_beacon_prep_time;

            /*
             * A regular but absolute timer is started which will expire
             * ADVNC_BCN_PREP_TIME symbols prior to beacon interval. On its expiry
             * the beacon frame is created and given to TAL.
             * The absolute time at which the beacon is to be prepared is calculated by
             * subtracting the beacon preparation time from the absolute time value of
             * the beacon transmission.
             */
            next_beacon_prep_time = pal_sub_time_us(next_beacon_tx_time,
                                                    TAL_CONVERT_SYMBOLS_TO_US(ADVNC_BCN_PREP_TIME));

            /* This is the timer started for preparing the beacon. */
            status = FAILURE;

            while (MAC_SUCCESS != status)
            {
                status = pal_timer_start(T_Beacon_Preparation,
                                         next_beacon_prep_time,
                                         TIMEOUT_ABSOLUTE,
                                         (FUNC_PTR)mac_t_prepare_beacon_cb,
                                         NULL);

                next_beacon_prep_time = pal_add_time_us(next_beacon_prep_time,
                                                        beacon_int_us);
            }
        }

        /* 3) Superframe timer for determining end of active portion. */
        /* TODO */
        /*
        if (tal_pib_SuperFrameOrder < tal_pib_BeaconOrder)
        {
                pal_timer_start(T_Superframe,
                                TAL_CONVERT_SYMBOLS_TO_US(
                                    TAL_GET_SUPERFRAME_DURATION_TIME(
                                        tal_pib_SuperFrameOrder)),
                                TIMEOUT_RELATIVE,
                                (FUNC_PTR)mac_t_superframe_cb,
                                NULL);
        }
        */

        /*
         * Once the timing calculation for the next beacon has been finished,
         * a pending broadcast frame will be transmitted.
         * Of course this is only done if the node is not scanning.
         */
        if (MAC_SCAN_IDLE == mac_scan_state)
        {
            /*
             * Check for pending broadcast data frames in the broadcast queue
             * and transmit exactly one broadcast data frame in case there
             * are pending broadcast frames.
             */
            if (broadcast_q.size > 0)
            {
                mac_tx_pending_bc_data();
            }
        }
    }   /* (tal_pib_BeaconOrder < NON_BEACON_NWK) */

    callback_parameter = callback_parameter;  /* Keep compiler happy. */
} /* mac_t_beacon_cb() */
#endif  /* BEACON_SUPPORT */



#ifdef BEACON_SUPPORT
/*
 * @brief Handles the superframe timer expiry
 *
 * This function is the callback function of the superframe timer.
 * This function is called once the inactive portion of a superframe
 * is entered.
 *
 * @param callback_parameter Callback parameter
 */
/* TODO */
//static void mac_t_superframe_cb(void *callback_parameter)
//{
//    /*
//     * Go to sleep (independent of the value of macRxOnWhenIdle)
//     * because we enter the incative portion now.
//     * Note: Do not use mac_sleep_trans() here, because this would check
//     * macRxOnWhenIdle first.
//     */
//    mac_trx_init_sleep();
//
//    callback_parameter = callback_parameter;  /* Keep compiler happy. */
//}

#endif /* BEACON_SUPPORT */



#ifdef BEACON_SUPPORT
/*
 * @brief Handles transmission of pending broadcast data
 *
 * This function handles the transmission of pending broadcast data
 * frames in a beacon-enabled network, which need to be transmitted
 * immediately after the beacon transmission.
 * As defined by 802.15.4-2006 exactly one broadcast data frame is
 * transmitted at one point of time. Further pending broadcast frames
 * are transmitted after the next beacon frame.
 */
void mac_tx_pending_bc_data(void)
{
    buffer_t *buf_ptr;
    frame_info_t *transmit_frame;
    broadcast_t *broadcast_frame;
    retval_t tal_tx_status;

    buf_ptr = qmm_queue_remove(&broadcast_q, NULL);

    ASSERT(buf_ptr != NULL);

    if (NULL == buf_ptr)
    {
        /* Nothing ot be done. */
        return;
    }

    /* Broadcast data present and to be sent. */
    broadcast_frame = (broadcast_t *)BMM_BUFFER_POINTER(buf_ptr);
    transmit_frame = broadcast_frame->data;

    /* Store the MSDU Handle for confirmation. */
    mac_msdu_handle = broadcast_frame->msduHandle;
    transmit_frame->buffer_header = buf_ptr;

    tal_tx_status = tal_tx_frame(transmit_frame, NO_CSMA_WITH_IFS, false);

    if (MAC_SUCCESS == tal_tx_status)
    {
        MAKE_MAC_BUSY();
    }
    else
    {
#if (DEBUG > 1)
        ASSERT("Broadcast data tx failed" == 0);
#endif
        mac_gen_mcps_data_conf((buffer_t *)transmit_frame->buffer_header,
                               (uint8_t)MAC_CHANNEL_ACCESS_FAILURE,
                               mac_msdu_handle,
                               0);
     }
}
#endif /* BEACON_SUPPORT */

#endif /* MAC_START_REQUEST_CONFIRM */

/* EOF */
