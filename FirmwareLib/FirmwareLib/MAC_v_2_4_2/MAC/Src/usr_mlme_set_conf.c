/**
 * @file usr_mlme_set_conf.c
 *
 * @brief This file contains user call back function for MLME-SET.confirm.
 *
 * $Id: usr_mlme_set_conf.c 16325 2009-06-23 16:40:23Z sschneid $
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

// LSR
// Variable declarations
//extern uint8_t u8DeviceStarted;

// Function prototypes
void usr_mlme_set_conf(uint8_t status, uint8_t PIBAttribute);
// ^

void usr_mlme_set_conf(uint8_t status, uint8_t PIBAttribute)
{
// LSR
// 	if (status == MAC_SUCCESS)	// The MAC layer was properly started.
// 	{
// 		if (u8DeviceStarted == false)	// The device did not start properly so perform a scan.
// 		{
// 			if (PIBAttribute == macShortAddress)
// 			{
// 				wpan_mlme_scan_req(MLME_SCAN_TYPE_ACTIVE, 0x00000020, 1, 0);
// 			}
// 		}
// 	}
// ^
PIBAttribute = PIBAttribute;	//keep compiler happy...
}  //end usr_mlme_set_conf
