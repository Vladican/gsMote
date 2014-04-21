/*
 * Node.c
 *
 * Created: 9/22/2013 7:28:35 PM
 *  Author: Vlad
 */ 

#include "E-000001-000009_firmware_rev_1_0.h"

volatile uint8_t TimedOut = 0;

int main(){
	
	uint8_t length;
	uint8_t gain = GAIN_1_gc;
	uint16_t ack = 0;
	volatile uint8_t RawGain;
	uint16_t freq = 2000;
	volatile uint32_t samples = 0;
	DataAvailable = 0;
	ADC_Sampling_Finished = 1;
	uint8_t RadioMessageBuffer[20];
	unsigned char ofile[] = {'o','u','t','p','u','t'};
	set_32MHz();
	if(wpan_init() != MAC_SUCCESS){
		//initialization failed	
	}
	wpan_task();
	//chb_init();
	//chb_set_channel(1);
	//chb_set_short_addr(0x0001);
	//pcb_t* pcb = chb_get_pcb();
	//SD_init();
	//getBootSectorData();
	
	//setup timeout timer
	//approx 2 seconds to wait (using largest prescaler of 1024)
	TCE0.PER = 4000;
	TCE0.CTRLFSET = 0x08;
	TCE0.INTCTRLA = TC_OVFINTLVL_LO_gc;
	PMIC.CTRL |= PMIC_LOLVLEN_bm;
	sei();

	while(1){
		if(1 /*pcb->data_rcv*/){
			//read the data
			//length = chb_read((chb_rx_data_t*)RadioMessageBuffer);
			//length should be >1 for setting gain/freq commands: the value is likely sent in a separate message
				switch ( RadioMessageBuffer[0])
				{
				case 'R':
					//collect data if the ADC is not collecting any data right now
					if(ADC_Sampling_Finished){
						//CO_collectADC(ADC_CH_1_gc, gain, freq, 10000, (int32_t*)FRAMReadBuffer, FR_READ_BUFFER_SIZE/4, TRUE);
						CO_collectSeismic1Channel(ADC_CH_8_gc, gain, freq, 6, FALSE, 1, 2, 3, 4, 10000,(int32_t*)FRAMReadBuffer, FR_READ_BUFFER_SIZE/4, TRUE);
					}
					//send acknowledgment
					//chb_write(0x0000,(uint8_t*)(&ack),2);						
					break;
				case 'G':
					//while(!pcb->data_rcv);
					//length = chb_read((chb_rx_data_t*)RadioMessageBuffer);
					//set gain to what is specified
					RawGain = (uint8_t)(*(int32_t*)(RadioMessageBuffer+1));
					switch(RawGain){
						case 1:
							gain = GAIN_1_gc;
							break;
						case 2:
							gain = GAIN_2_gc;
							break;
						case 4:
							gain = GAIN_4_gc;
							break;
						case 8:
							gain = GAIN_8_gc;
							break;
						case 16:
							gain = GAIN_16_gc;
							break;
						case 32:
							gain = GAIN_32_gc;
							break;
						case 64:
							gain = GAIN_64_gc;
							break;
						case 128:
							gain = GAIN_128_gc;
							break;
						default:
							//chb_write(0x0000,(uint8_t*)"invalid gain",strlen("invalid gain"));
							break;
					}
					//send acknowledgment
					//chb_write(0x0000,(uint8_t*)(&ack),2);					
					break;
				case 'F':

					//while(!pcb->data_rcv);
					//length = chb_read((chb_rx_data_t*)RadioMessageBuffer);
					//set sampling frequency to what is specified
					freq = (uint16_t)(*(int32_t*)(RadioMessageBuffer+1));
					//send acknowledgment
					//chb_write(0x0000,(uint8_t*)(&ack),2);
					break;
				case 'S':
					//stop the ADC if it is not already
					if(!ADC_Sampling_Finished){
						ADC_Stop_Sampling();
					}
					//otherwise, the ADC has finished sampling on its own and the data is ready to be transmitted
					//send acknowledgment
					//chb_write(0x0000,(uint8_t*)(&ack),2);
					break;
				case 'T':
					if(ADC_Sampling_Finished && DataAvailable){
						//get number of data points collected
						samples = ADC_Get_Num_Samples();
						if(samples > 0){	
							//uint16_t NumMessages = ((samples*4)/CHB_MAX_PAYLOAD);
							//if ((samples*4)%CHB_MAX_PAYLOAD > 0) NumMessages++;
							//start timeout timer
							TCE0.CTRLA = 0x07;
							TimedOut = 0;
							//send the number of messages the base station should expect after this message
							//while(chb_write(0x0000,(uint8_t*)(&NumMessages),2) != CHB_SUCCESS){
								if(TimedOut) break;
							//}
							if(TimedOut){
								//stop timeout counter and go back to waiting for command
								TimedOut = 0;
								TCE0.CTRLA = 0;
								break;
							}
							//reset timeout timer
							TimedOut = 0;
							TCE0.CTRLFSET = 0x08;  
							//read the data from FRAM and send it
							for(uint16_t i =0; i<(samples*4);){	
								if(samples*4 - i >= 100){
									readFRAM(100,(FRAMAddress-(samples*4))+i);						
									//while(chb_write(0x0000,FRAMReadBuffer,100) != CHB_SUCCESS);
									i += 100;
								}
								else{
									readFRAM(samples*4 - i,(FRAMAddress-(samples*4))+i);
									//while(chb_write(0x0000,FRAMReadBuffer,samples*4 - i) != CHB_SUCCESS);
									i += samples*4 - i;
								}
								//reset timeout timer
								TimedOut = 0;
								TCE0.CTRLFSET = 0x08;
								//while(!pcb->data_rcv){
									//break if timed out waiting for response
									if(TimedOut) break;
								//}
								if(TimedOut) break;		
								//length = chb_read((chb_rx_data_t*)RadioMessageBuffer);							
							}
							//chb_write(0x0000,FRAMReadBuffer,samples*4);								
							//write the data to SD card for good measure (make sure transmitted and collected data is the same)	
							//writeFile(ofile, FRAMReadBuffer, samples*4);			
						}
						if(TimedOut){
							//stop timeout counter and go back to waiting for command
							TimedOut = 0;
							TCE0.CTRLA = 0; 
							break;
						}														
						DataAvailable = 0;
					}
					else {
						//while(chb_write(0x0000,(uint8_t*)(&ack),2) != CHB_SUCCESS);
					}
					break;
				}		
		}		
	}	
}

ISR(TCE0_OVF_vect){
	TimedOut = 1;
}