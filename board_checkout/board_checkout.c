/*! 
 *  Hardware checkout procedure (D-000001-000004)
 *  for the SmartGeo Mote E-000001-000009
*/

#include "E-000001-000009_firmware_rev_1_0.h"



int main(void) {
	uint16_t BP_1a_avg_mV, BP_1a_min_mV, BP_1a_max_mV;
	uint16_t BP_1b_avg_mV, BP_1b_min_mV, BP_1b_max_mV;
	uint16_t BP_2_avg_mV, BP_2_min_mV, BP_2_max_mV;
	int32_t BP_3a_avg_uV, BP_3a_min_uV, BP_3a_max_uV;
	int32_t BP_3b_avg_uV, BP_3b_min_uV, BP_3b_max_uV;
	int32_t BP_3c_avg_uV, BP_3c_min_uV, BP_3c_max_uV;
	int32_t BP_3d_avg_uV, BP_3d_min_uV, BP_3d_max_uV;
	int32_t BP_3e_avg_uV, BP_3e_min_uV, BP_3e_max_uV;
	int32_t BP_4a_avg_uV, BP_4a_min_uV, BP_4a_max_uV;
	int32_t BP_4b_avg_uV, BP_4b_min_uV, BP_4b_max_uV;
	int32_t BP_4c_avg_uV, BP_4c_min_uV, BP_4c_max_uV;
	int32_t BP_4d_avg_uV, BP_4d_min_uV, BP_4d_max_uV;
	int32_t BP_4e_avg_uV, BP_4e_min_uV, BP_4e_max_uV;
	int32_t BP_5a_avg_uV, BP_5a_min_uV, BP_5a_max_uV;
	int32_t BP_5b_avg_uV, BP_5b_min_uV, BP_5b_max_uV;
	int32_t BP_6a_avg_uV, BP_6a_min_uV, BP_6a_max_uV;
	int32_t BP_6b_avg_uV, BP_6b_min_uV, BP_6b_max_uV;
	int32_t BP_7a_avg_uV, BP_7a_min_uV, BP_7a_max_uV;
	int32_t BP_7b_avg_uV, BP_7b_min_uV, BP_7b_max_uV;
	int32_t BP_8a_avg_uV, BP_8a_min_uV, BP_8a_max_uV;
	int32_t BP_8b_avg_uV, BP_8b_min_uV, BP_8b_max_uV;
	int32_t BP_9a_avg_uV, BP_9a_min_uV, BP_9a_max_uV;
	int32_t BP_9b_avg_uV, BP_9b_min_uV, BP_9b_max_uV;
	int32_t BP_10a_avg_uV, BP_10a_min_uV, BP_10a_max_uV;
	int32_t BP_10b_avg_uV, BP_10b_min_uV, BP_10b_max_uV;
	int32_t BP_11a_avg_uV, BP_11a_min_uV, BP_11a_max_uV, BP_11a_delta_uV, BP_11a_diff1_uV, BP_11a_diff2_uV;
	int32_t BP_11b_avg_uV, BP_11b_min_uV, BP_11b_max_uV, BP_11b_delta_uV, BP_11b_diff1_uV, BP_11b_diff2_uV;
	int32_t BP_12a_avg_uV, BP_12a_min_uV, BP_12a_max_uV, BP_12a_delta_uV, BP_12a_diff1_uV, BP_12a_diff2_uV;
	int32_t BP_12b_avg_uV, BP_12b_min_uV, BP_12b_max_uV, BP_12b_delta_uV, BP_12b_diff1_uV, BP_12b_diff2_uV;
	int32_t BP_13a_avg_uV, BP_13a_min_uV, BP_13a_max_uV, BP_13a_delta_uV, BP_13a_diff1_uV, BP_13a_diff2_uV;
	int32_t BP_13b_avg_uV, BP_13b_min_uV, BP_13b_max_uV, BP_13b_delta_uV, BP_13b_diff1_uV, BP_13b_diff2_uV;
	

	uint8_t filterSettings;
	
	// set system clock
	set_32MHz();  // for RC clock
	//setXOSC_32MHz();  // for crystal when installed
	// breakpoint 1a - collect room temperature
	CO_collectTemp(&BP_1a_avg_mV, &BP_1a_min_mV, &BP_1a_max_mV);
	// avg 830mV +/- 25% with min/max +/- 1% of avg 
	SD_init();	//initialize SD Card
	// breakpoint 1b - collect body temperature
	CO_collectTemp(&BP_1b_avg_mV, &BP_1b_min_mV, &BP_1b_max_mV);
	// avg should increase from breakpoint 1a
	// min/max +/- 1% of avg

	// breakpoint 2 - collect battery voltage
	CO_collectBatt(&BP_2_avg_mV, &BP_2_min_mV, &BP_2_max_mV);
	// avg 400mV +/- 25% with min/max +/- 1% of avg 

	//**********************************************************************
	//************** SETUP EXTERNAL CIRCUIT BEFORE PROCEEDING **************
	//************************ PAST NEXT BREAKPOINT ************************
	//**********************************************************************
	nop();

	//turn on power supply to create signal voltage for test measurements
	//Ext1Power(TRUE);
	
	//set filter for breakpoint 3	
	filterSettings = (uint8_t) (FILTER_CH_1AND5_bm | FILTER_HP_0_bm | FILTER_LP_600_gc);

	// breakpoint 3a - collect sample from Channel 1 (ELEC1/ELEC2) with gain of 1
	CO_collectADC(ADC_CH_1_gc, filterSettings, &BP_3a_avg_uV, &BP_3a_min_uV, &BP_3a_max_uV,
			GAIN_2_gc, SPS_4K_gc);
	// avg 600mV +/- 10% with min/max +/- 1% of avg 
	
	// breakpoint 3b - collect sample from Channel 1 (ELEC1/ELEC2) with gain of 2
	CO_collectADC(ADC_CH_1_gc, filterSettings, &BP_3b_avg_uV, &BP_3b_min_uV, &BP_3b_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg 300mV +/- 10% with min/max +/- 1% of avg 

	Ext1Power(FALSE);

	//**********************************************************************
	//************** SETUP EXTERNAL CIRCUIT BEFORE PROCEEDING **************
	//************************ PAST NEXT BREAKPOINT ************************
	//**********************************************************************
	nop();

	Ext1Power(TRUE);

	// breakpoint 3c - collect sample from Channel 1 (ELEC1/ELEC2) gain of 1
	CO_collectADC(ADC_CH_1_gc, filterSettings, &BP_3c_avg_uV, &BP_3c_min_uV, &BP_3c_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg 0mV +/- 10% with min/max +/- 1% of avg 

	Ext1Power(FALSE);

	//**********************************************************************
	//************** SETUP EXTERNAL CIRCUIT BEFORE PROCEEDING **************
	//************************ PAST NEXT BREAKPOINT ************************
	//**********************************************************************
	nop();

	Ext1Power(TRUE);

	// breakpoint 3d - collect sample from Channel 1 (ELEC1/ELEC2) polarity reversed with gain of 1
	CO_collectADC(ADC_CH_1_gc, filterSettings, &BP_3d_avg_uV, &BP_3d_min_uV, &BP_3d_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg -300mV +/- 10% with min/max +/- 1% of avg 

	// breakpoint 3e - collect sample from Channel 1 (ELEC1/ELEC2) polarity reversed with gain of 2
	CO_collectADC(ADC_CH_1_gc, filterSettings, &BP_3e_avg_uV, &BP_3e_min_uV, &BP_3e_max_uV,
			GAIN_2_gc, SPS_4K_gc);
	// avg -600mV +/- 10% with min/max +/- 1% of avg 

	Ext1Power(FALSE);

	//**********************************************************************
	//************** SETUP EXTERNAL CIRCUIT BEFORE PROCEEDING **************
	//************************ PAST NEXT BREAKPOINT ************************
	//**********************************************************************
	nop();

	Ext1Power(TRUE);

	// set filter for breakpoint 4
	filterSettings = (uint8_t) (FILTER_CH_2AND6_bm | FILTER_HP_0_bm | FILTER_LP_600_gc);

	// breakpoint 4a - collect sample from Channel 2 (ELEC3/ELEC4) with gain of 2
	CO_collectADC(ADC_CH_2_gc, filterSettings, &BP_4a_avg_uV, &BP_4a_min_uV, &BP_4a_max_uV,
			GAIN_2_gc, SPS_4K_gc);
	// avg 600mV +/- 10% with min/max +/- 1% of avg 

	// breakpoint 4b - collect sample from Channel 2 (ELEC3/ELEC4) with gain of 1
	CO_collectADC(ADC_CH_2_gc, filterSettings, &BP_4b_avg_uV, &BP_4b_min_uV, &BP_4b_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg 300mV +/- 10% with min/max +/- 1% of avg 

	Ext1Power(FALSE);

	//**********************************************************************
	//************** SETUP EXTERNAL CIRCUIT BEFORE PROCEEDING **************
	//************************ PAST NEXT BREAKPOINT ************************
	//**********************************************************************
	nop();

	Ext1Power(TRUE);

	// breakpoint 4c - collect sample from Channel 2 (ELEC3/ELEC4) with gain of 1
	CO_collectADC(ADC_CH_2_gc, filterSettings, &BP_4c_avg_uV, &BP_4c_min_uV, &BP_4c_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg 0mV +/- 10% with min/max +/- 1% of avg

	Ext1Power(FALSE);

	//**********************************************************************
	//************** SETUP EXTERNAL CIRCUIT BEFORE PROCEEDING **************
	//************************ PAST NEXT BREAKPOINT ************************
	//**********************************************************************
	nop();

	Ext1Power(TRUE);

	// breakpoint 4d - collect sample from Channel 2 (ELEC3/ELEC4) polarity reversed with gain of 1
	CO_collectADC(ADC_CH_2_gc, filterSettings, &BP_4d_avg_uV, &BP_4d_min_uV, &BP_4d_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg -300mV +/- 10% with min/max +/- 1% of avg


	// breakpoint 4e - collect sample from Channel 2 (ELEC3/ELEC4) polarity reversed with gain of 2
	CO_collectADC(ADC_CH_2_gc, filterSettings, &BP_4e_avg_uV, &BP_4e_min_uV, &BP_4e_max_uV,
			GAIN_2_gc, SPS_4K_gc);
	// avg -600mV +/- 10% with min/max +/- 1% of avg

	Ext1Power(FALSE);

	//**********************************************************************
	//************** SETUP EXTERNAL CIRCUIT BEFORE PROCEEDING **************
	//************************ PAST NEXT BREAKPOINT ************************
	//**********************************************************************
	nop();

	Ext1Power(TRUE);
	HVPower(TRUE);

	// set filter for breakpoint 5
	filterSettings = (uint8_t) (FILTER_CH_4AND8_bm | FILTER_HP_0_bm | FILTER_LP_600_gc);
	
	// breakpoint 5a - collect sample from Channel 4 with gain of 1
	CO_collectADC(ADC_CH_4_gc, filterSettings, &BP_5a_avg_uV, &BP_5a_min_uV, &BP_5a_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg 293mV +/- 10% with min/max +/- 1% of avg
	
	HVPower(FALSE);
	Ext1Power(FALSE);
	
	
	//**********************************************************************
	//************** SETUP EXTERNAL CIRCUIT BEFORE PROCEEDING **************
	//************************ PAST NEXT BREAKPOINT ************************
	//**********************************************************************
	nop();

	Ext1Power(TRUE);
	HVPower(TRUE);
	
	// breakpoint 5b - collect sample from Channel 4 with gain of 1
	CO_collectADC(ADC_CH_4_gc, filterSettings, &BP_5b_avg_uV, &BP_5b_min_uV, &BP_5b_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg 571mV +/- 10% with min/max +/- 1% of avg

	HVPower(FALSE);
	Ext1Power(FALSE);

	//**********************************************************************
	//************** SETUP EXTERNAL CIRCUIT BEFORE PROCEEDING **************
	//************************ PAST NEXT BREAKPOINT ************************
	//**********************************************************************
	nop();

	Ext1Power(TRUE);
	HVPower(TRUE);
	
	// set filter for breakpoint 6
	filterSettings = (uint8_t) (FILTER_CH_1AND5_bm | FILTER_HP_0_bm | FILTER_LP_600_gc);

	// breakpoint 6a - collect sample from Channel 5 with gain of 1
	CO_collectADC(ADC_CH_5_gc, filterSettings, &BP_6a_avg_uV, &BP_6a_min_uV, &BP_6a_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg ??mV +/- 10% with min/max +/- 1% of avg

	HVPower(FALSE);
	Ext1Power(FALSE);

	//**********************************************************************
	//************** SETUP EXTERNAL CIRCUIT BEFORE PROCEEDING **************
	//************************ PAST NEXT BREAKPOINT ************************
	//**********************************************************************
	nop();

	Ext1Power(TRUE);
	HVPower(TRUE);

	// breakpoint 6b - collect sample from Channel 4 with gain of 1
	CO_collectADC(ADC_CH_5_gc, filterSettings, &BP_6b_avg_uV, &BP_6b_min_uV, &BP_6b_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg ??mV +/- 10% with min/max +/- 1% of avg

	HVPower(FALSE);
	Ext1Power(FALSE);


	//**********************************************************************
	//************** SETUP EXTERNAL CIRCUIT BEFORE PROCEEDING **************
	//************************ PAST NEXT BREAKPOINT ************************
	//**********************************************************************
	nop();

	Ext1Power(TRUE);

	// breakpoint 7a - write known values with checksums to FRAM
	// read known values back from FRAM and recalculate checksums
	FRAMWriteKnownsCheck();
	// checksumADC[0] = checkSumFRAM[0] = 0x37 = 55
	// checksumADC[0] = checkSumFRAM[0] = 0xC9 = 201
	// checksumADC[0] = checkSumFRAM[0] = 0x35 = 53
	// sumFRAM[0] = sumFRAM[1] = sumFRAM[2] = 18CC5ED67 = 6656748903


	// breakpoint 7b - collect sample from all three seismic channels with
	// checksums on FRAM writes and read back recalculating checksums
	FRAMTest3Channel();
	// checksumADC and checkSumFRAM match

	// breakpoint 7c - collect sample from all three seismic channels with
	// checksums on FRAM writes and read back recalculating checksums
	FRAMTest1Channel();
	// checksumADC and checkSumFRAM match 

	Ext1Power(TRUE);

	nop();

	Ext1Power(TRUE);

	// set filter for breakpoint 7
	filterSettings = (uint8_t) (FILTER_CH_1AND5_bm | FILTER_HP_0_bm | FILTER_LP_600_gc);

	// breakpoint 7a - collect sample from Channel 5 with gain of 1
	CO_collectADC(ADC_CH_5_gc, filterSettings, &BP_7a_avg_uV, &BP_7a_min_uV, &BP_7a_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg ??mV +/- 10% with min/max +/- 1% of avg

	// breakpoint 7b - collect sample from Channel 5 with gain of 1
	CO_collectADC(ADC_CH_5_gc, filterSettings, &BP_7b_avg_uV, &BP_7b_min_uV, &BP_7b_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg ??mV +/- 10% with min/max +/- 1% of avg

	Ext1Power(FALSE);

	//**********************************************************************
	//************ TEARDOWN EXTERNAL CIRCUIT BEFORE PROCEEDING *************
	//************************ PAST NEXT BREAKPOINT ************************
	//**********************************************************************
	nop();

	// set filter for breakpoint 8
	filterSettings = (uint8_t) (FILTER_CH_2AND6_bm | FILTER_HP_0_bm | FILTER_LP_600_gc);

	// breakpoint 8a - collect sample from Channel 6 (ACC x-axis) with gain of 1
	// configure board resting on long edge with J1 facing up
	CO_collectADC(ADC_CH_6_gc, filterSettings, &BP_8a_avg_uV, &BP_8a_min_uV, &BP_8a_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg ??mV +/- 10% with min/max +/- 1% of avg

	// breakpoint 8b - collect sample from Channel 6 (ACC x-axis) with gain of 1
	// configure board resting on long edge with J1 facing down
 	CO_collectADC(ADC_CH_6_gc, filterSettings, &BP_8b_avg_uV, &BP_8b_min_uV, &BP_8b_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg ??mV +/- 10% with min/max +/- 1% of avg

	// set filter for breakpoint 9
	filterSettings = (uint8_t) (FILTER_CH_3AND7_bm | FILTER_HP_0_bm | FILTER_LP_600_gc);

	// breakpoint 9a - collect sample from Channel 7 (ACC y-axis) with gain of 1
	// configure board resting vertically short edge with J1 facing up
	CO_collectADC(ADC_CH_7_gc, filterSettings, &BP_9a_avg_uV, &BP_9a_min_uV, &BP_9a_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg ??mV +/- 10% with min/max +/- 1% of avg

	// breakpoint 9b - collect sample from Channel 7 (ACC y-axis) with gain of 1
	// configure board resting vertically short edge with J1 facing down
	CO_collectADC(ADC_CH_7_gc, filterSettings, &BP_9b_avg_uV, &BP_9b_min_uV, &BP_9b_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg ??mV +/- 10% with min/max +/- 1% of avg
	
	// set filter for breakpoint 10
	filterSettings = (uint8_t) (FILTER_CH_3AND7_bm | FILTER_HP_0_bm | FILTER_LP_600_gc);

	// breakpoint 10a - collect sample from Channel 8 (ACC z-axis) with gain of 1
	// configure board resting lies flat with J1 facing up
	CO_collectADC(ADC_CH_8_gc, filterSettings, &BP_10a_avg_uV, &BP_10a_min_uV, &BP_10a_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg ??mV +/- 10% with min/max +/- 1% of avg

	// breakpoint 10b - collect sample from Channel 8 (ACC z-axis) with gain of 1
	// configure board resting lies flat with J1 facing down
	CO_collectADC(ADC_CH_8_gc, filterSettings, &BP_10b_avg_uV, &BP_10b_min_uV, &BP_10b_max_uV,
			GAIN_1_gc, SPS_4K_gc);
	// avg ??mV +/- 10% with min/max +/- 1% of avg
	
	//**********************************************************************
	//************ SETUP SINE WAVE GENERATOR BEFORE PROCEEDING *************
	//************************ PAST NEXT BREAKPOINT ************************
	//**********************************************************************
	nop();

	// set filter for breakpoint 11
	filterSettings = (uint8_t) (FILTER_CH_2AND6_bm | FILTER_HP_0_bm | FILTER_LP_600_gc);

	// breakpoint 11a - collect sample from Channel 6 (ACC x-axis) with gain of 16
	CO_collectADC(ADC_CH_6_gc, filterSettings, &BP_11a_avg_uV, &BP_11a_min_uV, &BP_11a_max_uV,
			GAIN_16_gc, SPS_4K_gc);
	BP_11a_delta_uV = BP_11a_max_uV - BP_11a_min_uV;
	BP_11a_diff1_uV = BP_11a_max_uV - BP_11a_avg_uV;
	BP_11a_diff2_uV = BP_11a_avg_uV - BP_11a_min_uV;
	nop();
	// delta value of ??mV and diff1/diff2 within ??% of each other

	// breakpoint 11b - collect sample from Channel 6 (ACC x-axis) with gain of 16
	CO_collectADC(ADC_CH_6_gc, filterSettings, &BP_11b_avg_uV, &BP_11b_min_uV, &BP_11b_max_uV,
			GAIN_16_gc, SPS_4K_gc);
	BP_11b_delta_uV = BP_11b_max_uV - BP_11b_min_uV;
	BP_11b_diff1_uV = BP_11b_max_uV - BP_11b_avg_uV;
	BP_11b_diff2_uV = BP_11b_avg_uV - BP_11b_min_uV;
	nop();
	// delta value of ??mV and diff1/diff2 within ??% of each other

	// set filter for breakpoint 12
	filterSettings = (uint8_t) (FILTER_CH_3AND7_bm | FILTER_HP_0_bm | FILTER_LP_600_gc);

	// breakpoint 12a - collect sample from Channel 7 (ACC y-axis) with gain of 16
	CO_collectADC(ADC_CH_7_gc, filterSettings, &BP_12a_avg_uV, &BP_12a_min_uV, &BP_12a_max_uV,
			GAIN_16_gc, SPS_4K_gc);
	BP_12a_delta_uV = BP_12a_max_uV - BP_12a_min_uV;
	BP_12a_diff1_uV = BP_12a_max_uV - BP_12a_avg_uV;
	BP_12a_diff2_uV = BP_12a_avg_uV - BP_12a_min_uV;
	nop();
	// delta value of ??mV and diff1/diff2 within ??% of each other

	// breakpoint 12b - collect sample from Channel 7 (ACC y-axis) with gain of 16
	CO_collectADC(ADC_CH_7_gc, filterSettings, &BP_12b_avg_uV, &BP_12b_min_uV, &BP_12b_max_uV,
			GAIN_16_gc, SPS_4K_gc);
	BP_12b_delta_uV = BP_12b_max_uV - BP_12b_min_uV;
	BP_12b_diff1_uV = BP_12b_max_uV - BP_12b_avg_uV;
	BP_12b_diff2_uV = BP_12b_avg_uV - BP_12b_min_uV;
	nop();
	// delta value of ??mV and diff1/diff2 within ??% of each other
	
	// set filter for breakpoint 13
	filterSettings = (uint8_t) (FILTER_CH_3AND7_bm | FILTER_HP_0_bm | FILTER_LP_600_gc);

	// breakpoint 13a - collect sample from Channel 8 (ACC z-axis) with gain of 16
	CO_collectADC(ADC_CH_8_gc, filterSettings, &BP_13a_avg_uV, &BP_13a_min_uV, &BP_13a_max_uV,
			GAIN_16_gc, SPS_4K_gc);
	BP_13a_delta_uV = BP_13a_max_uV - BP_13a_min_uV;
	BP_13a_diff1_uV = BP_13a_max_uV - BP_13a_avg_uV;
	BP_13a_diff2_uV = BP_13a_avg_uV - BP_13a_min_uV;
	nop();
	// delta value of ??mV and diff1/diff2 within ??% of each other

	// breakpoint 13b - collect sample from Channel 8 (ACC z-axis) with gain of 16
	// configure board resting lies flat with J1 facing down
	CO_collectADC(ADC_CH_8_gc, filterSettings, &BP_13b_avg_uV, &BP_13b_min_uV, &BP_13b_max_uV,
			GAIN_16_gc, SPS_4K_gc);
	BP_13b_delta_uV = BP_13b_max_uV - BP_13b_min_uV;
	BP_13b_diff1_uV = BP_13b_max_uV - BP_13b_avg_uV;
	BP_13b_diff2_uV = BP_13b_avg_uV - BP_13b_min_uV;
	nop();
	// delta value of ??mV and diff1/diff2 within ??% of each other

	while(1);
}

