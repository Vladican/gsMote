/**
 * @file mac_api.h
 *
 * @brief MAC API for IEEE 802.15.4-2006
 *
 * $Id: mac_api.h 19560 2009-12-15 16:40:05Z sschneid $
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
#ifndef MAC_API_H
#define MAC_API_H

/* === Includes ============================================================= */

#include "pal.h"
#include "mac.h"
#include "return_val.h"
#include "qmm.h"
#include "mac_build_config.h"

/* === Externals ============================================================ */

/**
 * Queue used by MAC for communication to next higher layer.
 */
extern queue_t mac_nhle_q;

/* === Types ================================================================ */

#if !defined(DOXYGEN_NO_MAC)

/**
 * @brief Device address specification structure
 *
 * @ingroup apiMacTypes
 */
typedef struct wpan_addr_spec_tag
{
   /**
    * Address mode (@ref WPAN_ADDRMODE_NONE, @ref WPAN_ADDRMODE_SHORT, or @ref WPAN_ADDRMODE_LONG)
    */
    uint8_t AddrMode;

   /**
    * The 16 bit PAN identifier.
    */
    uint16_t PANId;

   /**
    * Device address. If AddrMode is @ref WPAN_ADDRMODE_SHORT, it is interpreted as a
    * 16 bit address.
    */
    address_field_t Addr;
} wpan_addr_spec_t;


/**
 * @brief PAN descriptor information structure
 *
 * @ingroup apiMacTypes
 */
typedef struct wpan_pandescriptor_tag
{
    /**
     * Coordinator address specification in received beacon frame.
     */
    wpan_addr_spec_t CoordAddrSpec;

    /**
     * The current logical channel used by the network.
     */
    uint8_t     LogicalChannel;

    /**
     * The current channel page occupied by the network.
     */
    uint8_t     ChannelPage;

    /**
     * Superframe specification in received beacon frame.
     */
    uint16_t    SuperframeSpec;

    /**
     * Set to true if the beacon is from a PAN coordinator accepting GTS requests.
     */
    bool        GTSPermit;

    /**
     * LQI at which the beacon was received. Lower values represent poorer link
     * quality.
     */
    uint8_t     LinkQuality;

    /**
     * Time at which the beacon frame was received, in symbol counts.  This quantity
     * shall be interpreted as only 24-bits, with the most significant 8-bits entirely
     * ignored.
     */
    uint32_t    TimeStamp;
} wpan_pandescriptor_t;

#endif /* if !defined(DOXYGEN_NO_MAC) */

/* === Macros =============================================================== */

/**
 * Capacity of queue between MAC and Next Higher Layer
 */
#define MAC_NHLE_QUEUE_CAPACITY         255

/* The following symbolic constants are just for MAC API */

/**
 * Value for the address mode, where no address is given.
 * (see @ref wpan_addr_spec_t::AddrMode)
 * @ingroup apiConst
 */
#define WPAN_ADDRMODE_NONE              (0x00)

/**
 * Value for the address mode, where a 16 bit short address is given.
 * (see @ref wpan_addr_spec_t::AddrMode)
 * @ingroup apiConst
 */
#define WPAN_ADDRMODE_SHORT             (0x02)

/**
 * Value for the address mode, where a 64 bit long address is given.
 * (see @ref wpan_addr_spec_t::AddrMode)
 * @ingroup apiConst
 */
#define WPAN_ADDRMODE_LONG              (0x03)

/**
 * Flag value for capability information field
 * (see @ref wpan_mlme_associate_req()).
 * The alternate PAN coordinator subfield shall be set if the device is
 * capable of becoming a PAN coordinator. Otherwise,
 * the alternate PAN coordinator subfield shall be set to 0.
 * @ingroup apiConst
 */
#define WPAN_CAP_ALTPANCOORD            (0x01)

