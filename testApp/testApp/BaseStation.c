/*
 * BaseStation.c
 *
 * Created: 1/29/2013 2:50:50 PM
 *  Author: Vlad
 */ 
# include "E-000001-000009_firmware_rev_1_0.h"
//#include "dma_driver.h"

volatile uint8_t TimedOut = 0;
// 
// void SetupTransmitChannel( void* MessageBuffer, uint16_t length)
// {
// 	DMA_SetupBlock(
// 	&DMA.CH0,
// 	MessageBuffer,
// 	DMA_CH_SRCRELOAD_BLOCK_gc,
// 	DMA_CH_SRCDIR_INC_gc,
// 	(void *) &USARTC0.DATA,
// 	DMA_CH_DESTRELOAD_BLOCK_gc,
// 	DMA_CH_DESTDIR_FIXED_gc,
// 	length,
// 	DMA_CH_BURSTLEN_1BYTE_gc,
// 	0, // Perform once
// 	FALSE
// 	);
// 	DMA_EnableSingleShot( &DMA.CH0);
// 	// USART Trigger source, Data Register Empty, 0x4C
// 	DMA_SetTriggerSource( &DMA.CH0, DMA_CH_TRIGSRC_USARTC0_DRE_gc);
// }


int main(){

	uint32_t length;
	uint16_t dest_addr;
	uint8_t  MessageBuffer[100];
	uint16_t NumReceivedMessages, NumMessages;
	//set timeout about 2 sec
	uint16_t timeout = 4000;
	//DMA_Enable();
	//enable DMA
// 	DMA.CTRL |= BIT7_bm;
// 	DMA.CH0.CTRLA = BIT2_bm;
// 	DMA.CH0.ADDRCTRL = BIT0_bm;
// 	//trigger on USARTC0 data register empty
// 	DMA.CH0.TRIGSRC = 0x4C;
	//SetupTransmitChannel((void *)FRAMReadBuffer, 100);
	
	
	set_32MHz();
	
	chb_init();
	chb_set_short_addr(0x0000);
	chb_set_channel(1);
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
		dest_addr = (uint16_t)(MessageBuffer[0]);
			
		
		if(length > 0){
			//process/send the bytes over radio
			chb_write(dest_addr,MessageBuffer+1,length-1);
		}
		
		TCE0.CTRLA = 0x07;
		TimedOut = 0;
		//set prescalar of 1024... each timer tick is 512 micro seconds
		//TCE0.CTRLA = 0x07;
		//wait for response/data over radio
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
				//if(pcb->data_rcv && DMA_IsOngoing() == 0){
				if(pcb->data_rcv){
					length = chb_read((chb_rx_data_t*)(FRAMReadBuffer));
					//pass the data to USB
					SerialWriteBuffer(FRAMReadBuffer,length);
					//DMA.CH0.TRFCNT = (uint16_t)length;
					//SetupTransmitChannel((void *)FRAMReadBuffer, (uint16_t)length);
					//DMA_EnableChannel(&DMA.CH0);
					NumReceivedMessages++;
					//reset timeout count
					//TimeoutCount = TCF0.CNT;
					//reset timer count
					//TCE0.CTRLA = 0x00;
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
	nop();
	TimedOut = 1;
	nop();
}