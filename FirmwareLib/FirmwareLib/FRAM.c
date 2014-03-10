/*
 * FRAM.c
 *
 * Created: 3/8/2014 11:40:36 AM
 *  Author: VLAD
 */ 
#include "FRAM.h"

void writeFRAM(uint8_t* buffer, uint16_t length) {
	
	ADCPower(TRUE);
	SPIInit(SPI_MODE_0_gc);
	SPIC.CTRL = FR_SPI_CONFIG_gc;
	SPICS(TRUE);
	PORTB.OUTCLR = PIN3_bm;  // pull down CS_FRAM to write enable
	nop();
	SPIC.DATA = FR_WREN;
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	PORTB.OUTSET = PIN3_bm;  // latch opcode
	nop(); // time for CS_FRAM to accept high signal
	PORTB.OUTCLR = PIN3_bm;  // pull down CS_FRAM to write enable
	nop();
	SPIC.DATA = FR_WRITE;
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	//send address at which to start writing data
	SPIC.DATA = *(((uint8_t*)&FRAMAddress)+1);
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	SPIC.DATA = *((uint8_t*)&FRAMAddress);
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	//write data to FRAM
	for(uint32_t i = 0; i< length; i++){
		SPIC.DATA = buffer[i];
		while(!(SPIC.STATUS & SPI_IF_bm));
		SPIBuffer[12] = SPIC.DATA;
	}
	
	PORTB.OUTSET = PIN3_bm;  // pull up CS_FRAM to write protect
	SPICS(FALSE);
	//SPIC.CTRL = ADC_SPI_CONFIG_gc;
	//PORTC.OUTCLR = PIN4_bm;  // enable SPI-SS
	
	//increment address by the written length
	FRAMAddress +=length;
}

// Read from FRAM
// FRAM power (VDC-2) must be on with CS_FRAM pulled high to write protect
void readFRAM (uint16_t numBytes) {
	
	ADCPower(TRUE);
	SPIInit(SPI_MODE_0_gc);
	SPIC.CTRL = FR_SPI_CONFIG_gc;
	SPICS(TRUE);
	PORTB.OUTCLR = PIN3_bm;  // pull down CS_FRAM to write enable
	nop();
	
	SPIC.DATA = FR_READ;
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	SPIC.DATA = *(((uint8_t*)&FRAMAddress) + 1);;
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	SPIC.DATA = *(((uint8_t*)&FRAMAddress) + 0);;
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	
	for(uint16_t i = 0; i < numBytes; i++) {
		SPIC.DATA = 0xAA;
		while(!(SPIC.STATUS & SPI_IF_bm));
		FRAMReadBuffer[i] = SPIC.DATA;
	}

	PORTB.OUTSET = PIN3_bm;  // CS_FRAM write protect
	SPICS(FALSE);
	SPIDisable();

}
