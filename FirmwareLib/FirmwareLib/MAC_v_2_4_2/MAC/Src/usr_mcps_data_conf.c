/**
 * @file usr_mcps_data_conf.c
 *
 * @brief This file contains user call back function for MCPS-DATA.confirm.
 *
 * $Id: usr_mcps_data_conf.c 16325 2009-06-23 16:40:23Z sschneid $
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
#include "mac_api.h"

/* === Macros ============================================================== */


/* === Globals ============================================================= */


/* === Prototypes ========================================================== */


/* === Implementation ====================================================== */

void usr_mcps_data_conf(uint8_t msduHandle, uint8_t status, uint32_t Timestamp)
{
    /* Keep compiler happy. */
    msduHandle = msduHandle;
    status = status;
    Timestamp = Timestamp;
}
/* EOF */
