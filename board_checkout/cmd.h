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
    \file 
    \ingroup


*/
/**************************************************************************/
#ifndef CHB_CMD_H
#define CHB_CMD_H

#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "types.h"

#define MAX_MSG_SIZE    60

/* baud rate register value calculation */      
#define CPU_FREQ    2000000

/* definitions for UART control */      
#define UART_PORT                   PORTD
#define UART_TX_PIN                 PIN3_bm
#define BAUD_RATE_LOW_REG           USARTD0.BAUDCTRLA
#define UART_CONTROL_REG            USARTD0.CTRLB
#define ENABLE_TRANSMITTER_BIT      USART_TXEN_bp
#define ENABLE_RECEIVER_BIT         USART_RXEN_bp
#define UART_STATUS_REG             USARTD0.STATUS
#define TRANSMIT_COMPLETE_BIT       USART_TXCIF_bp
#define DATA_REG_EMPTY_BIT          USART_DREIF_bp
#define RECEIVE_COMPLETE_BIT        USART_RXCIF_bp
#define UART_DATA_REG               USARTD0.DATA

// definitions for the power LED
#define PWRLED_PORT PORTA_OUT
#define PWRLED_DIR  PORTA_DIR
#define PWRLED_PIN  7

// command line structure
typedef struct
{
    char *cmd;
    void (*func)(U8 argc, char **argv);
} cmd_t;

int chb_putchar(char c, FILE *unused);
void cmd_init();
void cmd_poll();

#endif
