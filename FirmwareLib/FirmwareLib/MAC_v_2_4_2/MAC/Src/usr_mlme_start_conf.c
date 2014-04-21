/**
 * @file usr_mlme_start_conf.c
 *
 * @brief This file contains user call back function for MLME-START.confirm.
 *
 * $Id: usr_mlme_start_conf.c 16325 2009-06-23 16:40:23Z sschneid $
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
void usr_mlme_start_conf(uint8_t status);
// ^

void usr_mlme_start_conf(uint8_t status)
{
// LSR
//     if (status == MAC_SUCCESS)
//     {
//     	u8DeviceStarted = true;
//     }
// ^
}  //end usr_mlme_start_conf
