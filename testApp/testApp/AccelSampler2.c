/*
 * CFile1.c
 *
 * Created: 8/15/2012 11:33:22 AM
 *  Author: Vlad
 */

#include "E-000001-000009_firmware_rev_1_0.h"

volatile int numOfSamples;

int main(){
	//chb_init();
	set_32MHz();
	SD_init();
	//CO_collectADC_cont(ADC_CH_6_gc,(uint8_t) (FILTER_CH_2AND6_bm | FILTER_HP_0_bm | FILTER_LP_600_gc),GAIN_1_gc,SPS_512_gc);
	//CO_collectADC(ADC_CH_8_gc, (uint8_t) (FILTER_CH_3AND7_bm | FILTER_HP_0_bm | FILTER_LP_600_gc), FRAMReadBuffer, FRAMReadBuffer+4, FRAMReadBuffer+4, GAIN_16_gc, SPS_4K_gc);
	//function to sample from electrodes 1 and 2
	int32_t avg = 0;
	int32_t max = 0;
	int32_t min = 0;
	//CO_collectADC(ADC_CH_1_gc,(uint8_t) (FILTER_CH_1AND5_bm | FILTER_HP_0_bm | FILTER_LP_600_gc),&avg,&min,&max,GAIN_1_gc,SPS_1K_gc);
	CO_collectADC_cont(ADC_CH_1_gc, (uint8_t) (FILTER_CH_1AND5_bm | FILTER_HP_0_bm | FILTER_LP_600_gc),	GAIN_1_gc, SPS_1K_gc);
	//CO_collectSeismic1Channel(ADC_CH_1_gc,(uint8_t) (FILTER_CH_1AND5_bm | FILTER_HP_0_bm | FILTER_LP_600_gc),GAIN_1_gc,SSPS_SE_1K_gc,10,FALSE,2,4,6,8);
	while(1);
}
