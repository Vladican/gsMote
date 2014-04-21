//////////////////////////////////////////////////////////////////////////////////
//
//  Filename:	    SiFLEX_Ping_Pong.c
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

//////////////////////////////////////////////////////////////////////////////////
//      Includes                                                                //
//////////////////////////////////////////////////////////////////////////////////

#include "ieee_const.h"
#include "mac_api.h"
#include "SiFlex_IO.h"
#include "adc_driver.h"
#include "SiFLEX02_Ping_Pong.h"
#include "app_config.h"


//////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
//////////////////////////////////////////////////////////////////////////////////

uint8_t ConvertTransmitPowerToRegisterValue(uint8_t u8TxPowerLevel);
uint8_t CheckForStartPingPong(void);
void HeartbeatTimeoutCB(void);
void PingPongChangeRfChannel(void);
void PingPongChangeChannelState(void);
void PingPongNormalMode_1_State(void);
void PingPongNormalMode_2_State(void);
void InitPingPongStateMachine(void);
void PingPongStartState(void);
void PingPongTimeoutCB(void);
void PingPongWaitForDeviceStartState(void);
void SendPingPongRfMessage(void);
void ConfigureAdcForPowerSupplyReading(void);

//////////////////////////////////////////////////////////////////////////////////
// Variable Declarations
//////////////////////////////////////////////////////////////////////////////////

extern uint8_t u8DeviceStarted;
uint8_t u8TxMsgHandle;
uint8_t au8PingPongRfPacket[PING_PONG_RF_MSG_LENGTH + 17];		// Fourteen extra bytes for the header at the front, plus three at the back for channel and FCF.
void (*ptrPingPongState)(void);
SendMessageInfo_t SendMessageInfo;
PingPongValues_t PingPongValues;
TransceiverParams_t TransceiverParams;


//////////////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Callback function for TIMER_PING_PONG
 *
 * Code within the state machine needs to be slowed down
 * for user input.  PingPongValues.u8TimeoutEvent is checked 
 * before entering those states.  The timer executes this function
 * every 200ms.  See app_config.h to see where timers are declared.
 */
void PingPongTimeoutCB(void)
{
	PingPongValues.u8TimeoutEvent = true;
}  //end PingPongTimeoutCB


/**
 * @brief Callback function for TIMER_HEARTBEAT
 *
 * Indicates that the module is ready to begin ping pong via green LED.
 * The timer executes this function every 2sec.
 */
void HeartbeatTimeoutCB(void)
{
	if (PingPongValues.u8PingPongEnabled == false)	//only blink prior to ping pong
	{
		LedBlink(HEARTBEAT_LED, LED_BLINK_TIME);
		pal_timer_start(TIMER_HEARTBEAT, HEART_BEAT_TIME_PERIOD, TIMEOUT_RELATIVE, (void*)HeartbeatTimeoutCB, NULL);	//reset the timer
	}
} //end HeartbeatTimeoutCB


/**
 * @brief Callback function for VOLTAGE_READ
 *
 * This callback is executed every second.  If the voltage is above
 * 3.55 volts the code wil remain in a loop until the volage drops below
 * that value.
 */
void VoltageReadTimeoutCB()
{
	Wordu_t wuSupplyVoltageRaw;
	DWordu_t dwuSupplyVoltageMillivolts;
	while(1)
	{
		ADC_Ch_Conversion_Start(&ADCA.CH3);		// Start the ADC conversion for channel 3, which is configured to read the supply voltage.

		do{}while(!ADC_Ch_Conversion_Complete(&ADCA.CH3));
		wuSupplyVoltageRaw.u16 = ADC_ResultCh_GetWord(&ADCA.CH3);
		// Reference voltage is 1.0 volts.
		// Input voltage is divided by 10
		// Need to multiply by an additional 1000 to get value ready to be in mV.
		dwuSupplyVoltageMillivolts.u32 = 10000;
		dwuSupplyVoltageMillivolts.u32 = (dwuSupplyVoltageMillivolts.u32 * wuSupplyVoltageRaw.u16);
		// Divide by 11bit resolution
		dwuSupplyVoltageMillivolts.u32 = (dwuSupplyVoltageMillivolts.u32 / 2047);

		if (dwuSupplyVoltageMillivolts.u32 < MAXIMUM_SUPPLY_VOLTAGE_TRANSMIT_THRESHOLD)
		{
			break;
		}
	}

	pal_timer_start(VOLTAGE_READ, VOLTAGE_READ_TIME_PERIOD, TIMEOUT_RELATIVE, (void*)VoltageReadTimeoutCB, NULL);	//reset the timer
}	//end VoltageReadTimeoutCB


