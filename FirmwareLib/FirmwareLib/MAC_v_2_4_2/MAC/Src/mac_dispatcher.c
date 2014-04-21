/**
 * @file mac_dispatcher.c
 *
 * @brief Dispatches the events by decoding the message type
 *
 * $Id: mac_dispatcher.c 20086 2010-01-29 14:02:30Z sschneid $
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

#include <stddef.h>
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

/* === Macros ============================================================== */

#define CMD_ID_OCTET        (0)

/* === Globals ============================================================= */

#if (HIGHEST_STACK_LAYER == MAC)
static FLASH_DECLARE(const handler_t dispatch_table[LAST_MESSAGE + 1]) =
{
    /* Internal message */
    [MLME_RESET_REQUEST]                  = mlme_reset_request,

#if (MAC_GET_SUPPORT == 1)
    [MLME_GET_REQUEST]                    = mlme_get_request,
#endif  /* (MAC_GET_SUPPORT == 1) */

    [MLME_SET_REQUEST]                    = mlme_set_request,

#if (MAC_SCAN_SUPPORT == 1)
    [MLME_SCAN_REQUEST]                   = mlme_scan_request,
#endif /* (MAC_SCAN_SUPPORT == 1) */

#if (MAC_START_REQUEST_CONFIRM == 1)
    [MLME_START_REQUEST]                  = mlme_start_request,
#endif /* (MAC_START_REQUEST_CONFIRM== 1) */

#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1)
    [MLME_ASSOCIATE_REQUEST]              = mlme_associate_request,
#endif /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */

#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)
    [MLME_ASSOCIATE_RESPONSE]             = mlme_associate_response,
#endif /* (MAC_ASSOCIATION_INDICATION_RESPONSE== 1) */

    [MCPS_DATA_REQUEST]                   = mcps_data_request,

#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)
    [MLME_DISASSOCIATE_REQUEST]           = mlme_disassociate_request,
#endif  /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) */

#if (MAC_ORPHAN_INDICATION_RESPONSE == 1)
    [MLME_ORPHAN_RESPONSE]                = mlme_orphan_response,
#endif /* (MAC_ORPHAN_INDICATION_RESPONSE == 1) */

#if (MAC_INDIRECT_DATA_BASIC == 1)
    [MLME_POLL_REQUEST]                   = mlme_poll_request,
#endif /* (MAC_INDIRECT_DATA_BASIC == 1) */

#if (MAC_RX_ENABLE_SUPPORT == 1)
    [MLME_RX_ENABLE_REQUEST]              = mlme_rx_enable_request,
#endif /* (MAC_RX_ENABLE_SUPPORT == 1) */

#if (MAC_SYNC_REQUEST == 1)
    [MLME_SYNC_REQUEST]                   = mlme_sync_request,
#endif /* (MAC_SYNC_REQUEST == 1) */

#if ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_BASIC == 1))
    [MCPS_PURGE_REQUEST]                  = mcps_purge_request,
#endif /* ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_BASIC == 1)) */

    [TAL_DATA_INDICATION]                 = mac_process_tal_data_ind,
    [MCPS_DATA_CONFIRM]                   = mcps_data_conf,
    [MCPS_DATA_INDICATION]                = mcps_data_ind,

#if ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_BASIC == 1))
    [MCPS_PURGE_CONFIRM]                  = mcps_purge_conf,
#endif /* ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_BASIC == 1)) */

#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)
    [MLME_ASSOCIATE_INDICATION]           = mlme_associate_ind,
#endif /* (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) */

#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1)
    [MLME_ASSOCIATE_CONFIRM]              = mlme_associate_conf,
#endif /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */

#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)
    [MLME_DISASSOCIATE_INDICATION]        = mlme_disassociate_ind,
    [MLME_DISASSOCIATE_CONFIRM]           = mlme_disassociate_conf,
#endif  /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) */

#if (MAC_BEACON_NOTIFY_INDICATION == 1)
    [MLME_BEACON_NOTIFY_INDICATION]       = mlme_beacon_notify_ind,
#endif /* (MAC_BEACON_NOTIFY_INDICATION == 1) */

#if (MAC_ORPHAN_INDICATION_RESPONSE == 1)
    [MLME_ORPHAN_INDICATION]              = mlme_orphan_ind,
#endif /* (MAC_ORPHAN_INDICATION_RESPONSE == 1) */

#if (MAC_SCAN_SUPPORT == 1)
    [MLME_SCAN_CONFIRM]                   = mlme_scan_conf,
#endif /* (MAC_SCAN_SUPPORT == 1) */

#if (MAC_COMM_STATUS_INDICATION == 1)
    [MLME_COMM_STATUS_INDICATION]         = mlme_comm_status_ind,
#endif /* (MAC_COMM_STATUS_INDICATION == 1) */

#if MAC_SYNC_LOSS_INDICATION == 1
    [MLME_SYNC_LOSS_INDICATION]           = mlme_sync_loss_ind,
#endif /* (MAC_SYNC_LOSS_INDICATION == 1) */

#if (MAC_GET_SUPPORT == 1)
    [MLME_GET_CONFIRM]                    = mlme_get_conf,
#endif  /* (MAC_GET_SUPPORT == 1) */

    [MLME_SET_CONFIRM]                    = mlme_set_conf,
    [MLME_RESET_CONFIRM]                  = mlme_reset_conf,

#if (MAC_RX_ENABLE_SUPPORT == 1)
    [MLME_RX_ENABLE_CONFIRM]              = mlme_rx_enable_conf,
#endif /* (MAC_RX_ENABLE_SUPPORT == 1) */

#if (MAC_START_REQUEST_CONFIRM == 1)
    [MLME_START_CONFIRM]                  = mlme_start_conf,
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */

#if (MAC_INDIRECT_DATA_BASIC == 1)
    [MLME_POLL_CONFIRM]                   = mlme_poll_conf,
#endif /* (MAC_INDIRECT_DATA_BASIC == 1) */
};
#else
static FLASH_DECLARE(handler_t dispatch_table[LAST_MESSAGE + 1]) =
{
    /* Internal message */
    [MLME_RESET_REQUEST]                  = mlme_reset_request,

#if (MAC_GET_SUPPORT == 1)
    [MLME_GET_REQUEST]                    = mlme_get_request,
#endif  /* (MAC_GET_SUPPORT == 1) */

#if (MAC_SCAN_SUPPORT == 1)
    [MLME_SCAN_REQUEST]                   = mlme_scan_request,
#endif /* (MAC_SCAN_SUPPORT == 1) */

#if (MAC_START_REQUEST_CONFIRM == 1)
    [MLME_START_REQUEST]                  = mlme_start_request,
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */

#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1)
    [MLME_ASSOCIATE_REQUEST]              = mlme_associate_request,
#endif /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */

#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)
    [MLME_ASSOCIATE_RESPONSE]             = mlme_associate_response,
#endif /* (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) */

    [MCPS_DATA_REQUEST]                   = mcps_data_request,

#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)
    [MLME_DISASSOCIATE_REQUEST]           = mlme_disassociate_request,
#endif  /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) */

#if (MAC_ORPHAN_INDICATION_RESPONSE == 1)
    [MLME_ORPHAN_RESPONSE]                = mlme_orphan_response,
#endif /* (MAC_ORPHAN_INDICATION_RESPONSE == 1) */

#if (MAC_INDIRECT_DATA_BASIC == 1)
    [MLME_POLL_REQUEST]                   = mlme_poll_request,
#endif /* (MAC_INDIRECT_DATA_BASIC == 1) */

#if (MAC_RX_ENABLE_SUPPORT == 1)
    [MLME_RX_ENABLE_REQUEST]              = mlme_rx_enable_request,
#endif /* (MAC_RX_ENABLE_SUPPORT == 1) */

#if (MAC_SYNC_REQUEST == 1)
    [MLME_SYNC_REQUEST]                   = mlme_sync_request,
#endif /* (MAC_SYNC_REQUEST == 1) */

#if ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_BASIC == 1))
    [MCPS_PURGE_REQUEST]                  = mcps_purge_request,
#endif /* ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_BASIC == 1)) */

    [TAL_DATA_INDICATION]                 = mac_process_tal_data_ind,
};
#endif /* #if (HIGHEST_STACK_LAYER == MAC) */

