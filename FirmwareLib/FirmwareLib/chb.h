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
#ifndef CHIBI_H
#define CHIBI_H

#include "types.h"
#include "constants_and_globals.h"

// this enables the chibi stack to run in promiscuous mode
// this should only be used when raw data frames will be output for analysis. in normal
// usage, this should not be enabled.
#define CHB_PROMISCUOUS   0

#define CHB_HDR_SZ        9    // FCF + seq + pan_id + dest_addr + src_addr (2 + 1 + 2 + 2 + 2)
#define CHB_FCS_LEN       2
#define CHB_MAX_PAYLOAD   100


// frame_type = data
// security enabled = false
// frame pending = false
// ack request = false
// pan ID compression = true
#define CHB_FCF_BYTE_0    0x41    

// dest addr = 16-bit
// frame version = 802.15.4 (not 2003)
// src addr = 16-bit
#define CHB_FCF_BYTE_1    0x98

#define CHB_ACK_REQ_POS   5

enum
{
    CHB_SUCCESS                 = 0,
    CHB_SUCCESS_DATA_PENDING    = 1,
    CHB_CHANNEL_ACCESS_FAILURE  = 3,
    CHB_NO_ACK                  = 5,
    CHB_INVALID                 = 7
};

typedef struct
{
    U16 src_addr;
    U8 seq;
    volatile bool data_rcv;
    volatile bool tx_end;

    // stats
    U16 rcvd_xfers;
    U16 txd_success;
    U16 txd_noack;
    U16 txd_channel_fail;
    U16 overflow;
    U16 underrun;
    U8 battlow;
    U8 ed;
    U8 crc;
} pcb_t;

typedef struct
{
    U8 len;
    U16 src_addr;
    U16 dest_addr;
    U8 data[CHB_MAX_PAYLOAD];
} chb_rx_data_t;
//initialize radio and put it into listen mode
void chb_init();
//get the radio statistics which are encapsulated in the pcb struct (defined above)
pcb_t *chb_get_pcb();
//send a message using the radio. Takes a mote address (0xFFFF to broadcast), a pointer to the data to send and the length of the data to send.
//the function returns the status of the transmition: 0 if success, 5 if no acknowledgement received (only valid if non broadcast message) and 3 if channel access violation.
U8 chb_write(U16 addr, U8 *data, U32 len);
//read the data from the buffer where message is copied to when it is received. Should be done automatically when a message is received and the contents of the buffer are written to the FRAMReadBuffer.
//the function takes pointer to an array (min length of 128 bytes) and writes the contents of the buffer to it returning the status of the command (0 if successful). 
U8 chb_read(chb_rx_data_t *rx);
//enable pseudo interrupt on portE which triggers every time incoming radio message is stored in the message buffer
void radio_msg_received_int_enable();

#endif