/**
 * Flag value for capability information field
 * (see @ref wpan_mlme_associate_req()).
 * The device type subfield shall be set if the device is an FFD. Otherwise,
 * the device type subfield shall be set to 0 to indicate an RFD.
 * @ingroup apiConst
 */
#define WPAN_CAP_FFD                    (0x02)

/**
 * Flag value for capability information field
 * (see @ref wpan_mlme_associate_req()).
 * The power source subfield shall be set if the device is receiving power
 * from the alternating current mains. Otherwise, the power source subfield
 * shall be set to 0.
 * @ingroup apiConst
 */
#define WPAN_CAP_PWRSOURCE              (0x04)

/**
 * Flag value for capability information field
 * (see @ref wpan_mlme_associate_req()).
 * The receiver on when idle subfield shall be set if the device does not
 * disable its receiver to conserve power during idle periods. Otherwise, the
 * receiver on when idle subfield shall be set to 0.
 * @ingroup apiConst
 */
#define WPAN_CAP_RXONWHENIDLE           (0x08)

/**
 * Flag value for capability information field
 * (see @ref wpan_mlme_associate_req()).
 * The allocate address subfield shall be set if the device wishes the
 * coordinator to allocate a short address as a result of the association
 * procedure. If this subfield is set to 0, the special short address of
 * 0xfffe shall be allocated to the device and returned through the
 * association response command. In this case, the device shall communicate
 * on the PAN using only its 64 bit extended address.
 * @ingroup apiConst
 */
#define WPAN_CAP_ALLOCADDRESS           (0x80)

/**
 * Symbolic constant for disassociate reason - initiated by parent
 * (see @ref wpan_mlme_disassociate_req())
 * @ingroup apiConst
 */
#define WPAN_DISASSOC_BYPARENT          (0x01)

/**
 * Symbolic constant for disassociate reason - initiated by child
 * (see @ref wpan_mlme_disassociate_req())
 * @ingroup apiConst
 */
#define WPAN_DISASSOC_BYCHILD           (0x02)

/**
 * Marco to extract size of short address list in PAN descriptor
 */
#define WPAN_NUM_SHORT_ADDR_PENDING(x)      ((x) & 0x7)

/**
 * Macro to extract size of extended address list in PAN descriptor
 */
#define WPAN_NUM_EXTENDED_ADDR_PENDING(x)   (((x) >> 4) & 0x7)


/* === Prototypes =========================================================== */
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief The stack initialization function.
 *
 * This function initializes all resources, which are used from the stack.
 * It has to be called before any other function of the stack is called.
 *
 * @ingroup apiMacGeneral
 */
retval_t wpan_init(void);


/**
 * @brief The stack task function called by the application.
 *
 * This function should be called as frequently as possible by the application
 * in order to provide a permanent execution of the protocol stack.
 *
 * @return Boolean true if an event was processed otherwise false.
 * @ingroup apiMacGeneral
 */
bool wpan_task(void);

/*--------------------------------------------------------------------*/

/*
 * These functions have to be called from the application
 * in order to initiate an action in the communication
 * stack at the MAC level
 */
/**
  * @ingroup apiMacReq
  * @{
  */


/**
 * Initiate MCPS-DATA.request service and have it placed in the MCPS-SAP queue.
 *
 * @param SrcAddrMode   Address Mode of the source address.
 * @param DstAddrSpec   Pointer to wpan_addr_spec_t structure for destination.
 * @param msduHandle    Handle (identification) of the MSDU.
 * @param TxOptions     Bitmap for transmission options. Valid values:
 *                      - @ref WPAN_TXOPT_OFF,
 *                      - @ref WPAN_TXOPT_ACK,
 *                      - @ref WPAN_TXOPT_INDIRECT,
 *                      - @ref WPAN_TXOPT_INDIRECT_ACK.
 * @param msdu          Pointer to the data to be transmitted.
 * @param msduLength    Length of the data to be transmitted.
 *
 * @return true - success; false - buffer not available or queue full.
 */
