
#include "ADC.h"
#include "adc_driver.h"

volatile uint8_t checksumADC[3] = {0};  // checksum for FRAM test
volatile uint8_t checksumFRAM[3] = {0};  // checksum for FRAM test


void CO_collectTemp(uint16_t *avgV, uint16_t *minV, uint16_t *maxV) {
	
	uint32_t sum = 0;
	uint16_t tempResult;
	uint32_t average;
	uint32_t min = 4096;
	uint32_t max = 0;
	volatile int8_t offset;	

	ADCPower(TRUE);

	/* Move stored calibration values to ADCA. */
	ADC_CalibrationValues_Load(&ADCA);

	/* Set up ADC A to have unsigned conversion mode and 12 bit resolution. */
  	ADC_ConvMode_and_Resolution_Config(&ADCA, ADC_ConvMode_Unsigned, ADC_RESOLUTION_12BIT_gc);

	/* Set sample rate. */
	ADC_Prescaler_Config(&ADCA, ADC_PRESCALER_DIV32_gc);

	/* Set reference voltage on ADCA to be 1.0 V.*/
	ADC_Reference_Config(&ADCA, ADC_REFSEL_INT1V_gc); 

	/* Setup channel 0, 1, 2 and 3 with different inputs. */
	ADC_Ch_InputMode_and_Gain_Config(&ADCA.CH0,
	                                 ADC_CH_INPUTMODE_SINGLEENDED_gc,
	                                 ADC_DRIVER_CH_GAIN_NONE);

	
	/* Get offset value for ADCA. */
   	ADC_Ch_InputMux_Config(&ADCA.CH0, ADC_CH_MUXPOS_PIN1_gc, ADC_CH_MUXNEG_PIN1_gc);

	ADC_Enable(&ADCA);
	/* Wait until common mode voltage is stable. Default clk is 16MHz and
	 * therefore below the maximum frequency to use this function. */
	ADC_Wait_32MHz(&ADCA);
 	offset = ADC_Offset_Get_Unsigned(&ADCA, &ADCA.CH0, false);
    ADC_Disable(&ADCA);
    
	/* Set input to the channels in ADCA */
	ADC_Ch_InputMux_Config(&ADCA.CH0, ADC_CH_MUXPOS_PIN0_gc, 0);
	
	/* Setup sweep. */
	ADC_SweepChannels_Config(&ADCA, ADC_SWEEP_0_gc);

	/* Enable ADC .*/
	ADC_Enable(&ADCA);

	/* Wait until common mode voltage is stable. Default clk is 2MHz and
	 * therefore below the maximum frequency to use this function. */
	ADC_Wait_32MHz(&ADCA);

	/* Enable free running mode. */
	ADC_FreeRunning_Enable(&ADCA);

	/* Sample onboard ADC.*/
	for (uint16_t i = 0; i < NUM_SAMPLES; ++i) {

	  	do{
			/* If the conversion on the ADCA channel 0 never is
			 * complete this will be a deadlock. */
		}while(!ADC_Ch_Conversion_Complete(&ADCA.CH0));
		tempResult = ADC_ResultCh_GetWord_Signed(&ADCA.CH0, offset);
		sum+=tempResult;
		if (max < tempResult) { max = tempResult;}
		if (min > tempResult) { min = tempResult;}

	}

	/* Turn off free running and disable ADC module.*/
	ADC_FreeRunning_Disable(&ADCA);
	ADC_Pipeline_Flush(&ADCA);
	ADC_Disable(&ADCA);

	ADCPower(FALSE);

	average = sum / NUM_SAMPLES;

	//convert to mV
	*avgV = (average * 1000 / 4095) - 50;
	*maxV = (max * 1000 / 4095) - 50;
	*minV = (min * 1000  / 4095) - 50;
}

void CO_collectBatt(uint16_t *avgV, uint16_t *minV, uint16_t *maxV) {
	
	uint32_t sum = 0;
	uint16_t tempResultB;
	uint32_t average;
	uint32_t min = 4096;
	uint32_t max = 0;
	volatile int8_t offset;	

	ADCPower(TRUE);
	PortEx_DIRSET(BIT2_bm, PS_BANKB);
	PortEx_OUTSET(BIT2_bm, PS_BANKB); // activate PIO24 (VBATT)
	_delay_ms(100);
	//setPortEx(BIT2_bm, PS_BANKB);
		
	/* Move stored calibration values to ADCB. */
	ADC_CalibrationValues_Load(&ADCB);

	/* Set up ADC A to have unsigned conversion mode and 12 bit resolution. */
  	ADC_ConvMode_and_Resolution_Config(&ADCB, ADC_ConvMode_Unsigned, ADC_RESOLUTION_12BIT_gc);

	/* Set sample rate. */
	ADC_Prescaler_Config(&ADCB, ADC_PRESCALER_DIV32_gc);

	/* Set reference voltage on ADCB to be 1.0 V.*/
	ADC_Reference_Config(&ADCB, ADC_REFSEL_INT1V_gc); 

	/* Setup channel 0, 1, 2 and 3 with different inputs. */
	ADC_Ch_InputMode_and_Gain_Config(&ADCB.CH0,
	                                 ADC_CH_INPUTMODE_SINGLEENDED_gc,
	                                 ADC_DRIVER_CH_GAIN_NONE);

	
	// Get offset value for ADCB.
   	ADC_Ch_InputMux_Config(&ADCB.CH0, ADC_CH_MUXPOS_PIN1_gc, ADC_CH_MUXNEG_PIN1_gc);

	ADC_Enable(&ADCB);
	// Wait until common mode voltage is stable. Default clk is 16MHz
	ADC_Wait_32MHz(&ADCB);
	offset = ADC_Offset_Get_Unsigned(&ADCB, &ADCB.CH0, false);
    ADC_Disable(&ADCB);
    
	/* Set input to the channels in ADC B */
	ADC_Ch_InputMux_Config(&ADCB.CH0, ADC_CH_MUXPOS_PIN0_gc, 0);
	
	/* Setup sweep. */
	ADC_SweepChannels_Config(&ADCB, ADC_SWEEP_0_gc);

	/* Enable ADC .*/
	ADC_Enable(&ADCB);

	/* Wait until common mode voltage is stable. Default clk is 16MHz and
	 * therefore below the maximum frequency to use this function. */
	ADC_Wait_32MHz(&ADCB);

	/* Enable free running mode. */
	ADC_FreeRunning_Enable(&ADCB);

	/* Store samples.*/
	for (uint16_t i = 0; i < NUM_SAMPLES; ++i) {

	  	do{
			/* If the conversion on the ADCB channel 0 never is
			 * complete this will be a deadlock. */
		}while(!ADC_Ch_Conversion_Complete(&ADCB.CH0));
		tempResultB = ADC_ResultCh_GetWord_Signed(&ADCB.CH0, offset);
		sum+=tempResultB;
		if (max < tempResultB) { max = tempResultB;}
		if (min > tempResultB) { min = tempResultB;}

	}

	/* Turn off free running and disable ADC module.*/
	ADC_FreeRunning_Disable(&ADCB);
	ADC_Disable(&ADCB);

	average = sum / NUM_SAMPLES;
	
	PortEx_DIRCLR(BIT2_bm, PS_BANKB);  // deactivate PIO24 (VBATT)
	//setPortEx(0x00, PS_BANKB);
  	ADCPower(FALSE);

	//convert to mV
	*avgV = (average * 1000 / 4095) - 50;
	*maxV = (max * 1000 / 4095) - 50;
	*minV = (min * 1000  / 4095) - 50;
}

