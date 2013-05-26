/*
 * AccelSampler.c
 *
 * Created: 8/8/2012 12:01:17 PM
 *  Author: Vlad
 */ 

#include "E-000001-000009_firmware_rev_1_0.h"

volatile int numOfSamples;
int32_t testArray[150];

int main(void) {
	
	int UnityGain[] = {0,0,0};
	
// 	for (int i=0;i<1556;i++) FRAMReadBuffer[i] = i%121;
// 	SD_init();
// 	getBootSectorData();
// 	writeFile("testing2",FRAMReadBuffer,1556);
// 	nop();
// 	while(1){
// 		nop();
// 	}
	//CO_collectADC(ADC_CH_8_gc, (uint8_t) (FILTER_CH_3AND7_bm | FILTER_HP_0_bm | FILTER_LP_600_gc), GAIN_16_gc, SPS_4K_gc, 100, testArray);
	//CO_collectSeismic1Channel(ADC_CH_8_gc,(uint8_t) (FILTER_CH_3AND7_bm | FILTER_HP_0_bm | FILTER_LP_600_gc),GAIN_1_gc, SSPS_SE_4K_gc, 5, TRUE, 1, 2, 3, 4,100,testArray);
	CO_collectSeismic3Channel_2((uint8_t) (FILTER_CH_3AND7_bm | FILTER_CH_2AND6_bm | FILTER_HP_0_bm | FILTER_LP_600_gc),UnityGain, SSPS_SE_4K_gc, 5, TRUE, 1, 2, 3, 4,100,testArray);
}