bool wpan_mcps_data_req(uint8_t SrcAddrMode,
                        wpan_addr_spec_t *DstAddrSpec,
                        uint8_t msduLength,
                        uint8_t *msdu,
                        uint8_t msduHandle,
                        uint8_t TxOptions);



#if ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_BASIC == 1)) || defined(DOXYGEN)
/**
 * Initiate MCPS-PURGE.request service and have it placed in the MCPS-SAP queue.
 *
 * @param msduHandle    Handle of MSDU to be purged.
 *
 * @return true - success; false - buffer not available or queue full.
 */
bool wpan_mcps_purge_req(const uint8_t msduHandle);
#endif  /* ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_BASIC == 1)) */



#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) || defined(DOXYGEN)
/**
 * Initiate MLME-ASSOCIATE.request service and have it placed in the MLME-SAP queue.
 *
 * @param LogicalChannel        The logical channel on which to attempt association.
 * @param ChannelPage           The channel page on which to attempt association.
 * @param CoordAddrSpec         Pointer to wpan_addr_spec_t structure for coordinator.
 * @param CapabilityInformation Bitmap that describes the nodes capabilities.
 *                              (@ref WPAN_CAP_ALTPANCOORD |
 *                               @ref WPAN_CAP_FFD |
 *                               @ref WPAN_CAP_PWRSOURCE |
 *                               @ref WPAN_CAP_RXONWHENIDLE |
 *                               @ref WPAN_CAP_ALLOCADDRESS)
 *
 * @return true - success; false - buffer not available or queue full.
 */
bool wpan_mlme_associate_req(uint8_t LogicalChannel,
                             uint8_t ChannelPage,
                             wpan_addr_spec_t *CoordAddrSpec,
                             uint8_t CapabilityInformation);
#endif  /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */



#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) || defined(DOXYGEN)
/**
 * Initiate MLME-ASSOCIATE.response service and place it in the MLME-SAP queue.
 *
 * @param DeviceAddress      Extended address for device requesting association.
 * @param AssocShortAddress  Short address allocated on successful association.
 * @param status             Status of the association attempt. Valid values:
 *                           - @ref ASSOCIATION_SUCCESSFUL,
 *                           - @ref PAN_AT_CAPACITY,
 *                           - @ref PAN_ACCESS_DENIED,
 *                           - @ref ASSOCIATION_RESERVED.
 *
 * @return true - success; false - buffer not available or queue full.
 */
bool wpan_mlme_associate_resp(uint64_t DeviceAddress,
                              uint16_t AssocShortAddress,
                              uint8_t status);
#endif  /* (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) */



#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) || defined(DOXYGEN)
/**
 * Inititate MLME-DISASSOCIATE.request service and have it placed in the MLME-SAP queue.
 *
 * @param DeviceAddrSpec     Pointer to wpan_addr_spec_t structure for device
 *                           to which to send the disassociation notification
 *                           command.
 * @param DisassociateReason Reason for disassociation. Valid values:
 *                           - @ref WPAN_DISASSOC_BYPARENT,
 *                           - @ref WPAN_DISASSOC_BYCHILD.
 * @param TxIndirect         TRUE if the disassociation notification command
 *                           is to be sent indirectly
 *
 * @return true - success; false - buffer not available or queue full.
 */
bool wpan_mlme_disassociate_req(wpan_addr_spec_t *DeviceAddrSpec,
                                uint8_t DisassociateReason,
                                bool TxIndirect);
#endif  /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) */



#if (MAC_GET_SUPPORT == 1) || defined(DOXYGEN)
/**
 * Initiate MLME-GET.request service and have it placed in the MLME-SAP queue.
 *
 * @param PIBAttribute     PIB attribute to be retrieved.
 *
 * @return true - success; false - buffer not availability or queue full.
 */
bool wpan_mlme_get_req(uint8_t PIBAttribute);
#endif  /* (MAC_GET_SUPPORT == 1) */