/**
 * @brief Function responsible for changing Rf Channel in application.
 * Rf channel is physically set in PingPongChangeChannelState() and PingPongStartState()
 */
void PingPongChangeRfChannel(void)
{
	if (PingPongValues.u8RfChannel == RF_CHANNEL_MIN)
	{
		PingPongValues.u8RfChannel = RF_CHANNEL_MID;
	}
	else if (PingPongValues.u8RfChannel == RF_CHANNEL_MID)
	{
		PingPongValues.u8RfChannel = RF_CHANNEL_MAX;
	}
	// Else if channel is 10.
	else
	{
		PingPongValues.u8RfChannel = RF_CHANNEL_MIN;
	}
}  //end PingPongChangeRfChannel


/**
 * @brief Function responsible for setting up ADC power supply readings
 *
 */
void ConfigureAdcForPowerSupplyReading(void)
{
	ADC_CalibrationValues_Set(&ADCA);	// Move stored calibration values to ADC A.

	// Set up ADC A to have signed conversion mode and 12 bit resolution.
  	ADC_ConvMode_and_Resolution_Config(&ADCA, true, ADC_RESOLUTION_12BIT_gc);

	ADC_Prescaler_Config(&ADCA, ADC_PRESCALER_DIV512_gc);		// Sample rate is CPUFREQ/512.
	ADC_Reference_Config(&ADCA, ADC_REFSEL_INT1V_gc);			// Set reference voltage on ADC A to be internal 1.0v.


	// Setup channel 3 to use the internal inputs.
	ADC_Ch_InputMode_and_Gain_Config(&ADCA.CH3, ADC_CH_INPUTMODE_INTERNAL_gc, ADC_CH_GAIN_1X_gc);

	// Set input to the Vcc divided by 10.
	ADC_Ch_InputMux_Config(&ADCA.CH3, ADC_CH_MUXINT_SCALEDVCC_gc, ADC_CH_MUXNEG_PIN0_gc);

	ADC_Enable(&ADCA);				// Enable ADC A with internal 1.0v reference.

	ADC_Wait_32MHz(&ADCA);			// Wait until the common mode voltage is stable.

	VoltageReadTimeoutCB();
	//pal_timer_start(VOLTAGE_READ, VOLTAGE_READ_TIME_PERIOD, TIMEOUT_RELATIVE, (void*)VoltageReadTimeoutCB, NULL);	//reset the timer
}  //end ConfigureAdcForPowerSupplyReading


/**
 * @brief Function responsible for sending Ping Pong message
 */
void SendPingPongRfMessage(void)
{
	wpan_addr_spec_t SrcAddr;
    wpan_addr_spec_t DstAddr;
    uint8_t u8TxLength;

	// The master is address 100 and the slave is address 200.
	SrcAddr.AddrMode = WPAN_ADDRMODE_SHORT;
	SrcAddr.PANId = 0x1234;
	DstAddr.AddrMode = WPAN_ADDRMODE_SHORT;
	DstAddr.PANId = 0x1234;

	au8PingPongRfPacket[0] = PING_PONG_RF_MSG_TYPE;

	if (PingPongValues.u8Master == true)
	{
		SrcAddr.Addr.short_address = 100;			// Master is address 100.
		DstAddr.Addr.short_address = 200;			// Slave is address 200.
	}
	else
	{
		SrcAddr.Addr.short_address = 200;			// Slave is address 200.
		DstAddr.Addr.short_address = 100;			// Master is address 100.
	}

	SendMessageInfo.u8Retries = false;

	u8TxLength = (PING_PONG_RF_MSG_LENGTH + 2);

	u8TxMsgHandle++;

	wpan_mcps_data_req(SrcAddr.AddrMode, &DstAddr, u8TxLength, (uint8_t*)&au8PingPongRfPacket, u8TxMsgHandle, WPAN_TXOPT_OFF);
}  //end SendPingPongRfMessage


/**
 * @brief Function called to indicate if user pushed USER1 or 
 * USER2 on the board
 */
