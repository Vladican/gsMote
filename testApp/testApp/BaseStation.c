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
	//uint8_t data_byte;
	uint8_t length;
	chb_init();
	chb_set_short_addr(0x0001);
	chb_set_channel(0);
	StartSerial((uint32_t)9600);
	//radio_msg_received_int_enable();
	while(!chb_set_state(CHB_RX_AACK_ON) == RADIO_SUCCESS);
	pcb_t* pcb = chb_get_pcb();
	while(1){
		//wait for data over radio
		while(pcb->data_rcv){
			//read the data
			length = chb_read((chb_rx_data_t*)FRAMReadBuffer);
			//pass it to USB
			SerialWriteBuffer(FRAMReadBuffer,(uint32_t)length);
		}		
	}
}