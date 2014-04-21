//////////////////////////////////////////////////////////////////////////////////
//
//  Filename:	    SiFLEX_Ping_Pong.h
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

#include "types.h"

#ifndef SIFLEX02_PING_PONG_H
#define SIFLEX02_PING_PONG_H

//////////////////////////////////////////////////////////////////////////////////
// Type Definitions
//////////////////////////////////////////////////////////////////////////////////

typedef struct sPingPongValues
{
	uint8_t u8PingPongEnabled;
	uint8_t u8Master;
	uint8_t u8RfChannel;
	uint8_t u8TimerCount;
	uint8_t u8LEDflashCounter;
	uint8_t u8TimeoutEvent;
} PingPongValues_t;

typedef struct sTransceiverParams
{
	Wordu_t	wuPanId;
	Wordu_t wuShortTransceiverId;
	uint64_t u64LongTransceiverId;
	uint8_t u8RfChannelNumber;
	uint8_t u8RfTxPowerLevel;
}  TransceiverParams_t;

//////////////////////////////////////////////////////////////////////////////////
// Definitions
//////////////////////////////////////////////////////////////////////////////////

#define PING_PONG_NORMAL_STATE_TIME_PERIOD					200000     	// 200000 x 1usec => 200msec.
#define HEART_BEAT_TIME_PERIOD								2000000		// 2,000,000 x 1usec => 2 seconds.
#define PING_PONG_CHANGE_CHANNEL_STATE_TIME_PERIOD			150000     	// 150000 x 1usec => 150msec.
#define VOLTAGE_READ_TIME_PERIOD							1000000		// 1,000,000 x 1usec => 1 second		

#define PING_PONG_LQI_EXCELLENT								75
#define PING_PONG_LQI_GOOD									30

#define PING_PONG_RF_MSG_LENGTH								10
#define PING_PONG_RF_MSG_TYPE								0x70

#define RF_CHANNEL_MIN										1
#define RF_CHANNEL_MID										5
#define RF_CHANNEL_MAX										10
#define RF_TX_POWER_LEVEL_MAX								21			// There are 22 entries in the table that range from 0 to 21.
#define MAXIMUM_SUPPLY_VOLTAGE_TRANSMIT_THRESHOLD			3550		// Max transmit threshold voltage in millivolts ((3.55v x 1000) => 3550).

//////////////////////////////////////////////////////////////////////////////////
// External Variable Declarations
//////////////////////////////////////////////////////////////////////////////////

extern void (*ptrPingPongState)(void);
extern PingPongValues_t PingPongValues;
extern TransceiverParams_t TransceiverParams;

//////////////////////////////////////////////////////////////////////////////////
// External Function Prototypes
//////////////////////////////////////////////////////////////////////////////////

extern void SendPingPongRfMessage(void);
extern void InitPingPongStateMachine(void);
extern void PingPongNormalState_1(void);
extern void PingPongNormalState_2(void);

/////////////////////////////////////////////////////////////////////////////////

#endif	//end SIFLEX02_PING_PONG_H
