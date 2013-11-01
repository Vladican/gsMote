/*
 * CFile1.c
 *
 * Created: 9/20/2013 9:50:45 AM
 *  Author: Vlad
 */ 
#include "SerialUSB.h"

bool StartSerial(uint32_t BaudRate){
	uint16_t prescaler;
	if(BaudRate <600 || BaudRate >1000000){
		//baud rate too fast or too slow
		return false;
	}
	//set F_CPU/F_PER to 32 MHz (default is the 2 MHz RC oscillator)
	set_32MHz();
	//set output on transmit pin
	PORTC.DIRSET = PIN3_bm;
	PORTC.OUTSET = PIN3_bm;
	//set input on receive pin
	PORTC.DIRCLR = PIN2_bm;
	//prescalar of 15: baud = F_CPU/((2^bscale)*16*(scaler+1)) = 125000 (almost 128000)
	prescaler = (uint32_t)((((float)F_CPU)/((float)(16*BaudRate)))-1);
	//increment prescaler if truncated part was >= 0.5
	if((((float)F_CPU/((float)(16*BaudRate)))-1)-prescaler >= 0.5) prescaler++;
	
	USARTC0.BAUDCTRLA = prescaler & 0xFF;
	USARTC0.BAUDCTRLB = prescaler >>8;
	//8 data bits no parity 1 stop bit
	USARTC0.CTRLC = 3;
	//turn on Rx and Tx for USART
	USARTC0.CTRLB = BIT4_bm | BIT3_bm;
	return true;
}

void SerialWriteByte(uint8_t byte){
	//wait for transmit buffer to become available
	while((USARTC0.STATUS & BIT5_bm) != BIT5_bm){
		//wait
	}
	//send byte
	USARTC0.DATA = byte;
	//wait for transmit to finish
	//while((USARTC0.STATUS & BIT6_bm) != BIT6_bm) {
		//wait
	//}
}

uint8_t SerialReadByte(){
	uint8_t byte;
	//wait for reception of message
	while ((USARTC0.STATUS & BIT7_bm) != BIT7_bm){
		//add timeout logic
	}	
	//read in byte
	byte = USARTC0.DATA;
	return byte;	
}

void SerialWriteBuffer(uint8_t* buffer, uint32_t length){
	uint32_t i;
	for(i=0;i<length;i++){
		SerialWriteByte(buffer[i]);
	}
}

void StopSerial(){
	//turn off Rx and Tx for USART
	USARTC0.CTRLB &= ~(BIT4_bm | BIT3_bm);
	//clear output pin
	PORTC.OUTCLR = PIN3_bm;
	PORTC.DIRCLR = PIN3_bm;
}