#if (MAC_ORPHAN_INDICATION_RESPONSE == 1) || defined(DOXYGEN)
/**
 * Initiate MLME-ORPHAN.response service and have it placed in MLME_SAP queue.
 *
 * @param OrphanAddress    Address of orphaned device.
 * @param ShortAddress     Short address allocated to orphaned device.
 * @param AssociatedMember Boolean true if the orphaned device is associated.
 *
 * @return true - success; false - buffer not availability or queue full.
 */
bool wpan_mlme_orphan_resp(uint64_t OrphanAddress,
                           uint16_t ShortAddress,
                           bool AssociatedMember);
#endif  /* (MAC_ORPHAN_INDICATION_RESPONSE == 1) */



#if (MAC_INDIRECT_DATA_BASIC == 1) || defined(DOXYGEN)
/**
 * Initiate MLME-POLL.request service and have it placed in the MLME-SAP queue.
 *
 * @param CoordAddrSpec   Pointer to wpan_addr_spec_t structure for the coordinator.
 *
 * @return true - success; false - buffer not availability or queue full.
 */
bool wpan_mlme_poll_req(wpan_addr_spec_t *CoordAddrSpec);
#endif  /* (MAC_INDIRECT_DATA_BASIC == 1) */



/**
 * Initiate MLME-RESET.request service and have it placed in the MLME-SAP queue.
 *
 * @param SetDefaultPib  Boolean to set all PIB values to their respective defaults.
 *
 * @return true - success; false - buffer not available or queue full.
 */
bool wpan_mlme_reset_req(bool SetDefaultPib);



/**
 * Initiate MLME-SET.request service and have it placed in MLME_SAP queue.
 *
 * @param PIBAttribute      PIB attribute to be set.
 * @param PIBAttributeValue Pointer to new PIB attribute value.
 *
 * @return true - success; false - buffer not available or queue full.
 */
bool wpan_mlme_set_req(uint8_t PIBAttribute,
                       void *PIBAttributeValue);



#if (MAC_RX_ENABLE_SUPPORT == 1) || defined(DOXYGEN)
/**
 * Initiate MLME-RX-ENABLE.request service and have it placed in the MLME-SAP queue.
 *
 * @param DeferPermit     Set to true if receiver enable can be deferred until next
 *                        superframe if requested time has already passed.
 * @param RxOnTime        Number of symbols from start of superframe before receiver
 *                        is enabled.
 * @param RxOnDuration    Number of symbols for which the receiver is enabled,
 *
 * @return true - success; false - buffer not available or queue full.
 */
bool wpan_mlme_rx_enable_req(bool DeferPermit,
                             uint32_t RxOnTime,
                             uint32_t RxOnDuration);
#endif  /* (MAC_RX_ENABLE_SUPPORT == 1) */



#if ((MAC_SCAN_ED_REQUEST_CONFIRM == 1)      || \
     (MAC_SCAN_ACTIVE_REQUEST_CONFIRM == 1)  || \
     (MAC_SCAN_PASSIVE_REQUEST_CONFIRM == 1) || \
     (MAC_SCAN_ORPHAN_REQUEST_CONFIRM == 1)) || defined(DOXYGEN)
/**
 * Initiate MLME-SCAN.request service and have it placed in the MLME-SAP queue.
 *
 * @param ScanType      Type of scan to perform. Valid values:
 *                      - @ref MLME_SCAN_TYPE_ED,
 *                      - @ref MLME_SCAN_TYPE_ACTIVE,
 *                      - @ref MLME_SCAN_TYPE_PASSIVE,
 *                      - @ref MLME_SCAN_TYPE_ORPHAN.
 * @param ScanChannels  Channels to be scanned.
 * @param ScanDuration  Duration of each scan.
 * @param ChannelPage   The channel page on which to perform the scan.
 *
 * @return true - success; false - buffer not available or queue full.
 */
bool wpan_mlme_scan_req(uint8_t ScanType,
                        uint32_t ScanChannels,
                        uint8_t ScanDuration,
                        uint8_t ChannelPage);
