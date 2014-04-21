/*******************************************************************
    Copyright (C) 2009 FreakLabs
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name of the the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.

    Originally written by Christopher Wang aka Akiba.
    Please post support questions to the FreakLabs forum.
*******************************************************************/
/*!
    \file types.h
    \ingroup usb
*/
/*******************************************************************/
#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>

// Standard data types
typedef uint8_t     U8;     /// Generic 8 bit unsigned data type
typedef uint16_t    U16;    /// Generic 16 bit unsigned data type
typedef uint32_t    U32;    /// Generic 32 bit unsigned data type
typedef uint64_t    U64;    /// Generic 64 bit unsigned data type

typedef int8_t     S8;     /// Generic 8 bit signed data type
typedef int16_t    S16;    /// Generic 16 bit signed data type
typedef int32_t    S32;    /// Generic 32 bit signed data type

define LOCAL_DEVICE				0
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


#endif
