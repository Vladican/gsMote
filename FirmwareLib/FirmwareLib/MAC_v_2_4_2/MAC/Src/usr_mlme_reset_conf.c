/**
 * @file usr_mlme_reset_conf.c
 *
 * @brief This file contains user call back function for MLME-RESET.confirm.
 *
 * $Id: usr_mlme_reset_conf.c 16325 2009-06-23 16:40:23Z sschneid $
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
#include "ieee_const.h"
#include "SiFLEX02_Ping_Pong.h"
#include "mac_api.h"

// Variable declarations


// Function prototypes
// LSR
void usr_mlme_reset_conf(uint8_t status);
// ^

void usr_mlme_reset_conf(uint8_t status)
{
// LSR
	uint64_t u64TempLongTransceiverId;
	uint16_t u16TempShortTransceiverId;
	uint8_t u8Temp;

	if (status == MAC_SUCCESS)
    {
		wpan_mlme_set_req(phyTransmitPower, &TransceiverParams.u8RfTxPowerLevel);

		u8Temp = TRX_CCA_MODE3;		// Set CCA mode to carrier sense AND energy above threshold.
		wpan_mlme_set_req(phyCCAMode, &u8Temp);

		// Long address of 0 is reserved and should not be used for RF messaging.  This prevents an ack being received by a device
		// sending to a long address, when the receive device is configured with a short address.
		u64TempLongTransceiverId = 0;
		wpan_mlme_set_req(macIeeeAddress, &u64TempLongTransceiverId);

		u16TempShortTransceiverId = 1;		// Always startup with the short address at 1.  A value of 65,535 prevents the MAC from starting.  Once the
											// network is started we will change the MAC address to whatever it is configured as.
		wpan_mlme_set_req(macShortAddress, &u16TempShortTransceiverId);
	}
    else
    {
        wpan_mlme_reset_req(true);			// Something went wrong, so restart.
    }
// ^
}  //end usr_mlme_reset_conf