/*! \brief Sets gain on upto 8 on board AD8231 Op Amps via the A0, A1, A2 control lines.
 *	All selected op-amps will be set to the same gain value.
 *
 *	\param channel	channel for gain to be set
 *  \param gainExponent		Sets gain to 2^gainExponent[0:2].
 */
void set_ampGain(uint8_t channel, uint8_t gainExponent) {
	
	// set chip select.  note: AD8231 CS is select on low
	PortEx_OUTCLR((1 << channel), PS_BANKA);
	//setPortEx(~(1 << channel), PS_BANKA);
	// set gain
	// set bit A0
	if(gainExponent & BIT0_bm) {PORTA.OUTSET = PIN6_bm;}
	else {PORTA.OUTCLR = PIN6_bm;}
	// set bit A1
	if(gainExponent & BIT1_bm) {PORTB.OUTSET = PIN1_bm;}
	else {PORTB.OUTCLR = PIN1_bm;}
	// set bit A2
	if(gainExponent & BIT2_bm) {PORTB.OUTSET = PIN2_bm;}
	else {PORTB.OUTCLR = PIN2_bm;}

	_delay_us(1);

	PortEx_OUTSET(0xFF, PS_BANKA);	// write protect all AD8231 amps
	//setPortEx(0xFF, PS_BANKA);

}

void enableADCMUX(uint8_t on) {
	
	if(on) {
		PORTA.DIRSET = PIN5_bm;
		PORTA.OUTSET = PIN5_bm;
	} else {
		PORTA.OUTCLR = PIN5_bm;
		PORTA.DIRCLR = PIN5_bm;
	}
}

void CO_collectADC(uint8_t channel, uint8_t gainExponent, uint16_t SPS, uint16_t numOfSamples, int32_t* DataArray, uint16_t BufferSize, uint8_t use_FRAM) {
	
	CO_collectADC_ext(channel, (uint8_t) (FILTER_CH_1AND5_bm | FILTER_HP_0_bm | FILTER_LP_600_gc), gainExponent, SPS, numOfSamples, DataArray, BufferSize, use_FRAM);
}

