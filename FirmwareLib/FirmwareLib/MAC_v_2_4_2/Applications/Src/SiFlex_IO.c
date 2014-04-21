//////////////////////////////////////////////////////////////////////////////////
//
//  Filename:	    SiFlex_IO.c
//
//  Description:
//
//  Micro:
//  Compiler:
//
//  Written by:
//
//  Copyright (c)   2009 LS Research, LLC
//                  www.lsr.com
//
//	Version         rev 0.1 - prerelease
//
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
//      Includes                                                                //
//////////////////////////////////////////////////////////////////////////////////

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "avrtypes.h"
#include "app_config.h"
#include "pal.h"
#include "SiFlex_IO.h"


//////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
//////////////////////////////////////////////////////////////////////////////////

void LedInit(void);
void LedSet(LedId_t Led, LedAction_t LedAction);
void LedsAllOff(void);
void LedBlink(LedId_t Led, uint16_t u16LedBlinkTime);
void ButtonInit(void);
ButtonState_t ButtonRead(ButtonId_t ButtonNumber);

#ifdef STATIC_RF_TEST_MODES
	void StaticRfTestModeInit(void);
	StaticRfTestMode_t GetStaticRfTestMode(void);
#endif

extern retval_t pal_timer_start(uint8_t timer_id, uint32_t timer_count, timeout_type_t timeout_type, void *timer_cb, void *param_cb);


//////////////////////////////////////////////////////////////////////////////////
// Variable Declarations
//////////////////////////////////////////////////////////////////////////////////

static uint16_t u16Led1Timer = 0;
static uint16_t u16Led2Timer = 0;
static uint16_t u16Led3Timer = 0;
static uint8_t u8LedTimerInterruptActive = false;


// This timer interrupt should get called every 1msec if there is at least one timer active.
void LedBlinkTimer_cb(void)
{
	if (u16Led1Timer != 0)
	{
		u16Led1Timer--;
		if (u16Led1Timer == 0)
		{
			LedSet(LED1, LED_OFF);
			if ((u16Led2Timer == 0) && (u16Led3Timer == 0))
			{
				u8LedTimerInterruptActive = false;
			}
		}
	}

	if (u16Led2Timer != 0)
	{
		u16Led2Timer--;
		if (u16Led2Timer == 0)
		{
			LedSet(LED2, LED_OFF);
			if ((u16Led1Timer == 0) && (u16Led3Timer == 0))
			{
				u8LedTimerInterruptActive = false;
			}
		}
	}

	if (u16Led3Timer != 0)
	{
		u16Led3Timer--;
		if (u16Led3Timer == 0)
		{
			LedSet(LED3, LED_OFF);
			if ((u16Led1Timer == 0) && (u16Led2Timer == 0))
			{
				u8LedTimerInterruptActive = false;
			}
		}
	}

	if (u8LedTimerInterruptActive == true)
	{
		pal_timer_start(TIMER_LED_OFF, LED_TIMER_PERIOD, TIMEOUT_RELATIVE, (void *)LedBlinkTimer_cb, NULL);
	}
}  //end LedBlinkTimer_cb


void LedInit(void)
{
    LED_PORT |= (1 << LED1_PIN);
    LED_PORT |= (1 << LED2_PIN);
    LED_PORT |= (1 << LED3_PIN);
    LED_PORT_DIR |= (1 << LED1_PIN);
    LED_PORT_DIR |= (1 << LED2_PIN);
    LED_PORT_DIR |= (1 << LED3_PIN);
}  //end LedInit


// Set LED to LED_ON, LED_OFF, or LED_TOGGLE.
void LedSet(LedId_t Led, LedAction_t LedAction)
{
    uint8_t u8Pin;

    switch (Led)
    {
		case LED1:
				u8Pin = LED1_PIN;
				break;

		case LED2:
				u8Pin = LED2_PIN;
				break;

		case LED3:
				u8Pin = LED3_PIN;
				break;

        default:
				u8Pin = LED1_PIN;
				break;
    }

    switch (LedAction)
    {
        case LED_ON:
				LED_PORT &= ~(1 << u8Pin);
				break;

        case LED_OFF:
				LED_PORT |= (1 << u8Pin);
				break;

        case LED_TOGGLE:
				if ((LED_PORT & (1 << u8Pin)) == 0x00)
				{
					LED_PORT |= (1 << u8Pin);
				}
				else
				{
					LED_PORT &= ~(1 << u8Pin);
				}
				break;

        default:
				break;
    }
}  //end LedSet


