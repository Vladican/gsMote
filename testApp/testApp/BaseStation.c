/*
 * BaseStation.c
 *
 * Created: 1/29/2013 2:50:50 PM
 *  Author: Vlad
 */ 
# include "E-000001-000009_firmware_rev_1_0.h"

volatile uint8_t TimedOut = 0;

int main(){

	uint32_t length;
	uint16_t dest_addr;
	uint16_t ack = 0;
	uint8_t  MessageBuffer[100];
	uint16_t NumReceivedMessages, NumMessages, TimeoutCount;
	//set timeout about 2 sec
	uint16_t timeout = 4000;
	
	set_32MHz();
	
	chb_init();
	chb_set_short_addr(0x0000);
	chb_set_channel(1);
	chb_set_pwr(0);
	StartSerial((uint32_t)57600);

	while(!chb_set_state(CHB_RX_AACK_ON) == RADIO_SUCCESS);
	pcb_t* pcb = chb_get_pcb();
	
	
	//set the period to 2 seconds
	TCE0.PER = timeout;
	TCE0.CTRLFSET = 0x08;
	//set overflow interrupt
	TCE0.INTCTRLA = TC_OVFINTLVL_LO_gc;
	PMIC.CTRL |= PMIC_LOLVLEN_bm;
	sei();
	
	while(1){
		length = 0;
		//wait for inputs over serial
		while(!(USARTC0.STATUS & BIT7_bm));
		length = SerialReadByte();
		for(uint32_t i = 0; i<length; i++){
			MessageBuffer[i] = SerialReadByte();
		}
		dest_addr = ((uint16_t*)MessageBuffer)[0];
			
		
		if(length > 2){
			//clear out message buffer in case any stray message was received
			if(pcb->data_rcv){ 
				chb_read((chb_rx_data_t*)FRAMReadBuffer);
			}
			TCE0.CTRLA = 0x07;
			TimedOut = 0;				
			//process/send the bytes over radio
			while(chb_write(dest_addr,MessageBuffer+2,length-2) != CHB_SUCCESS){
				if(TimedOut) break;
			}
			if(TimedOut) {
				TCE0.CTRLA = 0x00;
				TCE0.CTRLFSET = 0x08;
				continue;
			}				
		
			TCE0.CTRLFSET = 0x08;
			TimedOut = 0;
			//set prescalar of 1024... each timer tick is 512 micro seconds
			//TCE0.CTRLA = 0x07;
			//wait for response/data over radio if the message was sent to only 1 mote and not broadcast
			if(dest_addr != 0xFFFF){
				while(!pcb->data_rcv){
					//no response detected so go back to waiting for next serial command
					if(TimedOut) break;
					//if(TCF0.CNT - TimeoutCount >= timeout) break;
				}
 				if(TimedOut) {
					TCE0.CTRLA = 0x00;
					TCE0.CTRLFSET = 0x08;
 					continue;
 				}			
				//if(TCF0.CNT - TimeoutCount >= timeout) continue;
				//read the data. expecting a 1 byte message containing number of messages that follow
				length = chb_read((chb_rx_data_t*)FRAMReadBuffer);
				if (length == 2){
					length = 0;
					NumReceivedMessages = 0;
					//get the number of messages (2 bytes)
					NumMessages = ((uint16_t*)FRAMReadBuffer)[0];
					//start timeout clock
					//TimeoutCount = TCF0.CNT;
					//reset timer count
					//TCE0.CTRLA = 0x00;
					TCE0.CTRLFSET = 0x08;
					//clear timeout flag
					TimedOut = 0;
					//TCE0.CTRLA = 0x07;
					while(NumReceivedMessages <NumMessages){
						//wait for all messages to come in
						if(pcb->data_rcv){
							length = chb_read((chb_rx_data_t*)(FRAMReadBuffer));
							//pass the data to USB
							SerialWriteBuffer(FRAMReadBuffer,length);
							NumReceivedMessages++;
							//reset timeout count
							//TimeoutCount = TCF0.CNT;
							//reset timer count
							//TCE0.CTRLA = 0x00;
							//send acknowledgment
							TCE0.CTRLFSET = 0x08;
							//clear timeout flag
							TimedOut = 0;
							while(chb_write(dest_addr,&ack,2) != CHB_SUCCESS){
								if(TimedOut) break;	
							}
							TCE0.CTRLFSET = 0x08;
							//clear timeout flag
							TimedOut = 0;
							//TCE0.CTRLA = 0x07;
						}
						//if(TCF0.CNT - TimeoutCount >= timeout) break;
						if(TimedOut) break;		
					}
					//SerialWriteBuffer(FRAMReadBuffer,length);
					//check if timed out
				}	
			}			
			TCE0.CTRLA = 0x00;
			TCE0.CTRLFSET = 0x08;
		}
	}	
}

ISR(TCE0_OVF_vect){
	//reset timer count
	//TCE0.CTRLA = 0x00;
	//TCE0.CTRLFSET = 0x0C;
	//set timeout flag
	TimedOut = 1;
}