/* === Prototypes ========================================================== */


/* === Implementation ====================================================== */

/**
 * @brief Obtains the message type from the buffer and calls respective handler
 *
 * This function decodes all events/messages and calls the appropriate handler.
 *
 * @param event Pointer to the buffer header whose body part holds the message
 * type and message elemnets
 */
void dispatch_event(uint8_t *event)
{
    /*
     * A pointer to the body of the buffer is obtained from the pointer to the
     * received header.
     */
    uint8_t *buffer = BMM_BUFFER_POINTER((buffer_t *)event);

    /* Check is done to see if the message type is valid */
    if (buffer[CMD_ID_OCTET] <= LAST_MESSAGE)
    {
        /*
         * The following statement reads the address from the dispatch table
         * of the function to be called by utilizing funicton pointers.
         * The macro PGM_READ_WORD is only relevant for AVR-GCC builds and
         * reads a DWord from flash, which is the start address of the function.
         *
         * How does this work for builds that are larger than 128K?
         *
         * For IAR builds this statement is fine, since PGM_READ_WORD just
         * turns to "*". The size of the function pointer is automatically
         * 3 byte for MCUs which have more than 128K flash. The the address
         * of the callback is properly derived from this location.
         *
         * AVR-GCC currently does not support function pointers larger than
         * 16 bit. This implies that functions residing in the upper 128K
         * cannot be addressed properly. Therefore this code does not work
         * in these cases.
         * In regular cases, where code is not larger than 128K, the size
         * of a function pointer is 16 bit and properly read via PGM_READ_WORD.
         */
        handler_t handler = (handler_t)PGM_READ_WORD(&dispatch_table[buffer[CMD_ID_OCTET]]);

        if (handler != NULL)
        {
            handler(event);
        }
        else
        {
            bmm_buffer_free((buffer_t *)event);
#if (DEBUG > 1)
            ASSERT("Dispatch handler unavailable" == 0);
#endif
        }
    }
}
/* EOF */

