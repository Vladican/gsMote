/**
 * @file mac.h
 *
 * @brief Provides MAC API to access MAC Layer functionality.
 *
 * $Id: mac.h 19591 2009-12-18 11:04:49Z sschneid $
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
#ifndef _MAC_H
#define _MAC_H

/* === Includes ============================================================= */

#include <stdbool.h>
#include "tal.h"
#include "qmm.h"
#include "mac_build_config.h"

/* === Macros =============================================================== */

#if ((MAC_SCAN_ED_REQUEST_CONFIRM == 1)      || \
     (MAC_SCAN_ACTIVE_REQUEST_CONFIRM == 1)  || \
     (MAC_SCAN_PASSIVE_REQUEST_CONFIRM == 1) || \
     (MAC_SCAN_ORPHAN_REQUEST_CONFIRM == 1))
/**
 * Maximum allowed PANDescriptors that is calculated using the
 * large buffer size.
 */
#define MAX_ALLOWED_PAN_DESCRIPTORS     ((LARGE_BUFFER_SIZE - \
        sizeof(mlme_scan_conf_t)) / sizeof(wpan_pandescriptor_t))
/**
 * Active/passive scan: implementation-defined maximum number of
 * PANDescriptors that can be stored.
 */
#define MAX_PANDESCRIPTORS \
        (MAX_ALLOWED_PAN_DESCRIPTORS > 5 ? 5 : MAX_ALLOWED_PAN_DESCRIPTORS)
#endif

/**
 * Size of FCF in octets
 */
#define FCF_SIZE                        (2)

/*
 * Defines a mask for the frame type. (Table 65 IEEE 802.15.4 Specification)
 */
#define FCF_FRAMETYPE_MASK              (0x07)

/*
 * A macro to get the frame type.
 */
#define FCF_GET_FRAMETYPE(x)            ((x) & FCF_FRAMETYPE_MASK)

/*
 * Defines the mask for the FCF address mode
 */
#define FCF_ADDR_MASK                   (3)

/*
 * Macro to get the source address mode.
 */
#define FCF_GET_SOURCE_ADDR_MODE(x) \
    (((x) >> FCF_SOURCE_ADDR_OFFSET) & FCF_ADDR_MASK)

/*
 * Macro to get the destination address mode.
 */
#define FCF_GET_DEST_ADDR_MODE(x)\
    (((x) >> FCF_DEST_ADDR_OFFSET) & FCF_ADDR_MASK)

/* === Types ================================================================ */

/**
 * MAC Address type
 */
typedef union
{
    uint16_t short_address;
    uint64_t long_address;
} address_field_t;

/* === Externals ============================================================ */

extern queue_t nhle_mac_q;

/* === Prototypes =========================================================== */

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup MAC API
 *
 * This module describes all MAC APIs to access the MAC functionality.
 *
 */
/*@{*/

/* 802.15.4 MAC layer entries */
void mcps_data_request(uint8_t *msg);
void mcps_data_conf(uint8_t *m);
void mcps_data_ind(uint8_t *m);

#if ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_BASIC == 1))
void mcps_purge_request(uint8_t *msg);
void mcps_purge_conf(uint8_t *m);
#endif  /* ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_BASIC == 1)) */

#if (MAC_GET_SUPPORT == 1)
void mlme_get_request(uint8_t *msg);
void mlme_get_conf(uint8_t *m);
#endif  /* (MAC_GET_SUPPORT == 1) */
void mlme_reset_request(uint8_t *msg);
#if ((MAC_SCAN_ED_REQUEST_CONFIRM == 1) || (MAC_SCAN_ACTIVE_REQUEST_CONFIRM == 1)  || \
     (MAC_SCAN_PASSIVE_REQUEST_CONFIRM == 1) || (MAC_SCAN_ORPHAN_REQUEST_CONFIRM == 1))
void mlme_scan_request(uint8_t *msg);
void mlme_scan_conf(uint8_t *m);
#endif
#if (MAC_START_REQUEST_CONFIRM == 1)
void mlme_start_request(uint8_t *msg);
void mlme_start_conf(uint8_t *m);
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */
#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1)
void mlme_associate_request(uint8_t *m);
void mlme_associate_conf(uint8_t *m);
#endif  /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */
#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)
void mlme_associate_response(uint8_t *m);
void mlme_associate_ind(uint8_t *m);
#endif /* (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) */
#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)
void mlme_disassociate_request(uint8_t *m);
void mlme_disassociate_conf(uint8_t *m);
void mlme_disassociate_ind(uint8_t *m);
#endif  /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) */
#if (MAC_ORPHAN_INDICATION_RESPONSE == 1)
void mlme_orphan_response(uint8_t *m);
void mlme_orphan_ind(uint8_t *m);
#endif /* (MAC_ORPHAN_INDICATION_RESPONSE == 1) */
#if (MAC_INDIRECT_DATA_BASIC == 1)
void mlme_poll_request(uint8_t *m);
void mlme_poll_conf(uint8_t *m);
#endif  /* (MAC_INDIRECT_DATA_BASIC == 1) */
#if (MAC_RX_ENABLE_SUPPORT == 1)
void mlme_rx_enable_request(uint8_t *m);
void mlme_rx_enable_conf(uint8_t *m);
#endif  /* (MAC_RX_ENABLE_SUPPORT == 1) */
void mlme_sync_request(uint8_t *m);
#if (MAC_BEACON_NOTIFY_INDICATION == 1)
void mlme_beacon_notify_ind(uint8_t *m);
#endif  /* (MAC_BEACON_NOTIFY_INDICATION == 1) */
#if ((MAC_ORPHAN_INDICATION_RESPONSE == 1) || (MAC_ASSOCIATION_INDICATION_RESPONSE == 1))
void mlme_comm_status_ind(uint8_t *m);
#endif  /* ((MAC_ORPHAN_INDICATION_RESPONSE == 1) || (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)) */

void mlme_reset_conf(uint8_t *m);
void mlme_sync_loss_ind(uint8_t *m);

#if (HIGHEST_STACK_LAYER == MAC)
void mlme_set_request(uint8_t *msg);
void mlme_set_conf(uint8_t *m);
#endif  /* (HIGHEST_STACK_LAYER == MAC) */
retval_t mlme_set(uint8_t attribute, pib_value_t *attribute_value, bool set_trx_to_sleep);

retval_t mac_init(void);
bool mac_task(void);


/*@}*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _MAC_H */
/* EOF */
