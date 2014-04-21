/**
 * @file mac_api.c
 *
 * @brief This file contains MAC API functions.
 *
 * $Id: mac_api.c 19779 2010-01-07 08:55:55Z sschneid $
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
#include "ieee_const.h"
#include "mac_msg_const.h"
#include "mac_api.h"
#include "mac_msg_types.h"
#if (MAC_INDIRECT_DATA_FFD == 1)
#include "indirect_data_structures.h"
#endif  /* (MAC_INDIRECT_DATA_FFD == 1) */
#include "stack_config.h"
#include "mac.h"
#include "mac_build_config.h"
#include "pal.h"
#include "mac_internal.h"

/* === Types =============================================================== */


/* === Macros ============================================================== */


/* === Globals ============================================================= */

/**
 * Queue used by MAC for communication to next higher layer.
 */
queue_t mac_nhle_q;

/* === Prototypes ========================================================== */


/* === Implementation ====================================================== */

retval_t wpan_init(void)
{
    /* Init queue used for MAC to next higher layer communication */
    qmm_queue_init(&mac_nhle_q, MAC_NHLE_QUEUE_CAPACITY);

    /*
     * Initialize MAC.
     */
    return mac_init();
}



bool wpan_task(void)
{
    bool event_processed;
    uint8_t *event = NULL;

    /* mac_task returns true if a request was processed completely */
    event_processed = mac_task();

    /*
     * MAC to NHLE event queue should be dispatched
     * irrespective of the dispatcher state.
     */
    event = (uint8_t *)qmm_queue_remove(&mac_nhle_q, NULL);

    /* If an event has been detected, handle it. */
    if (NULL != event)
    {
        dispatch_event(event);
        event_processed = true;
    }

    tal_task();
    pal_task();

    return (event_processed);
}



/* MAC level API */