#endif



#if (MAC_START_REQUEST_CONFIRM == 1) || defined(DOXYGEN)
/**
 * Initiate MLME-START service and have it placed in the MLME-SAP queue.
 *
 * @param PANId                 PAN identifier to be used by device.
 * @param LogicalChannel        The logical channel on which to start
 *                              using the new superframe configuration.
 * @param ChannelPage           The channel page on which to begin
 *                              using the new superframe configuration.
 * @param BeaconOrder           Beacon transmission interval.
 * @param SuperframeOrder       Duration of active portion of superframe.
 * @param PANCoordinator        Indicates whether node is PAN coordinator of PAN.
 * @param BatteryLifeExtension  Boolean true disables receiver of beaconing device
 *                              for a period after interframe spacing of beacon frame.
 * @param CoordRealignment      Boolean to transmit Coordinator Realignment command
 *                              prior to changing to new superframe configuration.
 *
 * @return true - success; false - buffer not available or queue full.
 */
bool wpan_mlme_start_req(uint16_t PANId,
                         uint8_t LogicalChannel,
                         uint8_t ChannelPage,
                         uint8_t BeaconOrder,
                         uint8_t SuperframeOrder,
                         bool PANCoordinator,
                         bool BatteryLifeExtension,
                         bool CoordRealignment);
#endif  /* (MAC_START_REQUEST_CONFIRM == 1) */



#if (MAC_SYNC_REQUEST == 1) || defined(DOXYGEN)
/**
 * Initiate MLME-SYNC.request service and have it placed in the MLME-SAP queue.
 *
 * @param LogicalChannel   The logical channel on which to attempt coordinator
 *                         synchronization.
 * @param ChannelPage      The channel page on which to attempt coordinator
 *                         synchronization.
 * @param TrackBeacon      Boolean to synchronize with next beacon and to track
 *                         all future beacons.
 * @return true - success; false - buffer not available or queue full.
 */
bool wpan_mlme_sync_req(uint8_t LogicalChannel,
                        uint8_t ChannelPage,
                        bool TrackBeacon);
#endif /* (MAC_SYNC_REQUEST == 1) */



/*@}*//* apiMacReq */

/**
  * @addtogroup apiMacCb
  * @{
  */


/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MCPS-DATA.confirm.
 *
 * @param msduHandle  Handle of MSDU handed over to MAC earlier.
 * @param status      Result for requested data transmission request.
 * @param Timestamp   The time, in symbols, at which the data were transmitted.
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mcps_data_conf(uint8_t msduHandle,
                        uint8_t status,
                        uint32_t Timestamp);



/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MCPS-DATA.indication.
 *
 * @param SrcAddrSpec      Pointer to source address specification.
 * @param DstAddrSpec      Pointer to destination address specification.
 * @param msduLength       Number of octets contained in MSDU.
 * @param msdu             Pointer to MSDU.
 * @param mpduLinkQuality  LQI measured during reception of the MPDU.
 * @param DSN              The DSN of the received data frame.
 * @param Timestamp        The time, in symbols, at which the data were received.
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mcps_data_ind(wpan_addr_spec_t *SrcAddrSpec,
                       wpan_addr_spec_t *DstAddrSpec,
                       uint8_t msduLength,
                       uint8_t *msdu,
                       uint8_t mpduLinkQuality,
                       uint8_t DSN,
                       uint32_t Timestamp);


#if ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_BASIC == 1)) || defined(DOXYGEN)
/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MCPS-PURGE.confirm.
 *
 * @param msduHandle           Handle (id) of MSDU to be purged.
 * @param status               Result of requested purge operation.
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mcps_purge_conf(uint8_t msduHandle,
                         uint8_t status);
#endif  /* ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_BASIC == 1)) */



