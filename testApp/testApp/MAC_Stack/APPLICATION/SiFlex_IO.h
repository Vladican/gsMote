//////////////////////////////////////////////////////////////////////////////////
//
//  Filename:	    SiFlex_IO.h
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
//  Version         rev 0.1 - prerelease
//
//////////////////////////////////////////////////////////////////////////////////

#ifndef SIFLEX_IO_H
#define SIFLEX_IO_H


//////////////////////////////////////////////////////////////////////////////////
// Type Definitions
//////////////////////////////////////////////////////////////////////////////////

// Enumerations used to identify LEDs.
typedef enum eLedIdTag
{
	LED1 = 1,
	LED2 = 2,
	LED3 = 3,
} LedId_t;


typedef enum eLedActionTag
{
    LED_ON,
    LED_OFF,
    LED_TOGGLE
} LedAction_t;


// Enumerations used to identify buttons.
typedef enum eButtonIdTag
{
    BUTTON_1,
    BUTTON_2
} ButtonId_t;


typedef enum eButtonStateTag
{
    BUTTON_PRESSED,
    BUTTON_OFF
} ButtonState_t;


typedef enum eStaticRfTestMode
{							// Jumpers -	TEST2	TEST1	TEST0
#ifndef ENHANCED_FCC_TEST_MODES
	RECEIVE_MID_CHANNEL = 0,			// 	  X		  X		  X
	TRANSMIT_MODULATED_HIGH_CHANNEL,	// 	  X		  X		  NC
	TRANSMIT_MODULATED_MID_CHANNEL,		// 	  X		  NC	  X
	TRANSMIT_MODULATED_LOW_CHANNEL,		// 	  X		  NC	  NC
	TRANSMIT_UNMODULATED_HIGH_CHANNEL,	// 	  NC	  X		  X
	TRANSMIT_UNMODULATED_MID_CHANNEL,	// 	  NC	  X		  NC
	TRANSMIT_UNMODULATED_LOW_CHANNEL,	// 	  NC	  NC	  X
	NORMAL								// 	  NC	  NC	  NC
#else	// else defined ENHANCED_FCC_TEST_MODES
												// Jumpers -	User2 Button	TEST2	TEST1	TEST0
												// Jumpers -	PF0				PC7		PC6		PC5
	RECEIVE_OQPSK_1MBPS_MID_CHANNEL = 3,		// 	  			Pressed			X		NC	  	NC
	RECEIVE_OQPSK_250KBPS_MID_CHANNEL,			// 	  			Pressed			NC	  	X		X
	RECEIVE_BPSK_MID_CHANNEL,					// 	  			Pressed			NC	  	X		NC
	TRANSMIT_RANDOM_OQPSK_1MBPS_HIGH_CHANNEL,	// 	  			Pressed			NC	  	NC	  	X
	TRANSMIT_RANDOM_OQPSK_1MBPS_MID_CHANNEL,	// 	  			Pressed			NC	  	NC	  	NC
	TRANSMIT_RANDOM_OQPSK_1MBPS_LOW_CHANNEL,	// 	  			Not Pressed		X		X		X
	TRANSMIT_RANDOM_OQPSK_250KBPS_HIGH_CHANNEL,	// 	  			Not Pressed		X		X		NC
	TRANSMIT_RANDOM_OQPSK_250KBPS_MID_CHANNEL,	// 	  			Not Pressed		X		NC	  	X
	TRANSMIT_RANDOM_OQPSK_250KBPS_LOW_CHANNEL,	// 	  			Not Pressed		X		NC	  	NC
	TRANSMIT_BPSK_HIGH_CHANNEL,					// 	  			Not Pressed		NC	  	X		X
	TRANSMIT_BPSK_MID_CHANNEL,					// 	  			Not Pressed		NC	  	X		NC
	TRANSMIT_BPSK_LOW_CHANNEL,					// 	  			Not Pressed		NC	  	NC	  	X
	NORMAL										// 	  			Not Pressed		NC	  	NC	  	NC
#endif
} StaticRfTestMode_t;


