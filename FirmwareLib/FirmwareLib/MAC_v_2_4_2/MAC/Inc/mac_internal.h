/**
 * @file mac_internal.h
 *
 * @brief Declares MAC internal functions, globals, and macros.
 *
 * $Id: mac_internal.h 20202 2010-02-08 09:32:20Z sschneid $
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
#ifndef MAC_INTERNAL_H
#define MAC_INTERNAL_H

/* === Includes ============================================================= */

#include "pal.h"
#include "ieee_const.h"
#include "mac_data_structures.h"

#if (DEBUG > 0)
/* Needs to be included for make_mac_disp_not_busy() while debugging */
#include "tal_internal.h"
#endif

/* === Macros =============================================================== */

/*
 * Beacon order used as timer interval for checking the expiration of indirect
 * transactions in a nonbeacon-enabled network
 */
#define BO_USED_FOR_MAC_PERS_TIME       (3)

/*
 * Final CAP slot in standard superframe without GTS
 */
#define FINAL_CAP_SLOT_DEFAULT          (0x0F)

/* === Types ================================================================ */

/**
 * MAC state type.
 */
typedef enum
{
/*
 * IEEE 802.15.4-defined MAC states.
 */
    /**
     * Node is idle,
     * i.e. it is neither associated nor has started its own network
     */
    MAC_IDLE = 0,

    /* Device has successfully associated */
    MAC_ASSOCIATED = 1,

    /**
     * Coordinator successfully associated with PAN Coordinator
     * and successfully started network with same PAN-Id
     * (not as PAN Coordinator)
     */
    MAC_COORDINATOR = 2,

    /** PAN coordinator successfully started */
    MAC_PAN_COORD_STARTED = 3
} SHORTENUM mac_state_t;


/**
 * MAC poll states.
 * These states describe the current status of the MAC for polling
 * for devices or coordinators, not for PAN coordinator.
 */
typedef enum
{
    /**
     * No polling ongoing.
     */
    MAC_POLL_IDLE = 0,

    /**
     * Successful transmission of association request frame,
     * wait for Association response.
     */
    MAC_AWAIT_ASSOC_RESPONSE,

    /**
     * Explicit poll ongoing (MLME_POLL.request),
     * Ack after Data request frame transmission received,
     * awaiting data response. */
    MAC_POLL_EXPLICIT,

    /**
     * Implicit poll ongoing (more pending data detected, either in beacon or
     * data frame),
     * awaiting data response, */
    MAC_POLL_IMPLICIT
} SHORTENUM mac_poll_state_t;



/**
 * Device or coordinator scan states.
 */
typedef enum
{
    /**
     * No scanning ongoing.
     */
    MAC_SCAN_IDLE = 0,

    /* Scanning in progress. */
    /** ED scan ongoing */
    MAC_SCAN_ED,
    /** Active scan proceeding */
    MAC_SCAN_ACTIVE,
    /** Orphan scan proceeding */
    MAC_SCAN_ORPHAN,
    /** Passive scan proceeding */
    MAC_SCAN_PASSIVE
} SHORTENUM mac_scan_state_t;



/**
 * Device or coordinator sync states.
 */
typedef enum
{
    /** Do not track beacons */
    MAC_SYNC_NEVER = 0,
    /** Track the next beacon */
    MAC_SYNC_ONCE,
    /** Track beacons continuously */
    MAC_SYNC_TRACKING_BEACON,
    /**
     * Track beacons continuously before beeing associated in order to obtain
     * synchronization with desired network
     */
    MAC_SYNC_BEFORE_ASSOC
} SHORTENUM mac_sync_state_t;


/**
 * MAC sleep state type.
 */
typedef enum
{
    /**< Radio is awake */
    RADIO_AWAKE = 0,
    /**< Radio is in sleep mode */
    RADIO_SLEEPING
} SHORTENUM mac_radio_sleep_state_t;

typedef void (*handler_t)(uint8_t *);

/* === Externals ============================================================ */

