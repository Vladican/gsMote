/*
 * CFile1.c
 *
 * Created: 8/15/2012 11:33:22 AM
 *  Author: Vlad
 */

#include "E-000001-000009_firmware_rev_1_0.h"


int main(){
	set_32MHz();
	synch(1);
	CO_collectADC_cont(ADC_CH_1_gc, (uint8_t) (FILTER_CH_1AND5_bm | FILTER_HP_0_bm | FILTER_LP_600_gc),	GAIN_1_gc, SPS_1K_gc);
	while(1);
}