uint8_t CheckForStartPingPong(void)
{
	uint8_t u8Ret = false;

	// If button 1 is pressed then set to master.
	if (ButtonRead(BUTTON_1) == BUTTON_PRESSED)
	{
		PingPongValues.u8Master = true;
		LedSet(LED_GREEN, LED_ON);
    	u8Ret = true;
	}
	// Else if button 2 is pressed, then set to slave.
	else if (ButtonRead(BUTTON_2) == BUTTON_PRESSED)
	{
		PingPongValues.u8Master = false;
		LedSet(LED_RED, LED_ON);
	    u8Ret = true;
	}

	while ((ButtonRead(BUTTON_1) == BUTTON_PRESSED) || (ButtonRead(BUTTON_2) == BUTTON_PRESSED));	// Wait for the user to release the buttons.
	
	return u8Ret;
}  //end CheckForStartPingPong


/**
 * @brief Function used to initiate the state machine.
 * First state executed in the state machine
 */
void InitPingPongStateMachine(void)
{
	// Configures and ultimately begins voltage sampling.
	ConfigureAdcForPowerSupplyReading();
	HeartbeatTimeoutCB();
	// Configure buttons 1 and 2 as inputs with pullups enabled.
	BUTTON_1_PINCTRL = (PORT_OPC_PULLUP_gc + PORT_ISC_BOTHEDGES_gc);
	BUTTON_1_PORT_DIR &= ~(1 << BUTTON_1_PIN);		// Set port PE5 as an input for button 1.
	BUTTON_2_PINCTRL = (PORT_OPC_PULLUP_gc + PORT_ISC_BOTHEDGES_gc);
	BUTTON_2_PORT_DIR &= ~(1 << BUTTON_2_PIN);		// Set port PF0 as an input for button 2.

	// Make pins PF1, PF2, and PF3 outputs for driving the red, yellow, and green LEDs respectively.
	LED_PORT |= (1 << LED_GREEN_PIN);
    LED_PORT |= (1 << LED_YELLOW_PIN);
    LED_PORT |= (1 << LED_RED_PIN);
    LED_PORT_DIR |= (1 << LED_GREEN_PIN);
    LED_PORT_DIR |= (1 << LED_YELLOW_PIN);
    LED_PORT_DIR |= (1 << LED_RED_PIN);

	ptrPingPongState = PingPongWaitForDeviceStartState;
}  //end InitPingPongStateMachine


/**
 * @brief Function called after state machine has been initiated.
 * Second state executed in the state machine.  Waits for the
 * module to be up and running, then for user input.
 */
void PingPongWaitForDeviceStartState(void)
{
// 	if (u8DeviceStarted == true)
// 	{
// 		if (CheckForStartPingPong() == true)
// 		{			
// 			PingPongValues.u8PingPongEnabled = true;
// 			PingPongValues.u8RfChannel = 5;
// 			PingPongValues.u8TimerCount = 10;
// 			PingPongValues.u8TimeoutEvent = false;
// 			
// 			// Before entering the next state, reset the timer
// 			pal_timer_start(TIMER_PING_PONG, PING_PONG_NORMAL_STATE_TIME_PERIOD, TIMEOUT_RELATIVE, (void *)PingPongTimeoutCB, NULL);
// 			ptrPingPongState = PingPongStartState;
// 		}
// 	}
}  //end PingPongWaitForDeviceStartState


/**
 * @brief Function called after state machine and board are initialized.
 * Third state executed in state machine.  Physically sets transceiver parameters.
 */
