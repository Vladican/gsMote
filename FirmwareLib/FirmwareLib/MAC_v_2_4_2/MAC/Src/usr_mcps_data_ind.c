/**
 * @file usr_mcps_data_ind.c
 *
 * @brief This file contains user call back function for MCPS-DATA.indication.
 *
 * $Id: usr_mcps_data_ind.c 16325 2009-06-23 16:40:23Z sschneid $
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
#include "SiFlex_IO.h"
#include "mac_api.h"
#include "SiFLEX02_Ping_Pong.h"

// Variable declarations

// Function prototypes
// LSR
void usr_mcps_data_ind(wpan_addr_spec_t *SrcAddrSpec,
                       wpan_addr_spec_t *DstAddrSpec,
                       uint8_t msduLength,
                       uint8_t *msdu,
                       uint8_t mpduLinkQuality,
                       uint8_t DSN,
                       uint32_t Timestamp);
// ^

void usr_mcps_data_ind(wpan_addr_spec_t *SrcAddrSpec,
                       wpan_addr_spec_t *DstAddrSpec,
                       uint8_t msduLength,
                       uint8_t *msdu,
                       uint8_t mpduLinkQuality,
                       uint8_t DSN,
                       uint32_t Timestamp)
{
// LSR
	// Check the last byte in the message (which indicates the channel the radio should be on), and then make sure it matches
	// the channel we are set to prior to processing it.  This is needed as there is a bug in the RF212 where images cause packets
	// to be received on other channels.
	// An additional RF channel byte is appended to the end of the message.
	if (*(msdu + msduLength - 1) == TransceiverParams.u8RfChannelNumber)
	{
		if ((DstAddrSpec->AddrMode == WPAN_ADDRMODE_SHORT) && (DstAddrSpec->Addr.short_address == TransceiverParams.wuShortTransceiverId.u16))
		{
			if (*msdu == PING_PONG_RF_MSG_TYPE)
			{

				if ((ptrPingPongState == PingPongNormalState_1) ||
				(ptrPingPongState == PingPongNormalState_2))
				{
					if (mpduLinkQuality >= PING_PONG_LQI_EXCELLENT)
					{
						LedBlink(LED_GREEN, LED_BLINK_TIME);
					}
					else if (mpduLinkQuality >= PING_PONG_LQI_GOOD)
					{
						LedBlink(LED_YELLOW, LED_BLINK_TIME);
					}
					else		// Else the LQI is marginal.
					{
						LedBlink(LED_RED, LED_BLINK_TIME);
					}
				}

				if (PingPongValues.u8Master == false)
				{
					SendPingPongRfMessage();
				}
			}
		}
	}
// ^
	// Keep compiler happy.
    DSN = DSN;
    Timestamp = Timestamp;
}  //end usr_mcps_data_ind
