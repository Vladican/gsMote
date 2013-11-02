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
	uint32_t length;
	uint8_t NumMessages, NumReceivedMessages;
	chb_init();
	chb_set_short_addr(0x0000);
	chb_set_channel(1);
	StartSerial((uint32_t)57600);
	//radio_msg_received_int_enable();
	while(!chb_set_state(CHB_RX_AACK_ON) == RADIO_SUCCESS);
	pcb_t* pcb = chb_get_pcb();
	while(1){
		//wait for data over radio
		while(pcb->data_rcv){
			//read the data. expecting a 1 byte message containing number of messages that follow
			length = chb_read((chb_rx_data_t*)FRAMReadBuffer);
			if (length == 1){
				length = 0;
				//get the number of messages (1 byte)
				NumMessages = FRAMReadBuffer[0];
				while(NumReceivedMessages <NumMessages){
					//wait for all messages to come in. Add error check to make sure not writing data beyond the buffer limit
					if(pcb->data_rcv){
						length += chb_read((chb_rx_data_t*)(FRAMReadBuffer+length));
						NumReceivedMessages++;
					}
				}					
				//pass the data to USB
				SerialWriteBuffer(FRAMReadBuffer,length);
			}			
		}
		//check for inputs over serial
		length = 0;
		while(USARTC0.STATUS & BIT7_bm){
			//read bytes over serial into array
			FRAMReadBuffer[length] = SerialReadByte();
			length++;
		}
		if(length > 0){
			//process/send the bytes over radio
			//switch(FRAMReadBuffer[0])
			chb_write(0xFFFF,FRAMReadBuffer,length);
		}		
	}
}