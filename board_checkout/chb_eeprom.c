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
#include "chb_eeprom.h"

/**************************************************************************/
/*!

*/
/**************************************************************************/
static void chb_eep_write_byte( U16 addr, U8 value )
{
    // flush the eeprom buffers
    /* Wait until NVM is not busy. */
    while ((NVM.STATUS & NVM_NVMBUSY_bm) == NVM_NVMBUSY_bm);

    /* Flush EEPROM page buffer if necessary. */
    if ((NVM.STATUS & NVM_EELOAD_bm) != 0) {
        NVM.CMD = NVM_CMD_ERASE_EEPROM_BUFFER_gc;
        NVM_EXEC();
    }

    // tell the non-volatile regs that we're going to load the eeprom addr
    NVM.CMD = NVM_CMD_LOAD_EEPROM_BUFFER_gc;

    // load the address
    NVM.ADDR0 = addr & 0xFF;
    NVM.ADDR1 = (addr >> 8) & 0x1F;
    NVM.ADDR2 = 0x00;

    // load the data to write
    NVM.DATA0 = value;

    // execute the eeprom write command
    NVM.CMD = NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc;
    NVM_EXEC();
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
static U8 chb_eep_read_byte(U16 addr)
{
    /* Wait until NVM is not busy. */
    while ((NVM.STATUS & NVM_NVMBUSY_bm) == NVM_NVMBUSY_bm);

    /* Set address to read from. */
    NVM.ADDR0 = addr & 0xFF;
    NVM.ADDR1 = (addr >> 8) & 0x1F;
    NVM.ADDR2 = 0x00;

    /* Issue EEPROM Read command. */
    NVM.CMD = NVM_CMD_READ_EEPROM_gc;
    NVM_EXEC();

    return NVM.DATA0;
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void chb_eeprom_write(U16 addr, U8 *buf, U16 size)
{
    // disable memory mapping
    NVM.CTRLB &= ~NVM_EEMAPEN_bm;

    // Write bytes
    for(U8 i=0; i<size; i++)
    {
        chb_eep_write_byte(addr+i, buf[i]);
    }
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void chb_eeprom_read(U16 addr, U8 *buf, U16 size)
{
    // disable memory mapping
    NVM.CTRLB &= ~NVM_EEMAPEN_bm;

    /* Write bytes.*/
    for(U8 i=0; i<size; i++)
    {
        buf[i] = chb_eep_read_byte(addr+i);
    }
}
