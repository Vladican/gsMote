//////////////////////////////////////////////////////////////////////////////////
//
//  Filename:	    main.c
//
//  Description:
//
//  Micro:
//  Compiler:
//
//  Written by:
//
//  Copyright (c)   2010 LS Research, LLC
//                  www.lsr.com
//
//  Version         rev 0.1 - prerelease
//
//////////////////////////////////////////////////////////////////////////////////

/* === INCLUDES ============================================================ */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "pal.h"
#include "tal.h"
#include "mac_api.h"
#include "app_config.h"
#include "SiFlex_IO.h"
#include "SiFLEX02_Ping_Pong.h"


// Variable Declarations
uint8_t u8DeviceStarted = false;

// Function Prototypes
int main(void);


int main(void)
{
	// Initialize the MAC layer and its underlying layers, like PAL, TAL, BMM.
    if (wpan_init() != MAC_SUCCESS)
    {
    	// Stay here; we need a valid IEEE address.
        // Check kit documentation how to create an IEEE address
        // and to store it into the EEPROM.
        pal_alert();
    }

    LedInit();
	ButtonInit();
    InitPingPongStateMachine();
	
    // The stack is initialized above, hence the global interrupts are enabled here.
    pal_global_irq_enable();

    // Reset the MAC layer to the default values
    // This request will cause a mlme reset confirm ->
    // usr_mlme_reset_conf
    wpan_mlme_reset_req(true);

    while (1)
    {
        wpan_task();
        ptrPingPongState();
    }
}  //end main