#if (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) || defined(DOXYGEN)
/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MLME-ASSOCIATE.confirm.
 *
 * @param AssocShortAddress    Short address allocated by the coordinator.
 * @param status               Result of requested association operation.
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mlme_associate_conf(uint16_t AssocShortAddress,
                             uint8_t status);
#endif  /* (MAC_ASSOCIATION_REQUEST_CONFIRM == 1) */



#if (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) || defined(DOXYGEN)
/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MLME-ASSOCIATE.indication.
 *
 * @param DeviceAddress         Extended address of device requesting association.
 * @param CapabilityInformation Capabilities of device requesting association.
 *                              (@ref WPAN_CAP_ALTPANCOORD |
 *                               @ref WPAN_CAP_FFD |
 *                               @ref WPAN_CAP_PWRSOURCE |
 *                               @ref WPAN_CAP_RXONWHENIDLE |
 *                               @ref WPAN_CAP_ALLOCADDRESS)
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mlme_associate_ind(uint64_t DeviceAddress,
                            uint8_t CapabilityInformation);
#endif  /* (MAC_ASSOCIATION_INDICATION_RESPONSE == 1) */



#if (MAC_BEACON_NOTIFY_INDICATION == 1) || defined(DOXYGEN)
/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MLME-BEACON-NOTIFY.indication.
 *
 * @param BSN            Beacon sequence number.
 * @param PANDescriptor  Pointer to PAN descriptor for received beacon.
 * @param PendAddrSpec   Pending address specification in received beacon.
 * @param AddrList       List of addresses of devices the coordinator has pending data.
 * @param sduLength      Length of beacon payload.
 * @param sdu            Pointer to beacon payload.
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mlme_beacon_notify_ind(uint8_t BSN,
                                wpan_pandescriptor_t *PANDescriptor,
                                uint8_t PendAddrSpec,
                                void *AddrList,
                                uint8_t sduLength,
                                uint8_t *sdu);
#endif  /* (MAC_BEACON_NOTIFY_INDICATION == 1) */



#if ((MAC_ORPHAN_INDICATION_RESPONSE == 1) || (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)) || defined(DOXYGEN)
/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MLME-COMM-STATUS.indication.
 *
 * @param SrcAddrSpec      Pointer to source address specification.
 * @param DstAddrSpec      Pointer to destination address specification.
 * @param status           Result for related response operation.
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mlme_comm_status_ind(wpan_addr_spec_t *SrcAddrSpec,
                              wpan_addr_spec_t *DstAddrSpec,
                              uint8_t status);
#endif  /* ((MAC_ORPHAN_INDICATION_RESPONSE == 1) || (MAC_ASSOCIATION_INDICATION_RESPONSE == 1)) */



#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) || defined(DOXYGEN)
/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MLME-DISASSOCIATE.confirm.
 *
 * @param status             Result of requested disassociate operation.
 * @param DeviceAddrSpec     Pointer to wpan_addr_spec_t structure for device
 *                           that has either requested disassociation or been
 *                           instructed to disassociate by its coordinator.
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mlme_disassociate_conf(uint8_t status,
                                wpan_addr_spec_t *DeviceAddrSpec);
#endif /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) || defined(DOXYGEN) */



#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) || defined(DOXYGEN)
/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MLME-DISASSOCIATE.indication.
 *
 * @param DeviceAddress        Extended address of device which initiated the
 *                             disassociation request.
 * @param DisassociateReason   Reason for the disassociation. Valid values:
 *                           - @ref WPAN_DISASSOC_BYPARENT,
 *                           - @ref WPAN_DISASSOC_BYCHILD.
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mlme_disassociate_ind(uint64_t DeviceAddress,
                               uint8_t DisassociateReason);
#endif  /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) || defined(DOXYGEN) */



#if (MAC_GET_SUPPORT == 1) || defined(DOXYGEN)
/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MLME-GET.confirm.
 *
 * @param status            Result of requested PIB attribute get operation.
 * @param PIBAttribute      Retrieved PIB attribute.
 * @param PIBAttributeValue Pointer to data containing retrieved PIB attribute,
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mlme_get_conf(uint8_t status,
                       uint8_t PIBAttribute,
                       void *PIBAttributeValue);
