/*
 * Node.c
 *
 * Created: 9/22/2013 7:28:35 PM
 *  Author: Vlad
 */ 
#include "E-000001-000009_firmware_rev_1_0.h"

int main(){
	
	uint8_t length;
	uint8_t gain = GAIN_1_gc;
	uint16_t freq = 5000;
	uint32_t samples = 0;
	DataAvailable = 0;
	ADC_Sampling_Finished = 1;
	uint8_t RadioMessageBuffer[20];
	set_32MHz();
	chb_init();
	chb_set_channel(1);
	chb_set_short_addr(0x0002);
	pcb_t* pcb = chb_get_pcb();
	while(1){
		if(pcb->data_rcv){
			//read the data
			length = chb_read((chb_rx_data_t*)RadioMessageBuffer);
			
			if(length == 1){
				switch ( RadioMessageBuffer[0])
				{
				case 'R':
					//collect data if the ADC is not collecting any data right now
					if(ADC_Sampling_Finished){
						//CO_collectADC(ADC_CH_1_gc, gain, freq, FR_READ_BUFFER_SIZE/4,(int32_t*)FRAMReadBuffer);
						CO_collectSeismic1Channel(ADC_CH_8_gc, gain, freq, 5, TRUE, 1, 2, 3, 4, FR_READ_BUFFER_SIZE/4,(int32_t*)FRAMReadBuffer);
					}						
					break;
				case 'G':
					//set gain to what is specified
					switch(RadioMessageBuffer[1]){
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
							chb_write(0x0000,(uint8_t*)"invalid gain",strlen("invalid gain"));
							break;
					}					
					break;
				case 'F':
					//set sampling frequency to what is specified
					freq = *(uint16_t*)(RadioMessageBuffer+1);
					break;
				case 'S':
					//stop the ADC if it is not already
					if(!ADC_Sampling_Finished){
						ADC_Stop_Sampling();
					}
					//otherwise, the ADC has finished sampling on its own and the data will be transmitted after this switch statement
					break;
				case 'T':
					if(ADC_Sampling_Finished && DataAvailable){
						//get number of data points collected
						samples = ADC_Get_Num_Samples();
						if(samples > 0) chb_write(0x0000,FRAMReadBuffer,samples*4);
						DataAvailable = 0;
					}
					break;
				}	
			}			
		}		
		//if all data collected or ADC was stopped while sampling, send collected data to base station
		//samples = ADC_Get_Num_Samples();
// 		if(ADC_Sampling_Finished && DataAvailable){
// 			//get number of data points collected
// 			samples = ADC_Get_Num_Samples();
// 			if(samples > 0) chb_write(0x0000,FRAMReadBuffer,samples*4);
// 			DataAvailable = 0;
// 		}	
	}	
}