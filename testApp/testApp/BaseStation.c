/*
 * BaseStation.c
 *
 * Created: 1/29/2013 2:50:50 PM
 *  Author: Vlad
 */ 
# include "E-000001-000009_firmware_rev_1_0.h"

int main(){

	uint32_t length;
	uint16_t dest_addr;
	uint8_t  MessageBuffer[100];
	uint16_t NumReceivedMessages, NumMessages;
	chb_init();
	chb_set_short_addr(0x0000);
	chb_set_channel(1);
	StartSerial((uint32_t)57600);

	while(!chb_set_state(CHB_RX_AACK_ON) == RADIO_SUCCESS);
	pcb_t* pcb = chb_get_pcb();
	while(1){
		length = 0;
		//wait for inputs over serial
		while(!(USARTC0.STATUS & BIT7_bm));
		length = SerialReadByte();
		for(uint32_t i = 0; i<length; i++){
			MessageBuffer[i] = SerialReadByte();
		}
		dest_addr = (uint16_t)(MessageBuffer[0]);
			
		
		if(length > 0){
			//process/send the bytes over radio
			chb_write(dest_addr,MessageBuffer+1,length-1);
		}
		
		//wait for response/data over radio
		while(!pcb->data_rcv);
		//read the data. expecting a 1 byte message containing number of messages that follow
		length = chb_read((chb_rx_data_t*)FRAMReadBuffer);
		if (length == 2){
			length = 0;
			NumReceivedMessages = 0;
			//get the number of messages (2 bytes)
			NumMessages = ((uint16_t*)FRAMReadBuffer)[0];
			while(NumReceivedMessages <NumMessages){
				//wait for all messages to come in
				if(pcb->data_rcv){
					length = chb_read((chb_rx_data_t*)(FRAMReadBuffer));
					//pass the data to USB
					SerialWriteBuffer(FRAMReadBuffer,length);
					NumReceivedMessages++;
				}
			}
			//SerialWriteBuffer(FRAMReadBuffer,length);
		}	
	}
}