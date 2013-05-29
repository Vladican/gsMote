/*
 * CFile1.c
 *
 * Created: 8/15/2012 11:33:22 AM
 *  Author: Vlad
 */

#include "E-000001-000009_firmware_rev_1_0.h"



int main(){
	volatile int32_t SampledData[130];
	char SampledDataInChars[1700];
	/*
	char buff[8];
	moteID = 1;
	RadioMonitorMode = SYNCHED;		//initialize the RadioMonitorMode to synched
// 	unsigned char message[8];
// 	strcpy(message,"reset");
// 	itoa((int)(moteID),buff,10);
// 	strcat(message,buff);
		
	set_32MHz();	//set the clock frequency
	synch(1);		//synch sampling at 1 sec periods
	CO_collectADC_cont(ADC_CH_1_gc, (uint8_t) (FILTER_CH_1AND5_bm | FILTER_HP_0_bm | FILTER_LP_600_gc),	GAIN_1_gc, SPS_1K_gc);	//collect and send samples from ADC at 1kHz
	while(1){
		//if(RadioMonitorMode == TIME_SYNCH) chb_write(0x0000,message,strlen(message));
		nop();
	}
	*/
	SD_init();
	getBootSectorData();
	set_32MHz();
	while(1){
		CO_collectADC(ADC_CH_1_gc,(uint8_t) (FILTER_CH_1AND5_bm | FILTER_HP_0_bm | FILTER_LP_600_gc),GAIN_1_gc,SPS_1K_gc,128,SampledData);
		DeciToString(SampledData,128,SampledDataInChars);
		writeFile("samples",SampledDataInChars,1700);	
	}		
}