/* Global data variables */
extern uint8_t *mac_conf_buf_ptr;
#ifdef BEACON_SUPPORT
extern uint8_t mac_final_cap_slot;
extern bool mac_bc_data_indicated;
#endif  /* BEACON_SUPPORT */
extern uint8_t mac_last_dsn;
extern uint64_t mac_last_src_addr;
extern uint8_t mac_msdu_handle;
extern parse_t mac_parse_data;
extern mac_radio_sleep_state_t mac_radio_sleep_state;
extern bool mac_busy;
extern bool mac_rx_enabled;
extern mac_state_t mac_state;
extern mac_scan_state_t mac_scan_state;
extern mac_sync_state_t mac_sync_state;
extern mac_poll_state_t mac_poll_state;

#if (MAC_SCAN_SUPPORT == 1)
extern uint8_t *mac_scan_cmd_buf_ptr;
extern uint8_t mac_scan_orig_channel;
extern uint8_t mac_scan_orig_page;
#if ((MAC_SCAN_ACTIVE_REQUEST_CONFIRM == 1) || (MAC_SCAN_PASSIVE_REQUEST_CONFIRM == 1))
extern uint16_t mac_scan_orig_panid;
#endif  /* ((MAC_SCAN_ACTIVE_REQUEST_CONFIRM == 1) || (MAC_SCAN_PASSIVE_REQUEST_CONFIRM == 1)) */
#endif /* (MAC_SCAN_SUPPORT == 1) */

#if (MAC_START_REQUEST_CONFIRM == 1)
extern uint8_t mac_beacon_payload[];
#endif  /* (MAC_START_REQUEST_CONFIRM == 1) */

extern queue_t tal_mac_q;

#if (MAC_INDIRECT_DATA_FFD == 1)
extern indirect_data_pers_timer_t mac_indirect_array[];
extern bool mac_indirect_flag;
extern queue_t indirect_data_q;
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

#if (MAC_START_REQUEST_CONFIRM == 1)
#ifdef BEACON_SUPPORT
extern queue_t broadcast_q;
#endif  /* BEACON_SUPPORT */
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */

/* MAC PIB variables */
#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)
extern uint8_t mac_pib_macAssociationPermit;
#endif /* (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) */

#if (MAC_START_REQUEST_CONFIRM == 1)
extern uint8_t mac_pib_macBeaconPayloadLength;
extern uint8_t mac_pib_macBSN;
#endif  /* (MAC_START_REQUEST_CONFIRM == 1) */

#if (MAC_INDIRECT_DATA_FFD == 1)
extern uint16_t mac_pib_macTransactionPersistenceTime;
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1)
extern uint8_t mac_pib_macAssociatedPANCoord;
#endif /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */

extern uint8_t mac_pib_macAutoRequest;
extern uint8_t mac_pib_macBattLifeExtPeriods;
extern uint64_t mac_pib_macCoordExtendedAddress;
extern uint16_t mac_pib_macCoordShortAddress;
extern uint8_t mac_pib_macDSN;
#if ((MAC_INDIRECT_DATA_BASIC == 1) || defined(BEACON_SUPPORT))
extern uint16_t mac_pib_macMaxFrameTotalWaitTime;
#endif  /* ((MAC_INDIRECT_DATA_BASIC == 1) || defined(BEACON_SUPPORT)) */
extern uint16_t mac_pib_macResponseWaitTime;
extern bool mac_pib_macRxOnWhenIdle;
extern uint8_t mac_pib_macSecurityEnabled;

#ifdef TEST_HARNESS
extern uint8_t mac_pib_privateIllegalFrameType;
extern uint8_t mac_pib_privateNoDataAfterAssocReq;
extern uint8_t mac_pib_privateTransactionOverflow;
extern uint8_t mac_pib_privateVirtualPANs;
#endif /* TEST_HARNESS */

/* === Prototypes =========================================================== */

