/*
 * AccelSampler.c
 *
 * Created: 8/8/2012 12:01:17 PM
 *  Author: Vlad
 */ 

#include "E-000001-000009_firmware_rev_1_0.h"

volatile int numOfSamples;

int main(void) {
	
	TestCard();
	/*
	numOfSamples = 0;
	chb_init();
	TCC1.PER = 64000;	//64000 cycles of cpu at 32MHz equals one 500 Hz interval
	TCC1.INTCTRLA = 0x02;  //enable timer overflow interrupt as medium priority interrupt
	TCC1.CTRLA = 0x01;  //start timer with clock precision of cpu clock (32MHz)
	PMIC.CTRL = ENABLE_ALL_INTERRUPT_LEVELS;
	sei(); //  Enable global interrupts
	//chb_init();
	while(1){
		if(numOfSamples >= 21){ 
			chb_write(0xFFFF,FRAMReadBuffer,numOfSamples*4);
			numOfSamples = 0;
		}			
	}
}	

ISR(TCC1_OVF_vect,ISR_NOBLOCK)
{
	/*
	//sei();
	int junk1,junk2;
	//CO_collectADC(ADC_CH_6_gc, (uint8_t) (FILTER_CH_2AND6_bm | FILTER_HP_0_bm | FILTER_LP_600_gc), FRAMReadBuffer+numOfSamples, junk1, junk2, GAIN_1_gc, SPS_4K_gc);
	//numOfSamples++;
	CO_collectADC(ADC_CH_7_gc, (uint8_t) (FILTER_CH_3AND7_bm | FILTER_HP_0_bm | FILTER_LP_600_gc), FRAMReadBuffer+(numOfSamples*4), junk1, junk2, GAIN_1_gc, SPS_4K_gc);
	numOfSamples++;
	CO_collectADC(ADC_CH_8_gc, (uint8_t) (FILTER_CH_3AND7_bm | FILTER_HP_0_bm | FILTER_LP_600_gc), FRAMReadBuffer+(numOfSamples*4), junk1, junk2, GAIN_1_gc, SPS_4K_gc);
	
	_delay_ms(1000);
	numOfSamples++;
	//sei();
	*/
	while(1){
		nop();
	}
}