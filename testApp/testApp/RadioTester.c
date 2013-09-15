/*
 * RadioTester.c
 *
 * Created: 9/14/2013 10:39:56 AM
 *  Author: Vlad
 */ 
#include "E-000001-000009_firmware_rev_1_0.h"

int main(){
	
	chb_init();
	uint8_t data[10] = {1,2,3,4,5,6,7,8,9,10};
	chb_set_short_addr(0x0002);
	chb_set_channel(0);
	while(1){
		chb_write(0xFFFF,data,10);
		delay_us(100000);
		chb_read(FRAMReadBuffer);
	}	
}