#endif  /* (MAC_GET_SUPPORT == 1) || defined(DOXYGEN) */



#if (MAC_ORPHAN_INDICATION_RESPONSE == 1) || defined(DOXYGEN)
/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MLME-ORPHAN.indication.
 *
 * @param OrphanAddress     Address of orphaned device.
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mlme_orphan_ind(uint64_t OrphanAddress);
#endif  /* (MAC_ORPHAN_INDICATION_RESPONSE == 1) || defined(DOXYGEN) */



#if (MAC_INDIRECT_DATA_BASIC == 1) || defined(DOXYGEN)
/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MLME-POLL.confirm.
 *
 * @param status           Result of requested poll operation.
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mlme_poll_conf(uint8_t status);
#endif  /* (MAC_INDIRECT_DATA_BASIC == 1) || defined(DOXYGEN) */



/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MLME-RESET.confirm.
 *
 * @param status           Result of requested reset operation.
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mlme_reset_conf(uint8_t status);



#if (MAC_RX_ENABLE_SUPPORT == 1) || defined(DOXYGEN)
/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MLME-RX-ENABLE.confirm.
 *
 * @param status           Result of requested receiver enable operation.
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mlme_rx_enable_conf(uint8_t status);
#endif  /* (MAC_RX_ENABLE_SUPPORT == 1) || defined(DOXYGEN) */



#if ((MAC_SCAN_ED_REQUEST_CONFIRM == 1)      || \
     (MAC_SCAN_ACTIVE_REQUEST_CONFIRM == 1)  || \
     (MAC_SCAN_PASSIVE_REQUEST_CONFIRM == 1) || \
     (MAC_SCAN_ORPHAN_REQUEST_CONFIRM == 1)) || defined(DOXYGEN)
/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MLME-SCAN.confirm.
 *
 * @param status            Result of requested scan operation.
 * @param ScanType          Type of scan performed.
 * @param ChannelPage       The channel page on which the scan was performed.
 * @param UnscannedChannels Bitmap of unscanned channels
 * @param ResultListSize    Number of elements in ResultList.
 * @param ResultList        Pointer to array of scan results .
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mlme_scan_conf(uint8_t status,
                        uint8_t ScanType,
                        uint8_t ChannelPage,
                        uint32_t UnscannedChannels,
                        uint8_t ResultListSize,
                        void *ResultList);
#endif



/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MLME-SET.confirm.
 *
 * @param status        Result of requested PIB attribute set operation.
 * @param PIBAttribute  Updated PIB attribute.
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mlme_set_conf(uint8_t status,
                       uint8_t PIBAttribute);



#if (MAC_START_REQUEST_CONFIRM == 1) || defined(DOXYGEN)
/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MLME-START.confirm.
 *
 * @param status        Result of requested start operation.
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mlme_start_conf(uint8_t status);
#endif  /* (MAC_START_REQUEST_CONFIRM == 1) || defined(DOXYGEN) */



/**
 * Callback function that must be implemented by application (NHLE) for MAC service
 * MLME-SYNC-LOSS.indication.
 *
 * @param LossReason     Reason for synchronization loss.
 * @param PANId          The PAN identifier with which the device lost
 *                       synchronization or to which it was realigned.
 * @param LogicalChannel The logical channel on which the device lost
 *                       synchronization or to which it was realigned.
 * @param ChannelPage    The channel page on which the device lost
 *                       synchronization or to which it was realigned.
 *
 * @return void
 *
 * @ingroup apiMacCb
 */
void usr_mlme_sync_loss_ind(uint8_t LossReason,
                            uint16_t PANId,
                            uint8_t LogicalChannel,
                            uint8_t ChannelPage);

/*@}*//* apiMacCb */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MAC_API_H */
/* EOF */
