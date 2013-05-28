/*
 * BaseStation.c
 *
 * Created: 1/29/2013 2:50:50 PM
 *  Author: Vlad
 */ 
# include "E-000001-000009_firmware_rev_1_0.h"

int main(){
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
}