#ifdef __cplusplus
extern "C" {
#endif
/** \defgroup MAC MAC internal functions
 *
 * This module describes all MAC internal functions.
 *
 */
/*@{*/

/* MAC-internal functions -- please keep them alphabetically sorted. */

#if (MAC_START_REQUEST_CONFIRM == 1)
#ifdef BEACON_SUPPORT
void mac_build_and_tx_beacon(bool beacon_enabled);
#else /* No BEACON_SUPPORT */
void mac_build_and_tx_beacon(bool beacon_enabled,
                             buffer_t *beacon_buffer_header);
#endif /* BEACON_SUPPORT / No BEACON_SUPPORT */
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */

#if (MAC_INDIRECT_DATA_BASIC == 1)
bool mac_build_and_tx_data_req(bool expl_poll,
                               bool force_own_long_addr,
                               uint8_t expl_dest_addr_mode,
                               address_field_t *expl_dest_addr,
                               uint16_t expl_dest_pan_id);
#endif /* (MAC_INDIRECT_DATA_BASIC == 1) */

#if (MAC_START_REQUEST_CONFIRM == 1)
void mac_coord_realignment_command_tx_success(uint8_t tx_status,
                                              buffer_t *buf);
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */

void mac_gen_mcps_data_conf(buffer_t *buf,
                            uint8_t status,
                            uint8_t handle,
                            uint32_t timestamp);

#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1)
void mac_gen_mlme_associate_conf(buffer_t *buf,
                                 uint8_t status,
                                 uint16_t assoc_short_addr);
#endif /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */

uint8_t mac_get_pib_attribute_size(uint8_t pib_attribute_id);

#if (MAC_INDIRECT_DATA_FFD == 1)
void mac_handle_tx_null_data_frame(void);
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

void mac_idle_trans(void);

#if (MAC_COMM_STATUS_INDICATION == 1)
void mac_mlme_comm_status(uint8_t srcAddrMode,
                          uint64_t *srcAddr,
                          uint8_t dstAddrMode,
                          uint64_t *dstAddr,
                          uint8_t status,
                          buffer_t *buf);
#endif /* (MAC_COMM_STATUS_INDICATION == 1) */

#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)
void mac_prep_disassoc_conf(buffer_t *buf,
                            uint8_t status,
                            frame_info_t *frame_ptr);
#endif /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) */

#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)
void mac_process_associate_request(buffer_t *assoc_req);
#endif /* (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) */

#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1)
void mac_process_associate_response(buffer_t *assoc_resp);
#endif /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */

#if ((MAC_SCAN_SUPPORT == 1) || (MAC_SYNC_REQUEST == 1))
void mac_process_beacon_frame(buffer_t *msg);
#endif /* ((MAC_SCAN_SUPPORT == 1) || (MAC_SYNC_REQUEST == 1)) */

#if (MAC_START_REQUEST_CONFIRM == 1)
void mac_process_beacon_request(buffer_t *msg);
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */

#if (MAC_SYNC_LOSS_INDICATION == 1)
void mac_process_coord_realign(buffer_t *ind);
void mac_sync_loss(uint8_t loss_reason);
#endif /* (MAC_SYNC_LOSS_INDICATION == 1) */

#if (MAC_SCAN_ORPHAN_REQUEST_CONFIRM == 1)
void mac_process_orphan_realign(buffer_t *ind);
#endif /* (MAC_SCAN_ORPHAN_REQUEST_CONFIRM == 1) */

#if (MAC_INDIRECT_DATA_FFD == 1)
void mac_process_data_request(buffer_t *msg);
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

#if (MAC_INDIRECT_DATA_BASIC == 1)
void mac_process_data_response(void);
#endif /* (MAC_INDIRECT_DATA_BASIC == 1) */

void mac_process_data_frame(buffer_t *data_ind);

#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)
void mac_process_disassociate_notification(buffer_t *msg);
#endif /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) */

#if (MAC_ORPHAN_INDICATION_RESPONSE == 1)
void mac_process_orphan_notification(buffer_t *msg);
#endif /* (MAC_ORPHAN_INDICATION_RESPONSE == 1) */

void mac_process_tal_data_ind(uint8_t *msg);

void mac_sleep_trans(void);

#if ((MAC_SCAN_ACTIVE_REQUEST_CONFIRM == 1) || (MAC_SCAN_ORPHAN_REQUEST_CONFIRM == 1))
void mac_scan_send_complete(retval_t status);
#endif /* ((MAC_SCAN_ACTIVE_REQUEST_CONFIRM == 1) || (MAC_SCAN_ORPHAN_REQUEST_CONFIRM == 1)) */

#ifdef BEACON_SUPPORT
#if (MAC_START_REQUEST_CONFIRM == 1)
void mac_start_beacon_timer(void);
#endif  /* (MAC_START_REQUEST_CONFIRM == 1) */
#endif  /* BEACON_SUPPORT */

#if (MAC_INDIRECT_DATA_FFD == 1)
void mac_start_persistence_timer(void);
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

#if (MAC_SYNC_REQUEST == 1)
void mac_start_missed_beacon_timer(void);
#endif /* (MAC_SYNC_REQUEST == 1) */

#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1)
void mac_t_assocresponsetime_cb(void *callback_parameter);
#endif /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */

#if ((MAC_ORPHAN_INDICATION_RESPONSE == 1) || (MAC_START_REQUEST_CONFIRM == 1))
bool mac_tx_coord_realignment_command(frame_msgtype_t cmd_type,
                                      buffer_t *buf,
                                      uint16_t new_panid,
                                      uint8_t new_channel,
                                      uint8_t new_page);
#endif /* ((MAC_ORPHAN_INDICATION_RESPONSE == 1) || (MAC_START_REQUEST_CONFIRM == 1)) */

#ifdef BEACON_SUPPORT
#if (MAC_START_REQUEST_CONFIRM == 1)
void mac_tx_pending_bc_data(void);
#endif  /* (MAC_START_REQUEST_CONFIRM == 1) */
#endif  /* BEACON_SUPPORT */

void mac_trx_init_sleep(void);
void mac_trx_wakeup(void);

/* Timer callbacks */
#if (MAC_INDIRECT_DATA_BASIC == 1)
void mac_t_poll_wait_time_cb(void *callback_parameter);
#endif /* (MAC_INDIRECT_DATA_BASIC == 1) */
#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1)
void mac_t_response_wait_cb(void *callback_parameter);
#endif /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */
#if (MAC_SYNC_REQUEST == 1)
void mac_t_start_inactive_device_cb(void *callback_parameter);
void mac_t_tracking_beacons_cb(void *callback_parameter);
#endif /* (MAC_SYNC_REQUEST == 1) */

#if (MAC_INDIRECT_DATA_FFD == 1)
uint8_t find_buffer_cb(void *buf, void *address);
void add_persistence_time(uint8_t *buffer);
void rearrange_persistence_array();
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */

void dispatch_event(uint8_t *event);

retval_t set_tal_pib_internal(uint8_t attribute, pib_value_t *attribute_value);

#if (MAC_INDIRECT_DATA_FFD == 1)
/*
 * @brief Checks whether the Persistence timer needs to be started
 *
 * If an FFD does have pending data, the MAC persistence timer
 * needs to be started.
 */
static inline void mac_check_persistence_timer(void)
{
    if (!pal_is_timer_running(T_Data_Persistence))
    {
        mac_start_persistence_timer();
    }
}
#endif /* (MAC_INDIRECT_DATA_FFD == 1) */



/**
 * This macro sets the MAC to busy
 */
#if (DEBUG > 0)
#define MAKE_MAC_BUSY()             do {        \
    if (mac_busy)                               \
    {                                           \
        ASSERT("MAC is already busy" == 0);     \
    }                                           \
    mac_busy = true;                            \
} while (0)
#else
#define MAKE_MAC_BUSY()             do {        \
    mac_busy = true;                            \
} while (0)
#endif



/**
 * This macro sets the MAC to not busy
 */
#if (DEBUG > 0)
#define MAKE_MAC_NOT_BUSY()         do {        \
    if (!mac_busy)                              \
    {                                           \
        ASSERT("MAC was not busy" == 0);        \
    }                                           \
    mac_busy = false;                           \
} while (0)
#else
#define MAKE_MAC_NOT_BUSY()         do {        \
    mac_busy = false;                           \
} while (0)
#endif

/*@}*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MAC_INTERNAL_H */
/* EOF */