void PingPongStartState(void)
{
	if (PingPongValues.u8TimeoutEvent == true)
	{
		TransceiverParams.u8RfTxPowerLevel = RF_TX_POWER_LEVEL_MAX;
		wpan_mlme_set_req(phyTransmitPower, &TransceiverParams.u8RfTxPowerLevel);
		
		if (PingPongValues.u8Master == true)
		{
			TransceiverParams.wuShortTransceiverId.u16 = 100;   	// The master is address 100.
		}
		else
		{
			TransceiverParams.wuShortTransceiverId.u16 = 200;    // The slave is address 200.
		}

		wpan_mlme_set_req(phyCurrentChannel, &PingPongValues.u8RfChannel);

		TransceiverParams.wuPanId.w = 0x1234;
		wpan_mlme_set_req(macPANId, &TransceiverParams.wuPanId);
		wpan_mlme_set_req(macShortAddress, &TransceiverParams.wuShortTransceiverId);

		LedSet(LED_GREEN, LED_OFF);
		LedSet(LED_YELLOW, LED_OFF);
		LedSet(LED_RED, LED_OFF);

		// Reset the timer and the check that prevents code being executed until timer has ran
		PingPongValues.u8TimeoutEvent = false;
		pal_timer_start(TIMER_PING_PONG, PING_PONG_NORMAL_STATE_TIME_PERIOD, TIMEOUT_RELATIVE, (void *)PingPongTimeoutCB, NULL);
		ptrPingPongState = PingPongNormalState_1;
	}
}  //end PingPongStartState


/**
 * @brief One of two "normal" states.  This state sends the message
 * if board is a master.  Also checks to see if the user wants to change
 * the Rf channel.
 */
void PingPongNormalState_1(void)
{
	if (PingPongValues.u8TimeoutEvent == true)
	{
		if (PingPongValues.u8Master == true)
		{
			SendPingPongRfMessage();
		}

		// If the button is held down for more than 2 seconds.
		if (ButtonRead(BUTTON_1) == BUTTON_PRESSED)
		{
			// 10 x 200ms = 2 sec.
			PingPongValues.u8TimerCount--;
			if (PingPongValues.u8TimerCount == 0)
			{
				LedSet(LED_GREEN, LED_ON);

				while (ButtonRead(BUTTON_1) == BUTTON_PRESSED);  // Wait for the user to release the button.

				PingPongValues.u8TimerCount = 14;      // Reset the button press count.
				PingPongValues.u8LEDflashCounter = 0;      // Reset the LED channel flash counter.
				// Prepare for next state.
				PingPongValues.u8TimeoutEvent = false;
				pal_timer_start(TIMER_PING_PONG, PING_PONG_NORMAL_STATE_TIME_PERIOD, TIMEOUT_RELATIVE, (void *)PingPongTimeoutCB, NULL);
				ptrPingPongState = PingPongChangeChannelState;
			}
			else
			{
				// Prepare for next state.
				PingPongValues.u8TimeoutEvent = false;
				pal_timer_start(TIMER_PING_PONG, PING_PONG_NORMAL_STATE_TIME_PERIOD, TIMEOUT_RELATIVE, (void *)PingPongTimeoutCB, NULL);
				ptrPingPongState = PingPongNormalState_2;
			}
		}
		else
		{
			// Prepare for next state.
			PingPongValues.u8TimerCount = 10;      // Reset the button press count (10 x 200msec = 2 seconds).
			PingPongValues.u8TimeoutEvent = false;
			pal_timer_start(TIMER_PING_PONG, PING_PONG_NORMAL_STATE_TIME_PERIOD, TIMEOUT_RELATIVE, (void *)PingPongTimeoutCB, NULL);
			ptrPingPongState = PingPongNormalState_2;
		}
	}
}  //end PingPongNormalState_1


/**
 * @brief Second of two "normal" states.  This state allows a "pause" for
 * the board that is a slave to receive a message.  Also checks to see 
 * if the user wants to change the Rf channel.
 */
void PingPongNormalState_2(void)
{
	if (PingPongValues.u8TimeoutEvent == true)
	{
		// If the button is held down for more than 2 seconds.
		if (ButtonRead(BUTTON_1) == BUTTON_PRESSED)
		{
			// 10 x 200ms = 2 sec.
			PingPongValues.u8TimerCount--;
			if (PingPongValues.u8TimerCount == 0)
			{
				LedSet(LED_GREEN, LED_ON);

				while (ButtonRead(BUTTON_1) == BUTTON_PRESSED);  // Wait for the user to release the button.

				PingPongValues.u8TimerCount = 14;      // Reset the button press count.
				PingPongValues.u8LEDflashCounter = 0;      // Reset the LED channel flash counter.
				// Prepare for next state.
				PingPongValues.u8TimeoutEvent = false;
				pal_timer_start(TIMER_PING_PONG, PING_PONG_NORMAL_STATE_TIME_PERIOD, TIMEOUT_RELATIVE, (void *)PingPongTimeoutCB, NULL);
				ptrPingPongState = PingPongChangeChannelState;
			}
			else
			{
				// Prepare for next state.
				PingPongValues.u8TimeoutEvent = false;
				pal_timer_start(TIMER_PING_PONG, PING_PONG_NORMAL_STATE_TIME_PERIOD, TIMEOUT_RELATIVE, (void *)PingPongTimeoutCB, NULL);
				ptrPingPongState = PingPongNormalState_1;
			}
		}
		else
		{
			// Prepare for next state.
			PingPongValues.u8TimerCount = 10;      // Reset the button press count (10 x 200msec = 2 seconds).
			PingPongValues.u8TimeoutEvent = false;
			pal_timer_start(TIMER_PING_PONG, PING_PONG_NORMAL_STATE_TIME_PERIOD, TIMEOUT_RELATIVE, (void *)PingPongTimeoutCB, NULL);
			ptrPingPongState = PingPongNormalState_1;
		}
	}
}  //end PingPongNormalState_2


