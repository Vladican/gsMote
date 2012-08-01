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
#include "cmd.h"
#include "chb_demo.h"

// set up the iostream here for the printf statement.
FILE file_str = FDEV_SETUP_STREAM(chb_putchar, NULL, _FDEV_SETUP_WRITE);

// command line message buffer and pointer
static U8 msg[MAX_MSG_SIZE];
static U8 *msg_ptr;

// uart character received flag
static volatile bool char_rcvd = false;

/**************************************************************************/
/*!
    This is the putchar function used for printf
*/
/**************************************************************************/
int chb_putchar(char c, FILE *unused)
{
    UART_DATA_REG = c; // prepare transmission
    while (!(UART_STATUS_REG & (1 << TRANSMIT_COMPLETE_BIT)));
    // wait until byte sendt
    UART_STATUS_REG |= (1 << TRANSMIT_COMPLETE_BIT); 
    return 0;
}

/**************************************************************************/
/*!
    This is the getchar function used when we see a character has been received.
*/
/**************************************************************************/
U8 uart_getchar()
{
    U8 c;
    while(!(UART_STATUS_REG & (1 << RECEIVE_COMPLETE_BIT)));  // wait for data
    c = UART_DATA_REG;
    return c;
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
static void cmd_menu()
{
    printf_P(PSTR("\n"));
    printf_P(PSTR("*************** CHIBI *******************\n"));
    printf_P(PSTR("CHIBI >> "));
}

/**************************************************************************/
/*!
    Parse the command line. This function tokenizes the command input, then
    searches for the command table entry associated with the commmand. Once found,
    it will jump to the corresponding function.
*/
/**************************************************************************/
void cmd_parse(char *cmd)
{
    U8 argc, i = 0;
    char *argv[30];
    cmd_t *cmd_tbl;

    fflush(stdout);

    // parse the command line statement 
    argv[i] = strtok(cmd, " ");
    do
    {
        argv[++i] = strtok(NULL, " ");
    } while ((i < 30) && (argv[i] != NULL));
    
    // get the command table 
    cmd_tbl = cmd_tbl_get();

    // parse the command table for valid command
    argc = i;
    for (i=0; cmd_tbl[i].cmd != NULL; i++)
    {
        if (!strcmp(argv[0], cmd_tbl[i].cmd))
        {
            cmd_tbl[i].func(argc, argv);
            cmd_menu();
            return;
        }
    }
    printf("CMD: Command not recognized.\n");

    cmd_menu();
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void cmd_handler()
{
    U8 c = uart_getchar();

    switch (c)
    {
    case '\r':
        // terminate the msg and reset the msg ptr. then send
        // it to the handler for processing.
        *msg_ptr = '\0';
        putchar('\n');
        cmd_parse((char *)msg);
        msg_ptr = msg;
        break;
    
    case '\b':
        putchar(c);
        if (msg_ptr > msg)
        {
            msg_ptr--;
        }
        break;
    
    default:
        putchar(c);
        *msg_ptr++ = c;
        break;
    }
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void cmd_init()
{
    // turn on power LED
    PWRLED_DIR |= 1<<PWRLED_PIN;
    PWRLED_PORT |= 1<<PWRLED_PIN;

    // setup the stdout stream
    stdout = &file_str;

    // init the msg ptr
    msg_ptr = msg;

    // init uart for 115200
    UART_PORT.DIRSET |= UART_TX_PIN;
    USARTD0_BAUDCTRLA = 0x03;
    USARTD0_BAUDCTRLB = 0xB0;
    USARTD0_CTRLA |= USART_RXCINTLVL_MED_gc;
    USARTD0_CTRLB = (1<<USART_RXEN_bp) | (1<<USART_TXEN_bp);
    USARTD0_CTRLC = 0x3;

    // enable low & med level interrupts to execute
    PMIC.CTRL |= PMIC_MEDLVLEN_bm;

    // turn on global interrupts
    sei();
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void cmd_poll()
{
    if (char_rcvd)
    {
        cmd_handler();
        char_rcvd = false;
    }
}

/**************************************************************************/
/*!
    UART Rx interrupt vector
*/
/**************************************************************************/
ISR(USARTD0_RXC_vect)
{
    char_rcvd = true;
}
