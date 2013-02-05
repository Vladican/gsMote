/*
 * AccelSampler.c
 *
 * Created: 8/8/2012 12:01:17 PM
 *  Author: Vlad
 */ 

#include "E-000001-000009_firmware_rev_1_0.h"

volatile int numOfSamples;
//uint8_t Filename[15];

int main(void) {
	
	for (int i=0;i<1556;i++) FRAMReadBuffer[i] = i%121;
	SD_init();
	getBootSectorData();
	writeFile("testing2",FRAMReadBuffer,1556);
	nop();
	while(1){
		nop();
	}
}