//////////////////////////////////////////////////////////////////////////////////
// Definitions
//////////////////////////////////////////////////////////////////////////////////

#define ENABLE_DEBUG_PINS

#if defined(ENABLE_DEBUG_PINS)
	// Debug Pin Definitions
	#define DEBUG1_PORT					PORTF_OUT
	#define DEBUG1_PORT_DIR             PORTF_DIR
	#define DEBUG2_PORT					PORTF_OUT
	#define DEBUG2_PORT_DIR             PORTF_DIR
	#define DEBUG3_PORT					PORTF_OUT
	#define DEBUG3_PORT_DIR             PORTF_DIR
	#define DEBUG4_PORT					PORTA_OUT
	#define DEBUG4_PORT_DIR             PORTA_DIR

	#define DEBUG1_PIN					5		// Module pin 39, uC pin 51 - PF5.
	#define DEBUG2_PIN					6		// Module pin 40, uC pin 54 - PF6.
	#define DEBUG3_PIN					7		// Module pin 41, uC pin 55 - PF7.
	#define DEBUG4_PIN					3		// Module pin 42, uC pin 1 - PA3.

	#define DEBUG1_LOW					DEBUG1_PORT &= ~(1 << DEBUG1_PIN)
	#define DEBUG1_HIGH					DEBUG1_PORT |= (1 << DEBUG1_PIN)
	#define DEBUG1_TOGGLE				DEBUG1_PORT ^= (1 << DEBUG1_PIN)
	#define DEBUG1_PULSE				DEBUG1_HIGH; DEBUG1_LOW
	#define DEBUG2_LOW					DEBUG2_PORT &= ~(1 << DEBUG2_PIN)
	#define DEBUG2_HIGH					DEBUG2_PORT |= (1 << DEBUG2_PIN)
	#define DEBUG2_TOGGLE				DEBUG2_PORT ^= (1 << DEBUG2_PIN)
	#define DEBUG2_PULSE				DEBUG2_HIGH; DEBUG2_LOW
	#define DEBUG3_LOW					DEBUG3_PORT &= ~(1 << DEBUG3_PIN)
	#define DEBUG3_HIGH					DEBUG3_PORT |= (1 << DEBUG3_PIN)
	#define DEBUG3_TOGGLE				DEBUG3_PORT ^= (1 << DEBUG3_PIN)
	#define DEBUG3_PULSE				DEBUG3_HIGH; DEBUG3_LOW
	#define DEBUG4_LOW					DEBUG4_PORT &= ~(1 << DEBUG4_PIN)
	#define DEBUG4_HIGH					DEBUG4_PORT |= (1 << DEBUG4_PIN)
	#define DEBUG4_TOGGLE				DEBUG4_PORT ^= (1 << DEBUG4_PIN)
	#define DEBUG4_PULSE				DEBUG4_HIGH; DEBUG4_LOW

	#define DEBUG_PINS_INIT				DEBUG1_LOW; DEBUG2_LOW; DEBUG3_LOW; DEBUG4_LOW; DEBUG1_PORT_DIR |= (1 << DEBUG1_PIN); DEBUG2_PORT_DIR |= (1 << DEBUG2_PIN); DEBUG3_PORT_DIR |= (1 << DEBUG3_PIN); DEBUG4_PORT_DIR |= (1 << DEBUG4_PIN)
#endif

// LED definitions
#define LED_PORT						PORTF_OUT
#define LED_PORT_DIR                    PORTF_DIR

#define LED1_PIN						3		// Module pin 28, uC pin 49 - PF3.
#define LED2_PIN						2		// Module pin 29, uC pin 48 - PF2.
#define LED3_PIN						1		// Module pin 30, uC pin 47 - PF1.