/*  \brief Collects avg, min, max of NUM_SAMPLES ADC samples
 *  \param channel	0x00 = channel 1 (VIN1) ELEC1/ELEC2
 * 					0x01 = channel 2 (VIN2) ELEC3/ELEC4
 *					0x02 = channel 3 (VIN3)
 *					0x03 = channel 4 (VIN4)
 *					0x04 = channel 5 (VIN5)
 *					0x05 = channel 6 (VIN6) ACC x-axis
 *					0x06 = channel 7 (VIN7) ACC y-axis
 *					0x07 = channel 8 (VIN8) ACC z-axis
 *	\param filterConfig	bit mask set as follows:
 *	bitwise-or the hardware filter configuration #defines together
 *  using one each channel(CH), high pass(HP), and low pass(LP)
 *  i.e. channel 1&5, highpass 0Hz, lowpass 600Hz => 
 *  FILTER_CH_1AND5_bm | FILTER_HP_0_bm | FILTER_LP_600_gc
 *
 *  DETAILS (recommended to use #defines instead
 *	filterConfig[0]	Channel 1 and 5 mask
 *	filterConfig[1]	Channel 2 and 6 mask
 *	filterConfig[2]	Channel 3 and 7 mask
 *	filterConfig[3]	Channel 4 and 8 mask
 *  filterConfig[4:6] Low Pass cutoff 000=>infinite, 001=>32kHz, 010=>6kHz, 100=>600Hz
 *	filterConfig[7] High Pass cutoff 0=>2Hz, 1=>0Hz 
 *  \param gainExponent		Sets gain to 2^gainExponent[0:2].
 *  \param spsExponent Sets samples per second = 2^spsExponent
*/
void CO_collectADC_ext(uint8_t channel, uint8_t filterConfig, uint8_t gainExponent, uint16_t SPS, uint16_t numOfSamples, int32_t* DataArray, uint16_t BufferSize, uint8_t use_FRAM) {


// 	#ifndef F_CPU
// 	#define F_CPU 32000000UL
// 	#endif
	
	uint16_t period;
	ADC_BUFFER = DataArray;
	ADC_Sampling_Finished = 0;
	ADC_buffer_size = BufferSize;
	if(use_FRAM){
		write_to_FRAM = 1;
	}
	else{
		write_to_FRAM = 0;
	}
	// Turn on power to ADC and PortEx
	ADCPower(TRUE);
	
	// set gain, filters, and ADC input for appropriate VIN
	set_ampGain(channel, gainExponent);
	set_filter(filterConfig);

	// if acc channel then enable DC Pass 
	// it is assumed that if not using high frequency acc specific function then 
	// DC Pass is wanted.
	if ((channel == ADC_CH_6_gc) ||	(channel == ADC_CH_7_gc) || 
		(channel == ADC_CH_8_gc)) ACC_DCPassEnable(TRUE);

	enableADCMUX(TRUE);
	setADCInput(channel);
	SPIInit(SPI_MODE_1_gc);
	SPIC.CTRL = ADC_SPI_CONFIG_gc;
	
	// Configure IO13 (PF0) to capture ADC DRDY signal
	PORTF.DIRCLR = PIN0_bm;
	PORTF.PIN0CTRL = PORT_ISC_FALLING_gc | PORT_OPC_TOTEM_gc;
	PORTF.INT0MASK = PIN0_bm;
	PORTF.INTCTRL = PORT_INT0LVL_MED_gc;				

	// Configure clock for AD7767 MCLK for desired sample frequency
	// f_samples = f_MCLK / 16
	// f_MCLK = f_peripheral(2^25Hz) / (f_period + 1Hz)
	// Set IO14 (PE5) to output
	PORTE.DIRSET = PIN5_bm;
	// Set Waveform generator mode and enable the CCx output to IO14 (PE5)
	TCE1.CTRLB = TC_WGMODE_SS_gc | TC1_CCBEN_bm;
	// set period of waveform generator and ccb as duty cycle (want half the period for duty cycle to have good clock signal)
	//period = (1 << (21 - spsExponent)) - 1;
	period = (F_CPU/16)/SPS;
	TCE1.PER = period;
	TCE1.CCBBUF = period / 2;
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	//reset count to zero
	TCC1.CTRLA = 0x00;
	TCC1.CTRLFSET = 0x0C;
	
	//set the period as number of samples to know when to stop sampling (and compensate for discarded samples at start of sampling)
	TCC1.PER = numOfSamples;
	//Configure IO13(PF0) to drive event channel that triggers event every time a sample is collected
	EVSYS.CH1MUX = EVSYS_CHMUX_PORTF_PIN0_gc;
	//set overflow interrupt to low lvl
	TCC1.INTCTRLA =  TC_OVFINTLVL_LO_gc;
	//set event system to update counter of number of samples every sample event
	//TCC1.CTRLA = ( TCC1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_EVCH1_gc;
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	// Set oscillator source and frequency and start
	TCE1.CTRLA = ( TCE1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_DIV1_gc;
	
	sampleCount = 0;
	discardCount = 0;
		
	// Enable interrupts.
	PMIC.CTRL |= PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm;
	sei();

	
	// wait for ADC to collect samples
	//while(sampleCount < numOfSamples);

	// turn off timer and interrupts
	//TCE1.CTRLA = ( TCE1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
	//PMIC.CTRL &= ~PMIC_LOLVLEN_bm;	
	//cli();

	//SPIDisable();	
	//enableADCMUX(FALSE);
	//ADCPower(FALSE);
}

//triggers when specified number of samples has been collected by ADC
ISR(TCC1_OVF_vect){

	// turn off ADC timer(s)
	TCE1.CTRLA = ( TCE1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
	TCC0.CTRLA = ( TCC0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
	TCD0.CTRLA = ( TCD0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
	TCC1.CTRLA = ( TCE1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;

	//turn off SPI bus and ADC MUX used by ADC
	SPICS(FALSE);
	SPIDisable();
	enableADCMUX(FALSE);
	
	//set a global flag to tell system that all the samples have been collected
	ADC_Sampling_Finished = 1;
	DataAvailable = 1;
}

//continuously take samples and send them via radio. NOT RECOMMENDED
// void CO_collectADC_cont(uint8_t channel, uint8_t filterConfig, uint8_t gainExponent, uint8_t spsExponent) {
// 
// uint16_t period;
// 
// // Turn on power to ADC and PortEx
// ADCPower(TRUE);
// //get data to write files to SD card
// //getBootSectorData();
// // set gain, filters, and ADC input for appropriate VIN
// set_ampGain(channel, gainExponent);
// set_filter(filterConfig);
// 
// // if acc channel then enable DC Pass
// // it is assumed that if not using high frequency acc specific function then
// // DC Pass is wanted.
// if ((channel == ADC_CH_6_gc) ||	(channel == ADC_CH_7_gc) ||
// (channel == ADC_CH_8_gc)) ACC_DCPassEnable(TRUE);
// 
// enableADCMUX(TRUE);
// setADCInput(channel);
// SPIInit(SPI_MODE_1_gc);
// SPIC.CTRL = ADC_SPI_CONFIG_gc;
// 
// // Configure IO13 (PF0) to capture ADC DRDY signal
// PORTF.DIRCLR = PIN0_bm;
// PORTF.PIN0CTRL = PORT_ISC_FALLING_gc | PORT_OPC_TOTEM_gc;
// //PORTF.PIN0CTRL = PORT_ISC_FALLING_gc | PORT_OPC_PULLUP_gc;
// PORTF.INT1MASK = PIN0_bm;
// PORTF.INTCTRL = PORT_INT1LVL_MED_gc;
// 
// // Configure clock for AD7767 MCLK for desired sample frequency
// //f_samples = f_MCLK / 16
// //f_MCLK = f_peripheral(2^25Hz) / (f_period + 1Hz)
// //the TCE1 counter increments every pulse of the system clock and once it reaches a certain value it 
// //toggles pin 5 (clock input to ADC). This effectively divides the system clock into a desired clock rate for 
// //the ADC. 
// // Set IO14 (PE5) to output
// PORTE.DIRSET = PIN5_bm;
// // Set Waveform generator mode(single slope) and enable the CCx output to IO14 (PE5)
// TCE1.CTRLB = TC_WGMODE_SS_gc | TC1_CCBEN_bm;
// // set period
// period = (1 << (21 - spsExponent)) - 1;
// TCE1.PER = period;
// TCE1.CCBBUF = period / 2;
// // Set oscillator source and frequency and start
// TCE1.CTRLA = ( TCE1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_DIV1_gc;
// 
// // Enable interrupts.
// PMIC.CTRL |= PMIC_MEDLVLEN_bm;
// //enable RR of lowlvl interrupts
// PMIC.CTRL |= PMIC_RREN_bm; 
// //perhaps next two lines are redundant...
// // chb_init();
// // chb_set_short_addr(moteID);
// 
// sampleCount = 0;
// TotalSampleCount = 0;
// discardCount = 0;
// sei();
// }

//turns off ADC timers/counters and spi bus 
void ADC_Stop_Sampling(){
	
	// turn off ADC timer(s)
	TCE1.CTRLA = ( TCE1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
	TCC0.CTRLA = ( TCC0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
	TCD0.CTRLA = ( TCD0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
	TCC1.CTRLA = ( TCE1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;

	//turn off SPI bus and ADC MUX used by ADC
	SPICS(FALSE);
	SPIDisable();
	enableADCMUX(FALSE);
	ADC_Sampling_Finished = 1;
	DataAvailable = 1;
}

//returns number of samples collected by last ADC sampling time
uint16_t ADC_Get_Num_Samples(){
	
	if(ADC_Sampling_Finished){
		volatile uint16_t count;
		count = TCC1.CNT;
		if(count == 0) count = TCC1.PER;
		return count;
	}
	else return 0;		
}

void ADC_Pause_Sampling(){
		//ignore interrupts from the ADC...don't turn it off to avoid the boot up time
	PORTF.INT1MASK = 0x00;
}

void ADC_Resume_Sampling(){
	//re-enable interrupt on port F which the ADC uses
	PORTF.INT1MASK = PIN0_bm;
}	

//ISR used by CO_collectADC_cont function
// ISR(PORTF_INT1_vect) {
// 	//freeze mote after x number of samples
// 	/*
// 	if(TotalSampleCount>=10){
// 		SD_disable();
// 		while(1){
// 			nop();
// 		}		
// 	}
// 	*/		
// 	// skip first samples because cannot perform recommended reset
// 	if (discardCount < ADC_DISCARD) {
// 		discardCount++;
// 	}		
// 	else {
// 		// collect data from offchip ADC
// 		SPIInit(SPI_MODE_1_gc);
// 		SPIC.CTRL = ADC_SPI_CONFIG_gc;
// 		SPICS(TRUE); // CS SPI-SS
// 		PORTF.OUTCLR = PIN1_bm; // pull ADC_CS down to enable data read
// 		for(uint8_t bufIndex = 0; bufIndex < 3; bufIndex++) {
// 			SPIC.DATA = 0xAA; // dummy data to start SPI clock
// 			while(!(SPIC.STATUS & SPI_IF_bm));
// 			SPIBuffer[bufIndex] = SPIC.DATA;
// 		}
// 		PORTF.OUTSET = PIN1_bm; // pull ADC_CS up to end data read
// 		SPICS(FALSE);
// 
// 		if(SPIBuffer[0] & BIT7_bm) *(((uint8_t*)&data24Bit[0]) + 3) = 0xFF; // sign extension if negative
// 		else *(((uint8_t*)&data24Bit[0]) + 3) = 0x00;				
// 		*(((uint8_t*)&data24Bit[0]) + 2) = SPIBuffer[0];
// 		*(((uint8_t*)&data24Bit[0]) + 1) = SPIBuffer[1];
// 		*(((uint8_t*)&data24Bit[0]) + 0) = SPIBuffer[2];
// 		
// 		//convert ADC reading to voltage here (in uV)
// 		var = data24Bit[0];
// 		*((int32_t*)&FRAMReadBuffer[sampleCount*4]) = (int32_t) -(var * ADC_VREF / ADC_MAX * ADC_DRIVER_GAIN_DENOMINATOR / ADC_DRIVER_GAIN_NUMERATOR);
// 		
// 		sampleCount++;
// 	//after 128 samples, store the data into sd card and reset sample buffer
// 	//after 30 samples, send the data over the radio
// 	if (sampleCount >= 30) { 
// 		//PORTF.OUTCLR = PIN1_bm;	//set ADC cs to high
// 		//SPIInit(SPI_MODE_0_gc);
// 		//SPICS(TRUE);
// 		//PortEx_DIRCLR(BIT3_bm, PS_BANKB);  //pull SD card CS low
// 		//PortEx_OUTCLR(BIT3_bm, PS_BANKB);
// 		//writeFile("samples");
// 		//writeFile("data",FRAMReadBuffer,512); 
// 		/*
// 		SD_write_block(10,FRAMReadBuffer,512);
// 		for (int i=0;i<512;i++) FRAMReadBuffer[i] = 0;
// 		SD_read_block(10,FRAMReadBuffer);
// 		*/
// 		sampleCount=0;
// 		TotalSampleCount++;
// 		discardCount = ADC_DISCARD -1; //discard the next sample after pausing the sampling to send/store data since the sample ready flag will be outdated and the value might be bad
// 		//PORTF.OUTSET = PIN1_bm; //re-enable ADC for further sampling
// 		
// 		//write code to send the data over radio instead. Include some identifying info (like mote number) with the data.
// 		memmove((void*)FRAMReadBuffer+2,(const void*)FRAMReadBuffer,sampleCount*4); //move the data in the FRAM buffer up by 1 byte to make room for metadata
// 		FRAMReadBuffer[0] = moteID;		//send moteID of the mote that gathered the data
// 		FRAMReadBuffer[1] = (uint8_t)sampleCount;	//send the number of data samples gathered cast as a byte since no more than 30/31 samples should be send at a time
// 		chb_write(0x0000,FRAMReadBuffer,sampleCount*4+2);	//send the samples and the metadata (for now just 1 byte containing moteID) to the base station
// 	}	
// 	}	
// }

//ISR used by CO_collectADC function
ISR(PORTF_INT0_vect) {
	// skip first samples because cannot perform recommended reset
	volatile int32_t currentSample;
	volatile int64_t var;
	if (discardCount < ADC_DISCARD) {
		discardCount++;
		if(discardCount == ADC_DISCARD){
			//set event system to update counter of number of samples every sample event from now on
			TCC1.CTRLA = ( TCC1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_EVCH1_gc;
		}
	} else { 
		// collect data from offchip ADC
		SPICS(TRUE); // CS SPI-SS
		PORTF.OUTCLR = PIN1_bm; // pull ADC_CS down to enable data read
		for(uint8_t bufIndex = 0; bufIndex < 3; bufIndex++) {
			SPIC.DATA = 0xAA; // dummy data to start SPI clock
			while(!(SPIC.STATUS & SPI_IF_bm));
			SPIBuffer[bufIndex] = SPIC.DATA;
		}
		PORTF.OUTSET = PIN1_bm; // pull ADC_CS up to end data read
		SPICS(FALSE);

		// create 32 bits from SPIBuffer[0:2] with sign extension of SPIBuffer[0][7]
		if(SPIBuffer[0] & BIT7_bm) *(((uint8_t*)&currentSample) + 3) = 0xFF; // sign extension if negative
		else *(((uint8_t*)&currentSample) + 3) = 0x00;
		*(((uint8_t*)&currentSample) + 2) = SPIBuffer[0];
		*(((uint8_t*)&currentSample) + 1) = SPIBuffer[1];
		*(((uint8_t*)&currentSample) + 0) = SPIBuffer[2];
		
		//ADC_BUFFER[sampleCount] = (int32_t) -((uint64_t)currentSample * ADC_VREF / ADC_MAX * ADC_DRIVER_GAIN_DENOMINATOR / ADC_DRIVER_GAIN_NUMERATOR);
		var = currentSample;
		ADC_BUFFER[sampleCount%ADC_buffer_size] = (int32_t) -(var * ADC_VREF / ADC_MAX * ADC_DRIVER_GAIN_DENOMINATOR / ADC_DRIVER_GAIN_NUMERATOR);
		if(write_to_FRAM){
			writeFRAM((uint8_t*)(ADC_BUFFER+(sampleCount%ADC_buffer_size)), 4);
		}
		sampleCount++;
	}
}

// Enable DC Pass for acc
// Pull PIO27 high to enable
void ACC_DCPassEnable(uint8_t enable) {
	if (enable) {
		PortEx_DIRSET(BIT5_bm, PS_BANKB);
		PortEx_OUTSET(BIT5_bm, PS_BANKB);
		//setPortEx(BIT5_bm, PS_BANKB);
	} else {
		PortEx_DIRCLR(BIT5_bm, PS_BANKB);
		//setPortEx(0x00, PS_BANKB);
	}
}

/*
void CO_collectSeismic3Channel(uint8_t filterConfig, uint8_t gain[], uint8_t subsamplesPerSecond,
uint8_t subsamplesPerChannel, uint8_t DCPassEnable, uint16_t averagingPtA, uint16_t averagingPtB,
uint16_t averagingPtC, uint16_t averagingPtD) {
	
	// Turn on power to ADC and PortEx
	ADCPower(TRUE);
	
	// Set gains, filters, and input channel
	set_ampGain(ADC_CH_6_gc, gain[0]);
	set_ampGain(ADC_CH_7_gc, gain[1]);
	set_ampGain(ADC_CH_8_gc, gain[2]);
	set_filter(filterConfig);
	ACC_DCPassEnable(DCPassEnable);
	SPIInit(SPI_MODE_1_gc);
	SPIC.CTRL = ADC_SPI_CONFIG_gc;

	enableADCMUX(TRUE);
	setADCInput(ADC_CH_6_gc);
	

	// Configure IO13 (PF0) to capture ADC DRDY signal
	PORTF.PIN0CTRL = PORT_ISC_FALLING_gc;
	PORTF.DIRCLR = PIN0_bm;
	
	// Configure IO13(PF0) to drive event channel
	EVSYS.CH0MUX = EVSYS_CHMUX_PORTF_PIN0_gc;

	// Configure counter for IO13(PF0) events
	TCC0.CTRLB = TC0_CCAEN_bm | TC0_CCBEN_bm | TC0_CCCEN_bm | TC0_CCDEN_bm;
	TCC0.CCA = averagingPtA;
	TCC0.CCB = averagingPtB;
	TCC0.CCC = averagingPtC;
	TCC0.CCD = averagingPtD;
	TCC0.PER = subsamplesPerChannel - 1;
	TCC0.INTCTRLA =  TC_OVFINTLVL_MED_gc;
	TCC0.INTCTRLB = TC_CCAINTLVL_HI_gc | TC_CCBINTLVL_HI_gc | TC_CCCINTLVL_HI_gc | TC_CCDINTLVL_HI_gc;
	TCC0.CTRLA = ( TCC0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_EVCH0_gc;

	FRAMAddress = FR_BASEADD;
	sampleCount = 0;
	SPICount = 0;
	checksumADC[0] = checksumADC[1] = checksumADC[2] = 0;

	// Enable interrupts.
	PMIC.CTRL |= (PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm);
	sei();

	SPICS(TRUE);

	// Configure clock signal for AD7767 MCLK
	PORTE.DIRSET = PIN5_bm;
	// Set Waveform generator mode and enable the CCx output to IO14 (PE5)
	TCE1.CTRLB = TC_WGMODE_SS_gc | TC1_CCBEN_bm;
	// set period
	TCE1.PER = (0x20 << subsamplesPerSecond);
	TCE1.CCBBUF = (0x10 << subsamplesPerSecond);
	// Set oscillator source and frequency and start
	TCE1.CTRLA = ( TCE1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_DIV1_gc;
	
	// wait for ADC to collect samples
	while(sampleCount < FR_TOTAL_NUM_SE_SAMPLES);

	// turn off timer and interrupts
	TCC0.CTRLA = ( TCC0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
	TCE1.CTRLA = ( TCE1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
	PMIC.CTRL &= ~(PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm);
	cli();

	SPICS(FALSE);
	SPIDisable();
	enableADCMUX(FALSE);
	ADCPower(FALSE);
	
}*/
void CO_collectSeismic3Axises(uint8_t gain[], uint16_t subsamplesPerSecond,
uint8_t subsamplesPerChannel, uint8_t DCPassEnable, uint16_t averagingPtA, uint16_t averagingPtB,
uint16_t averagingPtC, uint16_t averagingPtD, uint16_t numOfSamples, int32_t* DataArray, uint16_t BufferSize, uint8_t use_FRAM) {
	
	CO_collectSeismic3Axises_ext((uint8_t) (FILTER_CH_3AND7_bm | FILTER_HP_0_bm | FILTER_LP_600_gc), gain, subsamplesPerSecond,
	subsamplesPerChannel, DCPassEnable, averagingPtA, averagingPtB,
	averagingPtC, averagingPtD, numOfSamples, DataArray, BufferSize, use_FRAM);
}	
void CO_collectSeismic3Axises_ext(uint8_t filterConfig, uint8_t gain[], uint16_t subsamplesPerSecond,
uint8_t subsamplesPerChannel, uint8_t DCPassEnable, uint16_t averagingPtA, uint16_t averagingPtB,
uint16_t averagingPtC, uint16_t averagingPtD, uint16_t numOfSamples, int32_t* DataArray, uint16_t BufferSize, uint8_t use_FRAM) {
	
// 	#ifndef F_CPU
// 	#define F_CPU 32000000UL
// 	#endif
	
	ADC_BUFFER = DataArray;
	ADC_Sampling_Finished = 0;
	ADC_buffer_size = BufferSize;
	if(use_FRAM){
		write_to_FRAM = 1;
	}
	else{
		write_to_FRAM = 0;
	}
	// Turn on power to ADC and PortEx
	ADCPower(TRUE);
	
	// Set gains, filters, and input channel
	set_ampGain(ADC_CH_6_gc, gain[0]);
	set_ampGain(ADC_CH_7_gc, gain[1]);
	set_ampGain(ADC_CH_8_gc, gain[2]);
	set_filter(filterConfig);
	ACC_DCPassEnable(DCPassEnable);
	SPIInit(SPI_MODE_1_gc);
	SPIC.CTRL = ADC_SPI_CONFIG_gc;

	enableADCMUX(TRUE);
	setADCInput(ADC_CH_6_gc);
	

	// Configure IO13 (PF0) to capture ADC DRDY signal
	PORTF.PIN0CTRL = PORT_ISC_FALLING_gc;
	PORTF.DIRCLR = PIN0_bm;
	
	// Configure IO13(PF0) to drive event channel
	EVSYS.CH0MUX = EVSYS_CHMUX_PORTF_PIN0_gc;

	// Configure counter for IO13(PF0) events
	TCC0.CTRLB = TC0_CCAEN_bm | TC0_CCBEN_bm | TC0_CCCEN_bm | TC0_CCDEN_bm;
	TCC0.CCA = averagingPtA;
	TCC0.CCB = averagingPtB;
	TCC0.CCC = averagingPtC;
	TCC0.CCD = averagingPtD;
	TCC0.PER = subsamplesPerChannel - 1;
	TCC0.INTCTRLA =  TC_OVFINTLVL_MED_gc;
	TCC0.INTCTRLB = TC_CCAINTLVL_HI_gc | TC_CCBINTLVL_HI_gc | TC_CCCINTLVL_HI_gc | TC_CCDINTLVL_HI_gc;
	TCC0.CTRLA = ( TCC0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_EVCH0_gc;

	sampleCount = 0;
	SPICount = 0;
	checksumADC[0] = checksumADC[1] = checksumADC[2] = 0;

	// Configure clock signal for AD7767 MCLK
	PORTE.DIRSET = PIN5_bm;
	// Set Waveform generator mode and enable the CCx output to IO14 (PE5)
	TCE1.CTRLB = TC_WGMODE_SS_gc | TC1_CCBEN_bm;
	// set period of waveform generator and duty cycle (want duty cycle to be half the period to get clean clock signal)
	//TCE1.PER = (0x20 << subsamplesPerSecond);
	TCE1.PER = (F_CPU/16)/subsamplesPerSecond;
	//TCE1.CCBBUF = (0x10 << subsamplesPerSecond);
	TCE1.CCBBUF = ((F_CPU/16)/subsamplesPerSecond)/2;
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	//reset count to zero
	TCC1.CTRLA = 0x00;
	TCC1.CTRLFSET = 0x0C;	
	//set the period as number of samples to know when to stop sampling
	TCC1.PER = numOfSamples;
	//Configure IO13(PF0) to drive event channel that triggers event every time the 4 samples are collected and averaged
	EVSYS.CH1MUX = EVSYS_CHMUX_TCC0_OVF_gc;
	//set overflow interrupt to low lvl
	TCC1.INTCTRLA =  TC_OVFINTLVL_LO_gc;
	//set event system to update counter of number of samples every sample event
	TCC1.CTRLA = ( TCC1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_EVCH1_gc;
		
	////////////////////////////////////////////////////////////////////////////////////////////////////////
		
	// Set oscillator source and frequency and start
	TCE1.CTRLA = ( TCE1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_DIV1_gc;
	
	// Enable interrupts.
	PMIC.CTRL |= (PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm);
	sei();
	
	// wait for ADC to collect samples
	//while(sampleCount < numOfSamples);

	// turn off timer and interrupts
// 	TCC0.CTRLA = ( TCC0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
// 	TCE1.CTRLA = ( TCE1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
// 	PMIC.CTRL &= ~(PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm);
// 	cli();
// 
// 	SPICS(FALSE);
// 	SPIDisable();
// 	enableADCMUX(FALSE);
	//ADCPower(FALSE);
}

//first averaging point
ISR(TCC0_CCA_vect) {
	sampleCurrentChannel();
}

//second averaging point
ISR(TCC0_CCB_vect) {
	sampleCurrentChannel();
}

//third averaging point
ISR(TCC0_CCC_vect) {
	sampleCurrentChannel();
}

//final averaging point. Also change ADC channel to sample the next accelerometer axis
ISR(TCC0_CCD_vect) {
	sampleCurrentChannel();
	SPICount = 0;
	if(PORTB.OUT & PIN1_bm) {
		if (PORTA.OUT & PIN6_bm) PORTB.OUTTGL = PIN1_bm;
		else PORTA.OUTTGL = PIN6_bm;
	} else {
		PORTA.OUTCLR = PIN6_bm;
		PORTB.OUTSET = PIN1_bm;
	}
	
}

//consolidate the 4 averaging points
ISR(TCC0_OVF_vect) {
	volatile int64_t sum = 0;
	volatile int32_t currentSample;
		
	for(uint8_t i = 0; i < 12; i+=3) {
		if(SPIBuffer[i] & BIT7_bm) *(((uint8_t*)&currentSample) + 3) = 0xFF; // sign extension if negative
		else *(((uint8_t*)&currentSample) + 3) = 0x00;
		*(((uint8_t*)&currentSample) + 2) = SPIBuffer[i];
		*(((uint8_t*)&currentSample) + 1) = SPIBuffer[i+1];
		*(((uint8_t*)&currentSample) + 0) = SPIBuffer[i+2];
		sum += currentSample;
	}
		
	sum = sum / 4;
	ADC_BUFFER[sampleCount%ADC_buffer_size] = (int32_t)(sum * ADC_VREF / ADC_MAX * ADC_DRIVER_GAIN_DENOMINATOR / ADC_DRIVER_GAIN_NUMERATOR);
	if(write_to_FRAM){
		writeFRAM((uint8_t*)(ADC_BUFFER+(sampleCount%ADC_buffer_size)), 4);
	}
	sampleCount++;

}

void CO_collectSeismic1Channel(uint8_t channel, uint8_t gain, uint16_t subsamplesPerSecond, uint8_t subsamplesPerSample, uint8_t DCPassEnable, uint16_t averagingPtA,
								uint16_t averagingPtB, uint16_t averagingPtC, uint16_t averagingPtD, uint16_t numOfSamples, int32_t* DataArray, uint16_t BufferSize, uint8_t use_FRAM) {
	
	CO_collectSeismic1Channel_ext(channel, (uint8_t) (FILTER_CH_3AND7_bm | FILTER_HP_0_bm | FILTER_LP_600_gc), gain, subsamplesPerSecond, subsamplesPerSample, DCPassEnable, averagingPtA,
	averagingPtB, averagingPtC, averagingPtD, numOfSamples, DataArray, BufferSize, use_FRAM);
}

//collect data from 1 axis of accelerometer
void CO_collectSeismic1Channel_ext(uint8_t channel, uint8_t filterConfig, uint8_t gain, uint16_t subsamplesPerSecond, uint8_t subsamplesPerSample, uint8_t DCPassEnable, uint16_t averagingPtA, 
								uint16_t averagingPtB, uint16_t averagingPtC, uint16_t averagingPtD, uint16_t numOfSamples, int32_t* DataArray, uint16_t BufferSize, uint8_t use_FRAM) {
	
// 	#ifndef F_CPU
// 	#define F_CPU 32000000UL
// 	#endif
	
	ADC_BUFFER=DataArray;
	ADC_Sampling_Finished = 0;
	ADC_buffer_size = BufferSize;
	if(use_FRAM){
		write_to_FRAM = 1;
	}
	else{
		write_to_FRAM = 0;
	}
	// Turn on power to ADC and PortEx
	ADCPower(TRUE);
	
	// Set gains, filters, and input channel
	// set gain, filters, and ADC input for appropriate VIN
	set_ampGain(channel, gain);
	set_filter(filterConfig);
	ACC_DCPassEnable(DCPassEnable);
	SPIInit(SPI_MODE_1_gc);
	SPIC.CTRL = ADC_SPI_CONFIG_gc;

	enableADCMUX(TRUE);
	setADCInput(channel);
	
	// Configure IO13 (PF0) to capture ADC DRDY signal
	PORTF.PIN0CTRL = PORT_ISC_FALLING_gc;
	PORTF.DIRCLR = PIN0_bm;
	
	// Configure IO13(PF0) to drive event channel
	EVSYS.CH0MUX = EVSYS_CHMUX_PORTF_PIN0_gc;

	// Configure counter for IO13(PF0) events
	TCD0.CTRLB = TC0_CCAEN_bm | TC0_CCBEN_bm | TC0_CCCEN_bm | TC0_CCDEN_bm;
	TCD0.CCA = averagingPtA;
	TCD0.CCB = averagingPtB;
	TCD0.CCC = averagingPtC;
	TCD0.CCD = averagingPtD;
	TCD0.PER = subsamplesPerSample - 1;
	TCD0.INTCTRLA =  TC_OVFINTLVL_MED_gc;
	TCD0.INTCTRLB = TC_CCAINTLVL_HI_gc | TC_CCBINTLVL_HI_gc | TC_CCCINTLVL_HI_gc | TC_CCDINTLVL_HI_gc;
	TCD0.CTRLA = ( TCD0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_EVCH0_gc;

	//FRAMAddress = FR_BASEADD;
	sampleCount = 0;
	SPICount = 0;
	//checksumADC[0] = checksumADC[1] = checksumADC[2] = 0;
	

	// Configure clock signal for AD7767 MCLK
	PORTE.DIRSET = PIN5_bm;
	// Set Waveform generator mode and enable the CCx output to IO14 (PE5)
	TCE1.CTRLB = TC_WGMODE_SS_gc | TC1_CCBEN_bm;
	// set period
	//TCE1.PER = (0x20 << subsamplesPerSecond);
	//TCE1.CCBBUF = (0x10 << subsamplesPerSecond);
	TCE1.PER = (F_CPU/16)/subsamplesPerSecond;
	TCE1.CCBBUF = ((F_CPU/16)/subsamplesPerSecond)/2;
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	//reset count to zero
	TCC1.CTRLA = 0x00;
	TCC1.CTRLFSET = 0x0C;	
	//set the period as number of samples to know when to stop sampling
	TCC1.PER = numOfSamples;
	//Configure IO13(PF0) to drive event channel that triggers event every time the 4 samples are collected and averaged
	EVSYS.CH1MUX = EVSYS_CHMUX_TCD0_OVF_gc;
	//set overflow interrupt to low lvl
	TCC1.INTCTRLA =  TC_OVFINTLVL_LO_gc;
	//set event system to update counter of number of samples every sample event
	TCC1.CTRLA = ( TCC1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_EVCH1_gc;
		
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	// Set oscillator source and frequency and start
	TCE1.CTRLA = ( TCE1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_DIV1_gc;
	
	// Enable interrupts.
	PMIC.CTRL |= PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm;
	sei();
	
	// wait for ADC to collect samples
	//while(sampleCount < numOfSamples);

	// turn off timer and interrupts
// 	TCD0.CTRLA = ( TCD0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
// 	TCE1.CTRLA = ( TCE1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
// 	PMIC.CTRL &= ~(PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm);
// 	cli();
// 
// 	SPICS(FALSE);
// 	SPIDisable();
// 	enableADCMUX(FALSE);
	
}

//first averaging point
ISR(TCD0_CCA_vect) {
	sampleCurrentChannel();
}
//second averaging point
ISR(TCD0_CCB_vect) {
	sampleCurrentChannel();
}

//third averaging point
ISR(TCD0_CCC_vect) {
	sampleCurrentChannel();
}

//final averaging point
ISR(TCD0_CCD_vect) {
	sampleCurrentChannel();
	SPICount = 0;
}

//consolidate the 4 averaging points
ISR(TCD0_OVF_vect) {

	volatile int64_t sum = 0;
	volatile int32_t currentSample;
		
	for(uint8_t i = 0; i < 12; i+=3) {
		if(SPIBuffer[i] & BIT7_bm) *(((uint8_t*)&currentSample) + 3) = 0xFF; // sign extension if negative
		else *(((uint8_t*)&currentSample) + 3) = 0x00;
		*(((uint8_t*)&currentSample) + 2) = SPIBuffer[i];
		*(((uint8_t*)&currentSample) + 1) = SPIBuffer[i+1];
		*(((uint8_t*)&currentSample) + 0) = SPIBuffer[i+2];
		sum += currentSample;
	}
		
	sum = sum / 4;
	//get average of the 4 subsamples
	ADC_BUFFER[sampleCount%ADC_buffer_size] = (int32_t)(sum * ADC_VREF / ADC_MAX * ADC_DRIVER_GAIN_DENOMINATOR / ADC_DRIVER_GAIN_NUMERATOR);
	if(write_to_FRAM){
		writeFRAM((uint8_t*)(ADC_BUFFER+(sampleCount%ADC_buffer_size)), 4);
	}
	sampleCount++;
}

//sample an axis of accelerometer with ADC
void sampleCurrentChannel() {
	
	SPICS(TRUE);
	PORTF.OUTCLR = PIN1_bm; // pull ADC_CS down to enable data read
	SPIC.DATA = 0xAA; // dummy data to start SPI clock
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[SPICount] = SPIC.DATA;
	SPIC.DATA = 0xAA; // dummy data to start SPI clock
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[SPICount+1] = SPIC.DATA;
	SPIC.DATA = 0xAA; // dummy data to start SPI clock
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[SPICount+2] = SPIC.DATA;
	PORTF.OUTSET = PIN1_bm; // pull ADC_CS up to end data read
	SPICount +=3;
	SPICS(FALSE);
}

//write collected accelerometer samples to FRAM. OBSOLETE
void writeSE2FRAM() {

	volatile int32_t sum = 0;
	volatile int32_t currentSample;
	sampleCount++;
	
	SPIC.CTRL = FR_SPI_CONFIG_gc;
	
	for(uint8_t i = 0; i < 12; i+=3) {
		if(SPIBuffer[i] & BIT7_bm) *(((uint8_t*)&currentSample) + 3) = 0xFF; // sign extension if negative
		else *(((uint8_t*)&currentSample) + 3) = 0x00;
		*(((uint8_t*)&currentSample) + 2) = SPIBuffer[i];
		*(((uint8_t*)&currentSample) + 1) = SPIBuffer[i+1];
		*(((uint8_t*)&currentSample) + 0) = SPIBuffer[i+2];
		sum += currentSample;
	}
	
	sum = sum / 4;
	SPIBuffer[2] = *(((uint8_t*)&sum)+0);
	SPIBuffer[1] = *(((uint8_t*)&sum)+1);
	SPIBuffer[0] = *(((uint8_t*)&sum)+2);


	
	PORTC.OUTCLR = PIN4_bm;  // enable SPI-SS
	PORTB.OUTCLR = PIN3_bm;  // pull down CS_FRAM to write enable
	nop();
	SPIC.DATA = FR_WREN;
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	PORTB.OUTSET = PIN3_bm;  // latch opcode
	nop(); // time for CS_FRAM to accept high signal
	PORTB.OUTCLR = PIN3_bm;  // pull down CS_FRAM to write enable
	nop();
	SPIC.DATA = FR_WRITE;
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	SPIC.DATA = *(((uint8_t*)&FRAMAddress)+1);
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	SPIC.DATA = *((uint8_t*)&FRAMAddress);
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	SPIC.DATA = SPIBuffer[0];
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	SPIC.DATA = SPIBuffer[1];
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	SPIC.DATA = SPIBuffer[2];
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	
	PORTB.OUTSET = PIN3_bm;  // pull up CS_FRAM to write protect
	PORTC.OUTSET = PIN4_bm;  // disable SPI-SS
	SPIC.CTRL = ADC_SPI_CONFIG_gc;
	PORTC.OUTCLR = PIN4_bm;  // enable SPI-SS
	
	FRAMAddress +=3;
	checksumADC[0] += SPIBuffer[0];
	checksumADC[1] += SPIBuffer[1];
	checksumADC[2] += SPIBuffer[2];
}


//calculate checksum for FRAM. Not used
// void calcChecksumFRAM() {
// 	sumFRAM[0] = sumFRAM[1] = sumFRAM[2] = 0;
// 	checksumFRAM[0] = checksumFRAM[1] = checksumFRAM[2] = 0;
// 	FRAMAddress = FR_BASEADD;
// 	for (uint16_t bufferNum = 0; bufferNum < FR_NUM_READ_BUFFERS; bufferNum++) {
// 		readFRAM(FR_READ_BUFFER_SIZE);
// 		FRAMAddress += FR_READ_BUFFER_SIZE;
// 		for(uint16_t k = 0; k < FR_READ_BUFFER_SIZE; k++) {
// 			checksumFRAM[k%3] += FRAMReadBuffer[k];
// 			
// 			// create 64 bits from 3 sample bytes
// 			if(k%3 == 0) {
// 				if(FRAMReadBuffer[k] & BIT7_bm) *temp64 = 0xFFFFFFFFFF000000; // sign extension if negative
// 				else *temp64 = 0x0000000000000000;
// 				*(((uint8_t*)temp64) + 2) = FRAMReadBuffer[k];
// 			} else if(k%3 == 1) {
// 				*(((uint8_t*)temp64) + 1) = FRAMReadBuffer[k];
// 			} else {
// 				*(((uint8_t*)temp64) + 0) = FRAMReadBuffer[k];
// 			}
// 			
// 			if(k%9 == 2) sumFRAM[0] += *temp64;
// 			if(k%9 == 5) sumFRAM[1] += *temp64;
// 			if(k%9 == 8) sumFRAM[2] += *temp64;
// 
// 			
// 		}
// 		
// 	}
// 	
// }

//test function for FRAM
void FRAMWriteKnowns() {
	FRAMAddress = FR_BASEADD;
	sampleCount = 0;
	checksumADC[0] = checksumADC[1] = checksumADC[2] = 0;
	
	ADCPower(TRUE);
	SPIInit(SPI_MODE_1_gc);
	SPIC.CTRL = FR_SPI_CONFIG_gc;
	SPIBuffer[0] = 0x0D;
	SPIBuffer[1] = 0xF3;
	SPIBuffer[2] = 0x57;
	
	while(sampleCount < FR_TOTAL_NUM_SE_SAMPLES) {
		PORTC.OUTCLR = PIN4_bm;  // enable SPI-SS
		PORTB.OUTCLR = PIN3_bm;  // pull down CS_FRAM to write enable
		nop();
		SPIC.DATA = FR_WREN;
		while(!(SPIC.STATUS & SPI_IF_bm));
		SPIBuffer[12] = SPIC.DATA;
		PORTB.OUTSET = PIN3_bm;  // latch opcode
		nop(); // time for CS_FRAM to accept high signal
		PORTB.OUTCLR = PIN3_bm;  // pull down CS_FRAM to write enable
		nop();
		SPIC.DATA = FR_WRITE;
		while(!(SPIC.STATUS & SPI_IF_bm));
		SPIBuffer[12] = SPIC.DATA;
		SPIC.DATA = *(((uint8_t*)&FRAMAddress)+1);
		while(!(SPIC.STATUS & SPI_IF_bm));
		SPIBuffer[12] = SPIC.DATA;
		SPIC.DATA = *((uint8_t*)&FRAMAddress);
		while(!(SPIC.STATUS & SPI_IF_bm));
		SPIBuffer[12] = SPIC.DATA;
		SPIC.DATA = SPIBuffer[0];
		while(!(SPIC.STATUS & SPI_IF_bm));
		SPIBuffer[12] = SPIC.DATA;
		SPIC.DATA = SPIBuffer[1];
		while(!(SPIC.STATUS & SPI_IF_bm));
		SPIBuffer[12] = SPIC.DATA;
		SPIC.DATA = SPIBuffer[2];
		while(!(SPIC.STATUS & SPI_IF_bm));
		SPIBuffer[12] = SPIC.DATA;
		
		PORTB.OUTSET = PIN3_bm;  // pull up CS_FRAM to write protect
		PORTC.OUTSET = PIN4_bm;  // disable SPI-SS
		
		FRAMAddress +=3;
		checksumADC[0] += SPIBuffer[0];
		checksumADC[1] += SPIBuffer[1];
		checksumADC[2] += SPIBuffer[2];
		
		sampleCount++;
	}
	
	SPIDisable();
	ADCPower(FALSE);
}



/*! \brief Sets input channel to AD7767 via the A0, A1, A2 control lines of ADG758
 */
void setADCInput(uint8_t channel) {
	// set bit A0 to LSB
	if(channel & BIT0_bm) {PORTA.OUTSET = PIN6_bm;}
	else {PORTA.OUTCLR = PIN6_bm;}
	// set bit A1 to second bit
	if(channel & BIT1_bm) {PORTB.OUTSET = PIN1_bm;}
	else {PORTB.OUTCLR = PIN1_bm;}
	// set bit A3 to third bit
	if(channel & BIT2_bm) {PORTB.OUTSET = PIN2_bm;}
	else {PORTB.OUTCLR = PIN2_bm;}

}
