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
//#include "chb.h"
#include "chb_spi.h"

/**************************************************************************/
/*!

*/
/**************************************************************************/

void chb_spi_init()
{
    // configure the SPI slave_select, spi clk, and mosi pins as output. the miso pin
    // is cleared since its an input.
    CHB_SPI_DDIR |= (1<<CHB_SSPIN) | (1<<CHB_MOSI) | (1<<CHB_SCK);
    CHB_SPI_PORT |= (1<<CHB_SSPIN);

    // set to master mode
    // set the clock freq to fck/16
    CHB_CTRL |= (1<<SPI_MASTER_bp) | (1<<SPI_ENABLE_bp) | (1<<SPI_PRESCALER_gp);

    // set the slave select to idle
    CHB_SPI_DISABLE();
}

/**************************************************************************/
/*!
    This function both reads and writes data. For write operations, include data
    to be written as argument. For read ops, use dummy data as arg. Returned
    data is read byte val.
*/
/**************************************************************************/
uint8_t SPID_write(uint8_t byteToSend){
	uint8_t data;
	SPID.DATA = byteToSend;
	while(!(SPID.STATUS & SPI_IF_bm)); //wait for byte to be sent
	data = SPID.DATA; //read SPI data register to reset status flag
	return data;
}