// Interface board LED definitions
#define LED_GREEN						LED1
#define LED_YELLOW						LED2
#define LED_RED							LED3
#define LED_GREEN_PIN					LED1_PIN
#define LED_YELLOW_PIN					LED2_PIN
#define LED_RED_PIN						LED3_PIN

#define HEARTBEAT_LED					LED1
#define HOST_ACTIVITY_LED				LED2
#define RF_ACTIVITY_LED					LED3

#define LED_TIMER_INTERRUPT_INACTIVE	0
#define LED_TIMER_INTERRUPT_ACTIVE		1

#define LED_TIMER_PERIOD				(uint32_t)1000	// 1000 x 1usec = 1msec.
#define LED_BLINK_TIME					6				// 6 x 1msec = 5-6msec.

// Button Definitions
#define BUTTON_1_PORT					PORTE_IN
#define BUTTON_1_PORT_DIR				PORTE_DIR
#define BUTTON_1_PINCTRL				PORTE_PIN5CTRL
#define BUTTON_1_PIN					5		// Module pin 31, uC pin 41 - PE5.

#define BUTTON_2_PORT					PORTF_IN
#define BUTTON_2_PORT_DIR				PORTF_DIR
#define BUTTON_2_PINCTRL				PORTF_PIN0CTRL
#define BUTTON_2_PIN					0		// Module pin 30, uC pin 46 - PF0.

// Static test mode definitions
#define STATIC_TEST_PORT				PORTC_IN
#define STATIC_TEST_PORT_DIR			PORTC_DIR
#define STATIC_TEST_0_PINCTRL			PORTC_PIN5CTRL
#define STATIC_TEST_1_PINCTRL			PORTC_PIN6CTRL
#define STATIC_TEST_2_PINCTRL			PORTC_PIN7CTRL
#define STATIC_TEST_0_PIN				5		// Module pin 61, uC pin 21 - PC5.
#define STATIC_TEST_1_PIN				6		// Module pin 60, uC pin 22 - PC6.
#define STATIC_TEST_2_PIN				7		// Module pin 59, uC pin 23 - PC7.
#define STATIC_TEST_0_PIN_BITMASK		(1 << STATIC_TEST_0_PIN)
#define STATIC_TEST_1_PIN_BITMASK		(1 << STATIC_TEST_1_PIN)
#define STATIC_TEST_2_PIN_BITMASK		(1 << STATIC_TEST_2_PIN)

#define ANTENNA_RX_SWITCH_PORT			PORTF_OUT
#define ANTENNA_RX_SWITCH_PORT_DIR		PORTF_DIR
#define ANTENNA_RX_SWITCH_PINCTRL		PORTF_PIN4CTRL
#define ANTENNA_RX_SWITCH_PIN			4
#define SELECT_RX_ANTENNA_1				ANTENNA_RX_SWITCH_PORT |= (1 << ANTENNA_RX_SWITCH_PIN)
#define SELECT_RX_ANTENNA_2				ANTENNA_RX_SWITCH_PORT &= ~(1 << ANTENNA_RX_SWITCH_PIN)
#define ANTENNA_RX_SWITCH_INIT			ANTENNA_RX_SWITCH_PORT_DIR |= (1 << ANTENNA_RX_SWITCH_PIN); SELECT_RX_ANTENNA_1

//////////////////////////////////////////////////////////////////////////////////
// External Variable Declarations
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
// External Function Prototypes
//////////////////////////////////////////////////////////////////////////////////

extern void LedInit(void);
extern void LedSet(LedId_t Led, LedAction_t LedAction);
extern void LedsAllOff(void);
extern void LedBlink(LedId_t Led, uint16_t u16LedBlinkTime);
extern void ButtonInit(void);
extern ButtonState_t ButtonRead(ButtonId_t ButtonNumber);

#ifdef STATIC_RF_TEST_MODES
	extern void StaticRfTestModeInit(void);
	extern StaticRfTestMode_t GetStaticRfTestMode(void);
#endif


/////////////////////////////////////////////////////////////////////////////////

#endif	// SIFLEX_IO_H
