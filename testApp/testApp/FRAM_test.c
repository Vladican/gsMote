/*
 * FRAM_test.c
 *
 * Created: 3/8/2014 8:19:11 PM
 *  Author: VLAD
 */ 
#include "E-000001-000009_firmware_rev_1_0.h"

int main(){
	
	for(int i=0; i<200; i++){
		FRAMReadBuffer[i] = i;
	}
	for(int i=0; i<200; i++){
		FRAMReadBuffer[200+i] = i*2;
	}
	for(int i=0; i<200; i++){
		FRAMReadBuffer[400+i] = i*3;
	}
	for(int i=0; i<200; i++){
		FRAMReadBuffer[600+i] = i*4;
	}
	for(int i=0; i<200; i++){
		FRAMReadBuffer[800+i] = i*5;
	}
	writeFRAM(FRAMReadBuffer, 1000);
	for(int i=0; i<1000; i++){
		FRAMReadBuffer[i] = 0;
	}
	nop();
	readFRAM(1000, 0);
	nop();
	readFRAM(1000, 5);
	nop();
}