bool wpan_mcps_data_req(uint8_t SrcAddrMode,
                        wpan_addr_spec_t *DstAddrSpec,
                        uint8_t msduLength,
                        uint8_t *msdu,
                        uint8_t msduHandle,
                        uint8_t TxOptions)
{
    buffer_t *buffer_header;
    mcps_data_req_t *mcps_data_req;
    uint8_t *payload_pos;

    if (msduLength > aMaxMACPayloadSize)
    {
        /* Frame is too long and thus rejected immediately */
        return false;
    }

    /* Allocate a large buffer for mcps data request */
    buffer_header = bmm_buffer_alloc(LARGE_BUFFER_SIZE);

    if (NULL == buffer_header)
    {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mcps_data_req = (mcps_data_req_t *)BMM_BUFFER_POINTER(buffer_header);

    /* Construct mcps_data_req_t message */
    mcps_data_req->cmdcode = MCPS_DATA_REQUEST;

    /* Source addr mode */
    mcps_data_req->SrcAddrMode = SrcAddrMode;

    /* Destination addr spec */
    mcps_data_req->DstAddrMode = DstAddrSpec->AddrMode;
    mcps_data_req->DstPANId = DstAddrSpec->PANId;
    if (WPAN_ADDRMODE_SHORT == mcps_data_req->DstAddrMode)
    {
        /*
         * In case a short address is indicated, but the address is not
         * properly set, the entire address is first cleared.
         */
        mcps_data_req->DstAddr = 0;
        ADDR_COPY_DST_SRC_16(mcps_data_req->DstAddr, DstAddrSpec->Addr.short_address);
    }
    else
    {
        ADDR_COPY_DST_SRC_64(mcps_data_req->DstAddr, DstAddrSpec->Addr.long_address);
    }

    /* Other fields */
    mcps_data_req->msduHandle = msduHandle;
    mcps_data_req->TxOptions = TxOptions;
    mcps_data_req->msduLength = msduLength;
    mcps_data_req->data[0] = msduLength;

    /* Find the position where the data payload is to be updated */
    payload_pos = ((uint8_t *)mcps_data_req) + (LARGE_BUFFER_SIZE - FCF_SIZE - msduLength);

    /* Copy the payload to the end of buffer */
    memcpy(payload_pos, msdu, msduLength);

    if (MAC_SUCCESS == qmm_queue_append(&nhle_mac_q, buffer_header))
    {
        /*
         * MCPS-DATA.request is appended into NHLE MAC
         * queue successfully
         */
        return true;
    }
    else
    {
        /*
         * MCPS-DATA.request is not appended into NHLE MAC
         * queue, hence free the buffer allocated and return false
         */
        bmm_buffer_free(buffer_header);
        return false;
    }
}



#if ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_BASIC == 1))
bool wpan_mcps_purge_req(uint8_t msduHandle)
{
    buffer_t *buffer_header;
    mcps_purge_req_t *mcps_purge_req;

    /* Allocate small buffer for mcps purge request */
    buffer_header = bmm_buffer_alloc(LARGE_BUFFER_SIZE);

    if (NULL == buffer_header)
    {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mcps_purge_req = (mcps_purge_req_t *)BMM_BUFFER_POINTER(buffer_header);

    /* Update the purge request structure */
    mcps_purge_req->cmdcode = MCPS_PURGE_REQUEST;
    mcps_purge_req->msduHandle = msduHandle;

    if (MAC_SUCCESS == qmm_queue_append(&nhle_mac_q, buffer_header))
    {
        /*
         * MCPS-PURGE.request is appended into NHLE MAC
         * queue successfully
         */
        return true;
    }
    else
    {
        /*
         * MCPS-PURGE.request is not appended into NHLE MAC
         * queue, hence free the buffer allocated and return false
         */
        bmm_buffer_free(buffer_header);
        return false;
    }
}
#endif /* ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_BASIC == 1)) */



#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1)
bool wpan_mlme_associate_req(uint8_t LogicalChannel,
                             uint8_t ChannelPage,
                             wpan_addr_spec_t *CoordAddrSpec,
                             uint8_t CapabilityInformation)
{
    buffer_t *buffer_header;
    mlme_associate_req_t *mlme_associate_req;

    /* Allocate a buffer for mlme associate request */
    buffer_header = bmm_buffer_alloc(LARGE_BUFFER_SIZE);

    /* Check for buffer availability */
    if (NULL == buffer_header)
    {
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_associate_req = (mlme_associate_req_t*)BMM_BUFFER_POINTER(buffer_header);

    /* Construct mlme_associate_req_t message */
    mlme_associate_req->cmdcode = MLME_ASSOCIATE_REQUEST;

    /* Operating channel */
    mlme_associate_req->LogicalChannel = LogicalChannel;

    /* Coordinator address spec */
    mlme_associate_req->CoordAddrMode = CoordAddrSpec->AddrMode;
    mlme_associate_req->CoordPANId = CoordAddrSpec->PANId;
    ADDR_COPY_DST_SRC_64(mlme_associate_req->CoordAddress.long_address, CoordAddrSpec->Addr.long_address);

    /* Other fields */
    mlme_associate_req->CapabilityInformation = CapabilityInformation;
    mlme_associate_req->ChannelPage = ChannelPage;

    /* Insert service message into NHLE MLME queue */
    if (MAC_SUCCESS == qmm_queue_append(&nhle_mac_q, buffer_header))
    {
        /*
         * MLME-ASSOCIATE.request is appended into NHLE MAC
         * queue successfully
         */
        return true;
    }
    else
    {
        /*
         * MLME-ASSOCIATE.request is not appended into NHLE MAC
         * queue, hence free the buffer allocated
         */
        bmm_buffer_free(buffer_header);
        return false;
    }

}
#endif /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */



#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)
bool wpan_mlme_associate_resp(uint64_t DeviceAddress,
                              uint16_t AssocShortAddress,
                              uint8_t status)
{
    buffer_t *buffer_header;
    mlme_associate_resp_t *mlme_associate_resp;

    /* Allocate a small buffer for association response */
    buffer_header = bmm_buffer_alloc(LARGE_BUFFER_SIZE);

    if (NULL == buffer_header)
    {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_associate_resp = (mlme_associate_resp_t *)BMM_BUFFER_POINTER(buffer_header);

    /* Construct mlme_associate_resp_t message */
    mlme_associate_resp->cmdcode = MLME_ASSOCIATE_RESPONSE;

    /* Other fields */
    mlme_associate_resp->DeviceAddress = DeviceAddress;
    mlme_associate_resp->AssocShortAddress = AssocShortAddress;
    mlme_associate_resp->status = status;

    /* Insert mlme_associate_resp_t into NHLE MAC queue */
    if (MAC_SUCCESS == qmm_queue_append(&nhle_mac_q, buffer_header))
    {
        /*
         * MLME-ASSOCIATE.response is appended into NHLE MAC
         * queue successfully
         */
        return true;
    }
    else
    {
        /*
         * MLME-ASSOCIATE.response is not appended into NHLE MAC
         * queue, hence free the buffer allocated
         */
        bmm_buffer_free(buffer_header);
        return false;
    }
}
#endif /* (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) */



#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)
bool wpan_mlme_disassociate_req(wpan_addr_spec_t *DeviceAddrSpec,
                                uint8_t DisassociateReason,
                                bool TxIndirect)
{
    buffer_t *buffer_header;
    mlme_disassociate_req_t *mlme_disassociate_req;

    /* Allocate a small buffer for disassociation request */
    buffer_header = bmm_buffer_alloc(LARGE_BUFFER_SIZE);

    if (NULL == buffer_header)
    {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_disassociate_req = (mlme_disassociate_req_t *)BMM_BUFFER_POINTER(buffer_header);

    /* Update the disassociate request structure */
    mlme_disassociate_req->cmdcode = MLME_DISASSOCIATE_REQUEST;
    mlme_disassociate_req->DisassociateReason = DisassociateReason;
    mlme_disassociate_req->DeviceAddrMode = DeviceAddrSpec->AddrMode;
    mlme_disassociate_req->DevicePANId = DeviceAddrSpec->PANId;
    ADDR_COPY_DST_SRC_64(mlme_disassociate_req->DeviceAddress, DeviceAddrSpec->Addr.long_address);
    mlme_disassociate_req->TxIndirect = TxIndirect;

    if (MAC_SUCCESS == qmm_queue_append(&nhle_mac_q, buffer_header))
    {
        /*
         * MLME-DISASSOCIATE.request is appended into NHLE MAC
         * queue successfully
         */
        return true;
    }
    else
    {
        /*
         * MLME-DISASSOCIATE.request is not appended into NHLE MAC
         * queue, hence free the buffer allocated and return false
         */
        bmm_buffer_free(buffer_header);
        return false;
    }
}
#endif /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) */



#if (MAC_ORPHAN_INDICATION_RESPONSE == 1)
bool wpan_mlme_orphan_resp(uint64_t OrphanAddress,
                           uint16_t ShortAddress,
                           bool AssociatedMember)
{
    buffer_t *buffer_header;
    mlme_orphan_resp_t *mlme_orphan_resp;

    /* Allocate a small buffer for orphan response */
    buffer_header = bmm_buffer_alloc(LARGE_BUFFER_SIZE);

    if (NULL == buffer_header)
    {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_orphan_resp = (mlme_orphan_resp_t *)BMM_BUFFER_POINTER(buffer_header);

    /* Update the orphan response structure */
    mlme_orphan_resp->cmdcode = MLME_ORPHAN_RESPONSE;
    mlme_orphan_resp->OrphanAddress = OrphanAddress;
    mlme_orphan_resp->ShortAddress = ShortAddress;
    mlme_orphan_resp->AssociatedMember = AssociatedMember;

    if (MAC_SUCCESS == qmm_queue_append(&nhle_mac_q, buffer_header))
    {
        /*
         * MLME-ORPHAN.response is appended into NHLE MAC
         * queue successfully
         */
        return true;
    }
    else
    {
        /*
         * MLME-ORPHAN.response is not appended into NHLE MAC
         * queue, hence free the buffer allocated and return false
         */
        bmm_buffer_free(buffer_header);
        return false;
    }
}
#endif /* (MAC_ORPHAN_INDICATION_RESPONSE == 1) */



bool wpan_mlme_reset_req(bool SetDefaultPib)
{
    buffer_t *buffer_header;
    mlme_reset_req_t *mlme_reset_req;

    /* Allocate a small buffer for reset request */
    buffer_header = bmm_buffer_alloc(LARGE_BUFFER_SIZE);

    if (NULL == buffer_header)
    {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_reset_req = (mlme_reset_req_t *)BMM_BUFFER_POINTER(buffer_header);

    /* Update the reset request structure */
    mlme_reset_req->cmdcode = MLME_RESET_REQUEST;
    mlme_reset_req->SetDefaultPIB = SetDefaultPib;

    if (MAC_SUCCESS == qmm_queue_append(&nhle_mac_q, buffer_header))
    {
        /*
         * MLME-RESET.request is appended into NHLE MAC
         * queue
         */
        return true;
    }
    else
    {
        /*
         * MLME-RESET.request is not appended into NHLE MAC
         * queue, hence free the buffer allocated and return false
         */
        bmm_buffer_free(buffer_header);
        return false;
    }
}



#if (MAC_GET_SUPPORT == 1)
bool wpan_mlme_get_req(uint8_t PIBAttribute)
{
    buffer_t *buffer_header;
    mlme_get_req_t *mlme_get_req;

    /* Allocate a large buffer for get request as maximum beacon payload
       should be accommodated */
    buffer_header = bmm_buffer_alloc(LARGE_BUFFER_SIZE);

    /* Check for buffer availability */
    if (NULL == buffer_header)
    {
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_get_req = (mlme_get_req_t*)BMM_BUFFER_POINTER(buffer_header);

    /* Update the get request structure */
    mlme_get_req->cmdcode = MLME_GET_REQUEST;
    mlme_get_req->PIBAttribute = PIBAttribute;

    if (MAC_SUCCESS == qmm_queue_append(&nhle_mac_q, buffer_header))
    {
        /*
         * MLME-GET.request is appended into NHLE MAC
         * queue successfully
         */
        return true;
    }
    else
    {
        /*
         * MLME-GET.request is not appended into NHLE MAC
         * queue, hence free the buffer allocated and return false
         */
        bmm_buffer_free(buffer_header);
        return false;
    }
}
#endif  /* (MAC_GET_SUPPORT == 1) */



bool wpan_mlme_set_req(uint8_t PIBAttribute,
                       void *PIBAttributeValue)
{
    buffer_t *buffer_header;
    mlme_set_req_t *mlme_set_req;
    uint8_t pib_attribute_octet_no;

    /*
     * Allocate a large buffer for set request as maximum beacon payload
     * should be accommodated
     */
    buffer_header = bmm_buffer_alloc(LARGE_BUFFER_SIZE);

    /* Check for buffer availability */
    if (NULL == buffer_header)
    {
        return false;
    }

    /* Get size of PIB attribute to be set */
    pib_attribute_octet_no = mac_get_pib_attribute_size(PIBAttribute);

    /* Get the buffer body from buffer header */
    mlme_set_req = (mlme_set_req_t *)BMM_BUFFER_POINTER(buffer_header);

    /* Construct mlme_set_req_t message */
    mlme_set_req->cmdcode = MLME_SET_REQUEST;

    /* Attribute and attribute value length */
    mlme_set_req->PIBAttribute = PIBAttribute;

    /* Attribute value */
    memcpy((void *)&(mlme_set_req->PIBAttributeValue),
                    (void *)PIBAttributeValue,
                    (size_t)pib_attribute_octet_no);

    /* Insert message into NHLE MAC queue */
    if (MAC_SUCCESS == qmm_queue_append(&nhle_mac_q, buffer_header))
    {
        /*
         * MLME-SET.request is appended into NHLE MAC
         * queue
         */
        return true;
    }
    else
    {
       /*
        * MLME-SET.request is not appended into NHLE MAC
        * queue, hence free the buffer allocated and return false
        */
        bmm_buffer_free(buffer_header);
        return false;
    }
}



#if (MAC_RX_ENABLE_SUPPORT == 1)
bool wpan_mlme_rx_enable_req(bool DeferPermit,
                             uint32_t RxOnTime,
                             uint32_t RxOnDuration)
{
    buffer_t *buffer_header;
    mlme_rx_enable_req_t *mlme_rx_enable_req;

    /* Allocate a small buffer for rx enable request */
    buffer_header = bmm_buffer_alloc(LARGE_BUFFER_SIZE);

    if (NULL == buffer_header)
    {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_rx_enable_req = (mlme_rx_enable_req_t *)BMM_BUFFER_POINTER(buffer_header);

    /* Update the rx enable request structure */
    mlme_rx_enable_req->cmdcode = MLME_RX_ENABLE_REQUEST;
    mlme_rx_enable_req->DeferPermit = DeferPermit;
    mlme_rx_enable_req->RxOnTime = RxOnTime;
    mlme_rx_enable_req->RxOnDuration = RxOnDuration;

    if (MAC_SUCCESS == qmm_queue_append(&nhle_mac_q, buffer_header))
    {
        /*
         * MLME-RX-ENABLE.request is appended into NHLE MAC
         * queue successfully
         */
        return true;
    }
    else
    {
        /*
         * MLME-RX-ENABLE.request is not appended into NHLE MAC
         * queue, hence free the buffer allocated and return false
         */
        bmm_buffer_free(buffer_header);
        return false;
    }
}
#endif /* (MAC_RX_ENABLE_SUPPORT == 1) */



#if (MAC_SCAN_SUPPORT == 1)
bool wpan_mlme_scan_req(uint8_t ScanType,
                        uint32_t ScanChannels,
                        uint8_t ScanDuration,
                        uint8_t ChannelPage)
{
    buffer_t *buffer_header;
    mlme_scan_req_t* mlme_scan_req;

    /* Allocate a small buffer for scan request */
    buffer_header = bmm_buffer_alloc(LARGE_BUFFER_SIZE);

    if (NULL == buffer_header)
    {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_scan_req = (mlme_scan_req_t*)BMM_BUFFER_POINTER(buffer_header);

    /* Update the scan request structure */
    mlme_scan_req->cmdcode = MLME_SCAN_REQUEST;
    mlme_scan_req->ScanType = ScanType;
    mlme_scan_req->ScanChannels = ScanChannels;
    mlme_scan_req->ScanDuration = ScanDuration;
    mlme_scan_req->ChannelPage = ChannelPage;

    if (MAC_SUCCESS == qmm_queue_append(&nhle_mac_q, buffer_header))
    {
        /*
         * MLME-SCAN.request is appended into NHLE MAC
         * queue successfully
         */
        return true;
    }
    else
    {
        /*
         * MLME-SCAN.request is not appended into NHLE MAC
         * queue, hence free the buffer allocated and return false
         */
        bmm_buffer_free(buffer_header);
        return false;
    }
}
#endif /* (MAC_SCAN_SUPPORT == 1) */



#if (MAC_START_REQUEST_CONFIRM == 1)
bool wpan_mlme_start_req(uint16_t PANId,
                         uint8_t LogicalChannel,
                         uint8_t ChannelPage,
                         uint8_t BeaconOrder,
                         uint8_t SuperframeOrder,
                         bool PANCoordinator,
                         bool BatteryLifeExtension,
                         bool CoordRealignment)
{
    buffer_t *buffer_header;
    mlme_start_req_t *mlme_start_req;

    /* Allocate a small buffer for start request */
    buffer_header = bmm_buffer_alloc(LARGE_BUFFER_SIZE);

    if (NULL == buffer_header)
    {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_start_req = (mlme_start_req_t *)BMM_BUFFER_POINTER(buffer_header);

    /* Update the start request structure */
    mlme_start_req->cmdcode = MLME_START_REQUEST;
    mlme_start_req->PANId = PANId;
    mlme_start_req->LogicalChannel = LogicalChannel;
    mlme_start_req->BeaconOrder = BeaconOrder;
    mlme_start_req->SuperframeOrder = SuperframeOrder;
    mlme_start_req->PANCoordinator = PANCoordinator;
    mlme_start_req->BatteryLifeExtension = BatteryLifeExtension;
    mlme_start_req->CoordRealignment = CoordRealignment;
    mlme_start_req->ChannelPage = ChannelPage;

    if (MAC_SUCCESS == qmm_queue_append(&nhle_mac_q, buffer_header))
    {
        /*
         * MLME-START.request is appended into NHLE MAC
         * queue successfully
         */
        return true;
    }
    else
    {
        /*
         * MLME-START.request is not appended into NHLE MAC
         * queue, hence free the buffer allocated ad return false
         */
        bmm_buffer_free(buffer_header);
        return false;
    }
}
#endif /* (MAC_START_REQUEST_CONFIRM == 1) */



#if (MAC_SYNC_REQUEST == 1)
bool wpan_mlme_sync_req(uint8_t LogicalChannel,
                        uint8_t ChannelPage,
                        bool TrackBeacon)
{
    buffer_t *buffer_header;
    mlme_sync_req_t *mlme_sync_req;

    /* Allocate a small buffer for sync request */
    buffer_header = bmm_buffer_alloc(LARGE_BUFFER_SIZE);

    if (NULL == buffer_header)
    {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_sync_req = (mlme_sync_req_t *)BMM_BUFFER_POINTER(buffer_header);

    /* Update the sync request structure */
    mlme_sync_req->cmdcode = MLME_SYNC_REQUEST;
    mlme_sync_req->LogicalChannel = LogicalChannel;
    mlme_sync_req->ChannelPage = ChannelPage;
    mlme_sync_req->TrackBeacon = TrackBeacon;

    if (MAC_SUCCESS == qmm_queue_append(&nhle_mac_q, buffer_header))
    {
        /*
         * MLME-SYNC.request is appended into NHLE MAC
         * queue successfully
         */
        return true;
    }
    else
    {
        /*
         * MLME-SYNC.request is not appended into NHLE MAC
         * queue, hence free the buffer allocated and return false
         */
        bmm_buffer_free(buffer_header);
        return false;
    }
}
#endif /* (MAC_SYNC_REQUEST == 1) */


#if (MAC_INDIRECT_DATA_BASIC == 1)
bool wpan_mlme_poll_req(wpan_addr_spec_t *CoordAddrSpec)
{
    buffer_t *buffer_header;
    mlme_poll_req_t *mlme_poll_req;

    /* Allocate a small buffer for poll request */
    buffer_header = bmm_buffer_alloc(LARGE_BUFFER_SIZE);

    if (NULL == buffer_header)
    {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_poll_req = (mlme_poll_req_t *)BMM_BUFFER_POINTER(buffer_header);

    /* construct mlme_poll_req_t message */
    mlme_poll_req->cmdcode = MLME_POLL_REQUEST;

    /* Other fileds. */
    mlme_poll_req->CoordAddrMode = CoordAddrSpec->AddrMode;
    mlme_poll_req->CoordPANId = CoordAddrSpec->PANId;
    ADDR_COPY_DST_SRC_64(mlme_poll_req->CoordAddress, CoordAddrSpec->Addr.long_address);

    if (MAC_SUCCESS == qmm_queue_append(&nhle_mac_q, buffer_header))
    {
        /*
         * MLME-POLL.request is appended into NHLE MAC
         * queue successfully
         */
        return true;
    }
    else
    {
        /*
         * MLME-POLL.request is not appended into NHLE MAC
         * queue, hence free the buffer allocated and return false
         */
        bmm_buffer_free(buffer_header);
        return false;
    }
}
#endif /* (MAC_INDIRECT_DATA_BASIC == 1) */

/* EOF */

