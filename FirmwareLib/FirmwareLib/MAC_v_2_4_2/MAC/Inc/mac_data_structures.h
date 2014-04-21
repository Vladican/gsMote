/**
 * @file mac_data_structures.h
 *
 * @brief This file contains MAC related data structures, types and enums.
 *
 * $Id: mac_data_structures.h 16325 2009-06-23 16:40:23Z sschneid $
 *
 * @author    Atmel Corporation: http://www.atmel.com
 * @author    Support email: avr@atmel.com
 */
/*
 * Copyright (c) 2009, Atmel Corporation All rights reserved.
 *
 * Licensed under Atmel's Limited License Agreement --> EULA.txt
 */

/* Prevent double inclusion */
#ifndef MAC_DATA_STRUCTURES_H
#define MAC_DATA_STRUCTURES_H

/* === Includes ============================================================= */

#include "mac.h"

/* === Macros =============================================================== */

/**
 * Maximal number of pending addresses in beacon frame.
 */
#define MAX_PENDING_ADDR                    (7)

/* === Types ================================================================ */

/**
 * Beacon Payload type
 */
typedef struct mac_beacon_payload_tag
{
    uint16_t superframe_spec;
    uint8_t gts_spec;
    uint8_t pending_addr_spec;
    uint16_t pending_addr_list_short[MAX_PENDING_ADDR];
    uint64_t pending_addr_list_long[MAX_PENDING_ADDR];
    uint8_t payload[aMaxBeaconPayloadLength];
} mac_beacon_payload_t;



/**
 * Data Payload type
 */
typedef struct mac_data_payload_tag
{
    /*
     * Use aMaxPHYPacketSize rather than aMaxMACFrameSize for frames
     * captured in promiscuous mode.
     */
    uint8_t payload[aMaxPHYPacketSize];
} mac_data_payload_t;



/**
 * Association Request type
 */
typedef struct mac_assoc_req_tag
{
    uint8_t capability_info;
} mac_assoc_req_t;

/**
 * Association Response type
 */
typedef struct mac_assoc_response_tag
{
    uint16_t short_addr;
    uint8_t assoc_status;
} mac_assoc_response_t;



/**
 * Disassociation Request type
 */
typedef struct mac_disassoc_req_tag
{
    uint8_t disassoc_reason;
} mac_disassoc_req_t;

/**
 * Coordinator Realignment type
 */
typedef struct mac_coord_realign_tag
{
    uint16_t pan_id;
    uint16_t coord_short_addr;
    uint8_t logical_channel;
    uint16_t short_addr;
    uint8_t channel_page;
} mac_coord_realign_t;



/**
 * General Command frame payload type
 */
typedef union
{
    mac_beacon_payload_t beacon_data;
    mac_data_payload_t data;
    mac_assoc_req_t assoc_req_data;
    mac_assoc_response_t assoc_response_data;
    mac_disassoc_req_t disassoc_req_data;
    mac_coord_realign_t coord_realign_data;
} frame_payload_t;



/**
 * This structure is used to form a table of offsets into the frame
 * (mostly into the MHR of the frame), indexed by the addressing bits
 * of the FCF.
 *
 * Possible values for each field are indicated in the per-field
 * comments. Offset 0 means this field is not present (as offset 0
 * into the frame would point to the FCF).
 */
typedef struct
{
    /** Offset of destination PANId within frame */
    uint8_t dest_panid: 3;      /* 0 or 3 */
    /** Offset of destination address (short or long) within frame */
    uint8_t dest_addr:  4;      /* 0 or 5 */
    /** Offset of source PANId within frame */
    uint8_t src_panid: 4;       /* 0, 3, 7, or 13 */
    /** Offset of source address (short or long) within frame */
    uint8_t src_addr:  4;       /* 0, 5, 7, 9, 13, or 15 */
    /** Offset of MAC payload within frame */
    uint8_t payload: 5;         /* 0, 3, 7, 9, 11, 13, 15, 17, 21, or 23 */
} mhr_offset_t;



typedef struct parse_tag
{
    uint16_t fcf;
    uint8_t frame_type;
    uint8_t frame_length;
    uint8_t sequence_number;
    uint8_t dest_addr_mode;
    uint16_t dest_panid;
    address_field_t dest_addr;
    uint8_t src_addr_mode;
    uint16_t src_panid;
    address_field_t src_addr;
    uint8_t mac_command;
    uint8_t ppduLinkQuality;
    uint32_t timestamp;
    uint8_t payload_index;
    uint8_t payload_length;
    frame_payload_t payload_data;
    mhr_offset_t mhro;
} parse_t;

/* === Externals ============================================================ */


/* === Prototypes =========================================================== */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MAC_DATA_STRUCTURES_H */
/* EOF */
