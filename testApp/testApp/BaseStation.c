/*
 * BaseStation.c
 *
 * Created: 1/29/2013 2:50:50 PM
 *  Author: Vlad
 */ 
# include "E-000001-000009_firmware_rev_1_0.h"

int main(){
	chb_init();
	SD_init();
	getBootSectorData();
	uint16_t MotesInSystem = 2;
	while(1){
		while(MotesReadyToSynch < MotesInSystem){
			//store samples in SD card as they come in
			if(StartOfFreeSpace >= 512){
				cli();
				for(uint32_t i=0;i<StartOfFreeSpace;i+=512){	//atomically write the data that accumulated in the FRAM buffer to the SD card
					writeFile("DATA",i);						
				}
				StartOfFreeSpace = 0;					
				sei();
			}				
		}
		//send synch message
		MotesReadyToSynch = 0;	//reset unsynched motes number
	}	
}