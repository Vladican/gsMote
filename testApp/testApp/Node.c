/*
 * Node.c
 *
 * Created: 9/22/2013 7:28:35 PM
 *  Author: Vlad
 */ 
#include "E-000001-000009_firmware_rev_1_0.h"

int main(){

	chb_init();
	chb_set_channel(1);
	chb_set_short_addr(0x0002);
	while(1){
		//collect data
		CO_collectADC(ADC_CH_1_gc, (uint8_t) (FILTER_CH_3AND7_bm | FILTER_HP_0_bm | FILTER_LP_600_gc), GAIN_1_gc, 1000, 32,(int32_t*)FRAMReadBuffer);
		//send data to base station
		chb_write(0x0000,FRAMReadBuffer,128);
	}	
}