/**
 * @brief State responsible for allowing the user to change the Rf channel.
 */
void PingPongChangeChannelState(void)
{
	if (PingPongValues.u8TimeoutEvent == true)
	{
		PingPongValues.u8LEDflashCounter++;
		// If Rf channel is min, mid, or max there will be 
		// 1, 2, or 3 respective LED flashes with 300ms between
		// them
		if (PingPongValues.u8LEDflashCounter == 1)
		{
			LedSet(LED_RED, LED_ON);
		}
		// Difference of 2 between count since last if -> 2x150ms = 300ms
		else if (PingPongValues.u8LEDflashCounter == 3)
		{
			// If channel MID or MAX, then turn on the LED.
			if ((PingPongValues.u8RfChannel == RF_CHANNEL_MID) || (PingPongValues.u8RfChannel == RF_CHANNEL_MAX))
			{
				LedSet(LED_RED, LED_ON);
			}
		}
		// Difference of 2 between count since last else if -> 2x150ms = 300ms
		else if (PingPongValues.u8LEDflashCounter == 5)
		{
			// If channel 10, then turn on the LED.
			if (PingPongValues.u8RfChannel == RF_CHANNEL_MAX)
			{
				LedSet(LED_RED, LED_ON);
			}
		}
		else
		{
			LedSet(LED_RED, LED_OFF);
			// Difference of 9 since priro else if -> 9x150ms = 1.3 seconds between LED flash[s]
			if (PingPongValues.u8LEDflashCounter == 14)
			{
				PingPongValues.u8LEDflashCounter = 0;
			}
		}
		// If the button is held down for more than 2.1 seconds (14 x 150msec => 2.1 seconds).
		if (ButtonRead(BUTTON_1) == BUTTON_PRESSED)
		{
			PingPongValues.u8TimerCount--;
			if (PingPongValues.u8TimerCount == 0)
			{
				LedSet(LED_GREEN, LED_OFF);
				LedSet(LED_YELLOW, LED_OFF);
				LedSet(LED_RED, LED_OFF);
				PingPongValues.u8TimerCount = 10;
				wpan_mlme_set_req(phyCurrentChannel, &PingPongValues.u8RfChannel);  // Physicall set Rf channel.

				while (ButtonRead(BUTTON_1) == BUTTON_PRESSED);  // Wait for the user to release the button.
				// Prepare for next state.
				PingPongValues.u8TimeoutEvent = false;
				pal_timer_start(TIMER_PING_PONG, PING_PONG_NORMAL_STATE_TIME_PERIOD, TIMEOUT_RELATIVE, (void *)PingPongTimeoutCB, NULL);
				ptrPingPongState = PingPongNormalState_1;
			}
		}
		// Button was pressed to change Rf channel
		else
		{
			if(PingPongValues.u8TimerCount != 14)
			{
				LedSet(LED_RED, LED_OFF);
				PingPongValues.u8LEDflashCounter = 0;
				PingPongChangeRfChannel();
			}

			PingPongValues.u8TimerCount = 14;
		}
		// Remain in this state.
		PingPongValues.u8TimeoutEvent = false;
		pal_timer_start(TIMER_PING_PONG, PING_PONG_CHANGE_CHANNEL_STATE_TIME_PERIOD, TIMEOUT_RELATIVE, (void *)PingPongTimeoutCB, NULL);
	}
}  //end PingPongChangeChannelState