void LedsAllOff(void)
{
	LedSet(LED1, LED_OFF);
	LedSet(LED2, LED_OFF);
	LedSet(LED3, LED_OFF);
}  //end LedsAllOff


// u16LedBlinkTime is in units of 5msec.
void LedBlink(LedId_t Led, uint16_t u16LedBlinkTime)
{
    switch (Led)
    {
		case LED1:
				LedSet(LED1, LED_ON);
				u16Led1Timer = u16LedBlinkTime;
				break;

		case LED2:
				LedSet(LED2, LED_ON);
				u16Led2Timer = u16LedBlinkTime;
				break;

		case LED3:
				LedSet(LED3, LED_ON);
				u16Led3Timer = u16LedBlinkTime;
				break;

        default:
				break;
    }

    pal_global_irq_disable();

    if (u8LedTimerInterruptActive == false)
	{
		u8LedTimerInterruptActive = true;
		pal_timer_start(TIMER_LED_OFF, LED_TIMER_PERIOD, TIMEOUT_RELATIVE, (void *)LedBlinkTimer_cb, NULL);
	}
    pal_global_irq_enable();
}  //end LedBlink


void ButtonInit(void)
{
	BUTTON_1_PINCTRL = (PORT_OPC_PULLUP_gc + PORT_ISC_BOTHEDGES_gc);
	BUTTON_2_PINCTRL = (PORT_OPC_PULLUP_gc + PORT_ISC_BOTHEDGES_gc);
	BUTTON_1_PORT |= (1 << BUTTON_1_PIN);
	BUTTON_2_PORT |= (1 << BUTTON_2_PIN);
	BUTTON_1_PORT_DIR &= ~(1 << BUTTON_1_PIN);
	BUTTON_2_PORT_DIR &= ~(1 << BUTTON_2_PIN);
}  //end ButtonInit


ButtonState_t ButtonRead(ButtonId_t ButtonNumber)
{
	static uint8_t u8ButtonPin;
	static uint8_t* pu8ButtonPort;

	switch (ButtonNumber)
	{
		case BUTTON_1:
				u8ButtonPin = BUTTON_1_PIN;
				pu8ButtonPort = BUTTON_1_PORT;
				break;

		case BUTTON_2:
		default:
				u8ButtonPin = BUTTON_2_PIN;
				pu8ButtonPort = BUTTON_2_PORT;
				break;
	}

	if (((uint8_t)pu8ButtonPort & (1 << u8ButtonPin)) == 0x00)
	{
		return BUTTON_PRESSED;
	}
	else
	{
		return BUTTON_OFF;
	}
}  //end ButtonRead


#ifdef STATIC_RF_TEST_MODES
void StaticRfTestModeInit(void)
{
	// Configure pin directions as inputs.
	STATIC_TEST_PORT_DIR &= ~(STATIC_TEST_0_PIN_BITMASK | STATIC_TEST_1_PIN_BITMASK | STATIC_TEST_2_PIN_BITMASK);

	// Enable internal pullup resistors.
	STATIC_TEST_0_PINCTRL = PORT_OPC_WIREDANDPULL_gc;
	STATIC_TEST_1_PINCTRL = PORT_OPC_WIREDANDPULL_gc;
	STATIC_TEST_2_PINCTRL = PORT_OPC_WIREDANDPULL_gc;
}  //end StaticRfTestModeInit


StaticRfTestMode_t GetStaticRfTestMode(void)
{
	StaticRfTestMode_t StaticRfTestMode;

	StaticRfTestMode = (STATIC_TEST_PORT >> 5);

#ifdef ENHANCED_FCC_TEST_MODES
	if (ButtonRead(BUTTON_2) != BUTTON_PRESSED)
	StaticRfTestMode |= 0x08;
#endif

	return StaticRfTestMode;
}  //end GetStaticRfTestMode
#endif
