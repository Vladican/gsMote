/**
 * @file usr_mlme_scan_conf.c
 *
 * @brief This file contains user call back function for MLME-SCAN.confirm.
 *
 * $Id: usr_mlme_scan_conf.c 16325 2009-06-23 16:40:23Z sschneid $
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

#include <stdint.h>
#include <stdbool.h>
#include "SiFLEX02_Ping_Pong.h"
#include "mac_api.h"

#if (MAC_SCAN_SUPPORT == 1)

// Variable declarations


// Function prototypes
// LSR
void usr_mlme_scan_conf(uint8_t status,
                        uint8_t ScanType,
                        uint8_t ChannelPage,
                        uint32_t UnscannedChannels,
                        uint8_t ResultListSize,
                        void *ResultList);
// ^

void usr_mlme_scan_conf(uint8_t status,
                        uint8_t ScanType,
                        uint8_t ChannelPage,
                        uint32_t UnscannedChannels,
                        uint8_t ResultListSize,
                        void *ResultList)
{
// LSR
	wpan_mlme_start_req(TransceiverParams.wuPanId.u16, TransceiverParams.u8RfChannelNumber, 0, 15, 15, true, false, false);	//  start the MAC
// ^
	// Keep compiler happy.
    status = status;
    ScanType = ScanType;
    ChannelPage = ChannelPage;
    UnscannedChannels = UnscannedChannels;
    ResultListSize = ResultListSize;
    ResultList = ResultList;
}  //end usr_mlme_scan_conf

#endif // (MAC_SCAN_SUPPORT == 1)

/* EOF */
