//////////////////////////////////////////////////////////////////////////////////
//
//  Filename:	    types.h
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
//  Version
//
//////////////////////////////////////////////////////////////////////////////////

#ifndef TYPES_H
#define TYPES_H

#include "avrtypes.h"

#define LOCAL_DEVICE				0
#define REMOTE_DEVICE				1


typedef unsigned char 	uchar;
typedef uint16_t		word;


typedef struct _words 	// word structure
{
	uchar lb;	    	// low byte
	uchar hb;       	// high byte
} Words_t;


typedef union _wordu    // word union
{
	word		w;   	// word
	uint16_t 	u16; 	// unsigned integer
	Words_t  	ws;     // word structure
} Wordu_t;


typedef struct _dwords   // word structure
{
	uchar lb;	    	// low byte
	uchar mlb;	    	// middle low byte
	uchar mhb;      	// middle high byte
	uchar hb;       	// high byte
} DWords_t;


typedef union _dwordu
{
	uint32_t	u32;
	DWords_t	dws;
} DWordu_t;


typedef struct _sLocalRemoteAddress
{
	uint8_t u8LocalRemote;
	Wordu_t wuAddress;
} LocalRemoteAddress_t;


typedef struct _sSendMessageInfo
{
	uint8_t u8PacketType;
	uint8_t u8PacketId;
	uint8_t u8Retries;
	uint8_t u8MsgInProgress;
	uint8_t u8Status;
} SendMessageInfo_t;


typedef struct _sLastMessageInfo
{
	uint8_t u8Status;
	uint8_t u8PacketId;
	uint8_t u8RetriesLeft;
	uint8_t u8LinkQuality;
} LastMessageInfo_t;


typedef struct _sSecurityInfo
{
	uint8_t u8Status;
	uint8_t au8FrameCounter[4];
	uint8_t u8NwkHeaderLength;
} SecurityInfo_t;


#endif  // TYPES_H
