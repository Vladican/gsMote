/*
 * BaseStation.c
 *
 * Created: 1/29/2013 2:50:50 PM
 *  Author: Vlad
 */ 
# include "E-000001-000009_firmware_rev_1_0.h"

int main(){
	/*
	moteID = 0;
	RadioMonitorMode = DATA_GATHERING;
	char message[15];
	strcpy(message,"start sampling");
	chb_init();
	chb_set_short_addr(moteID);
 	SD_init();
 	getBootSectorData();
	uint16_t MotesInSystem = 1;
	while(1){
		while(MotesReadyToSynch < MotesInSystem){
			//store samples in SD card as they come in
			if(StartOfFreeSpace >= 512){
				cli();
				//atomically write the data that accumulated in the FRAM buffer to the SD card
				writeFile("DATA",FRAMReadBuffer,StartOfFreeSpace);						
				StartOfFreeSpace = 0;					
				sei();
			}				
		}
		//send synch message
		MotesReadyToSynch = 0;	//reset unsynched motes number
		chb_write(0xFFFF,message,strlen(message));
	}	
	*/
	uint8_t data_byte;
	//set F_CPU/F_PER to 32 MHz (default is the 2 MHz RC oscillator)
	set_32MHz();
	//set output on transmit pin
	PORTC.DIRSET = PIN3_bm;
	PORTC.OUTSET = PIN3_bm;
	//set input on receive pin
	PORTC.DIRCLR = PIN2_bm;
	//prescalar of 15: baud = F_CPU/((2^bscale)*16*(scaler+1)) = 125000 (almost 128000)
	USARTC0.BAUDCTRLA = 15;
	//8 data bits no parity 1 stop bit
	USARTC0.CTRLC = 3;
	//turn on Rx and Tx for USART
	USARTC0.CTRLB = BIT4_bm | BIT3_bm;
	while(1){
		//wait for reception of message
		if ((USARTC0.STATUS & BIT7_bm) == BIT7_bm){
			//read in byte
			data_byte = USARTC0.DATA;
			//increment byte
			data_byte++;
			//wait for transmit buffer to become available
			while((USARTC0.STATUS & BIT5_bm) != BIT5_bm){
				//wait
			}				
			//transmit incremented byte
			USARTC0.DATA = data_byte;
			//wait for transmit to finish
			while((USARTC0.STATUS & BIT6_bm) != BIT6_bm) {
			//wait
			}
		}
	}
}