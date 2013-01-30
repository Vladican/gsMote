#include "E-000001-000009_firmware_rev_1_0.h"

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

void ADCPower(uint8_t on) {
	if (on) {
		PORTA.DIRSET = PIN1_bm| PIN2_bm | PIN3_bm | PIN4_bm | PIN6_bm | PIN7_bm; // portEx-CS and HV1/HV2 and A0
		PORTB.DIRSET = PIN1_bm| PIN2_bm | PIN3_bm; // FRAM-CS and A1/A2
		PORTC.DIRSET = PIN0_bm | PIN1_bm;// VDCA and VDC-2 and MUX-SYNC1
		PORTE.DIRSET = PIN4_bm; // MUX-SYNC2
		PORTF.DIRSET = PIN1_bm | PIN2_bm | PIN3_bm; // DAC LDAC and CS

		// high signal to write protect
		PORTA.OUTSET = PIN1_bm| PIN2_bm | PIN3_bm | PIN4_bm | PIN7_bm;; // portEx-CS
		PORTB.OUTSET = PIN3_bm; // FRAM-CS
		PORTC.OUTSET = PIN0_bm | PIN1_bm; // VDCA and VDC-2 on and MUX-SYNC1
		PORTE.OUTSET = PIN4_bm; // MUX-SYNC2
		PORTF.OUTSET = PIN1_bm | PIN2_bm | PIN3_bm;  // ADC-CS and DAC write/latch
		channelStatus = 0x00; // POR to zeros
		_delay_ms(100);

		// set SPI-MISO as input
		PORTC.DIRCLR = PIN6_bm;
		
		bankA_DIR = bankA_OUT = bankB_DIR = bankB_OUT = 0x00; // all pins input on reset
		PortEx_DIRSET(0xFF, PS_BANKA);
		PortEx_OUTSET(0xFF, PS_BANKA);  //write protect IN-AMP 1 thru 8
		//setPortEx(0xFF, PS_BANKA);
		set_filter(0xFF);  // set filters initially to ensure data out pulled high

	} else {
		// low signal for low power
		PORTA.OUTCLR = PIN1_bm| PIN2_bm | PIN3_bm | PIN4_bm | PIN6_bm | PIN7_bm; // portEx-CS and A0
		PORTB.OUTCLR = PIN1_bm | PIN2_bm | PIN3_bm; // FRAM-CS and A1/A2
		PORTC.OUTCLR = PIN0_bm | PIN1_bm; // VDCA and VDC-2 on and MUX-SYNC1
		PORTE.OUTCLR = PIN4_bm; // MUX-SYNC2
		PORTF.OUTCLR = PIN1_bm | PIN2_bm | PIN3_bm;  // ADC-CS and DAC write/latch


		PORTA.DIRCLR = PIN1_bm| PIN2_bm | PIN3_bm | PIN4_bm | PIN7_bm | PIN6_bm;
		PORTB.DIRCLR = PIN1_bm | PIN2_bm| PIN3_bm;
		PORTC.DIRCLR = PIN0_bm | PIN1_bm;
		PORTE.DIRCLR = PIN4_bm;
		PORTF.DIRCLR = PIN1_bm | PIN2_bm | PIN3_bm;
		
		// set SPI-MISO as input
		PORTC.DIRCLR = PIN6_bm;
		
		
		bankA_DIR = bankA_OUT = bankB_DIR = bankB_OUT = 0x00; // all pins input on reset
		channelStatus = 0x00;
		
	}
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
/*  \brief Sets input analog filters
 *	\param filterConfig	bit mask set as follows:
 *	bitwise or the hardware filter config #defines together
 *  using one each channel(CH), high pass(HP), and low pass(LP)
 *  i.e. channel 1/5, highpass 0Hz, lowpass 600Hz => 
 *  FILTER_CH_1AND5_bm | FILTER_HP_0_bm | FILTER_LP_600_gc
 *
 *  DETAILS
 *	filterConfig[0]	Channel 1 and 5 mask
 *	filterConfig[1]	Channel 2 and 6 mask
 *	filterConfig[2]	Channel 3 and 7 mask
 *	filterConfig[3]	Channel 4 and 8 mask
 *  filterConfig[4:6] Low Pass cutoff 000=>infinite, 001=>32kHz, 010=>6kHz, 100=>600Hz
 *	filterConfig[7] High Pass cutoff 0=>2Hz, 1=>0Hz 
*/
void set_filter(uint8_t filterConfig) {
	// hack to get ADC to work
	//filterConfig |= 0x0F;

	// boolean flags for upper/lower channels CS
	uint8_t lowerCS = filterConfig & 0x03; 
	uint8_t upperCS = filterConfig & 0x0C;

	// update left and right channel status
	if (filterConfig & (BIT0_bm | BIT2_bm)) channelStatus = 
		(0xF0 & channelStatus) | (filterConfig >> 4);  //right
	if (filterConfig & (BIT1_bm | BIT3_bm)) channelStatus =
		(0xF0 & filterConfig) | (0x0F & channelStatus); //left
		
	SPIInit(SPI_MODE_1_gc);

	
	SPIBuffer[0] = channelStatus;
	
	// enable appropriate chip select
	if (lowerCS) lowerMuxCS(TRUE);
	if (upperCS) upperMuxCS(TRUE);

	SPICS(TRUE);

	// Send all logic high to ensure that the SDO line on the chip is
	// left in high Z state after SPI transaction.
	// The t-1 SDI transaction is output on the SDO and the pin left in the configuration.
	// of the last bit
	SPIC.DATA = 0xFF;
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;

	nop();

	SPIC.DATA = SPIBuffer[0];
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	SPICS(FALSE);

	if (lowerCS) lowerMuxCS(FALSE);
	if (upperCS) upperMuxCS(FALSE);
	SPIDisable();
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
void CO_collectADC(uint8_t channel, uint8_t filterConfig, int32_t *avgV, int32_t *minV, int32_t *maxV, uint8_t gainExponent, uint8_t spsExponent) {

	int64_t sum = 0;
	int64_t average;
	int64_t min = ADC_MAX;
	int64_t max = -ADC_MAX;
	uint16_t period;
	
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
	PORTF.INTCTRL = PORT_INT0LVL_LO_gc;				

	// Configure clock for AD7767 MCLK for desired sample frequency
	// f_samples = f_MCLK / 16
	// f_MCLK = f_peripheral(2^25Hz) / (f_period + 1Hz)
	// Set IO14 (PE5) to output
	PORTE.DIRSET = PIN5_bm;
	// Set Waveform generator mode and enable the CCx output to IO14 (PE5)
	TCE1.CTRLB = TC_WGMODE_SS_gc | TC1_CCBEN_bm;
	// set period
	period = (1 << (21 - spsExponent)) - 1;
	TCE1.PER = period;
	TCE1.CCBBUF = period / 2;
	// Set oscillator source and frequency and start
	TCE1.CTRLA = ( TCE1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_DIV1_gc;
	
	// Enable interrupts.
	PMIC.CTRL |= PMIC_LOLVLEN_bm;
	sei();

	sampleCount = 0;
	discardCount = 0;
	
	// wait for ADC to collect samples
	while(sampleCount < NUM_SAMPLES);

	// turn off timer and interupts
	TCE1.CTRLA = ( TCE1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
	PMIC.CTRL &= ~PMIC_LOLVLEN_bm;	
	cli();

	SPIDisable();	
	enableADCMUX(FALSE);
	ADCPower(FALSE);

	// collect average, max and min of SP samples
	for (sampleCount = 0; sampleCount < NUM_SAMPLES; sampleCount++) {
		sum += data24Bit[sampleCount];
		if (max < data24Bit[sampleCount]) { max = data24Bit[sampleCount];}
		if (min > data24Bit[sampleCount]) { min = data24Bit[sampleCount];}
	}
	average = sum / NUM_SAMPLES;

	//convert to uV
	*avgV = (int32_t) -(average * ADC_VREF / ADC_MAX * ADC_DRIVER_GAIN_DENOMINATOR / ADC_DRIVER_GAIN_NUMERATOR);
	*maxV = (int32_t) -(max * ADC_VREF / ADC_MAX * ADC_DRIVER_GAIN_DENOMINATOR / ADC_DRIVER_GAIN_NUMERATOR);
	*minV = (int32_t) -(min * ADC_VREF / ADC_MAX * ADC_DRIVER_GAIN_DENOMINATOR / ADC_DRIVER_GAIN_NUMERATOR);

}

void CO_collectADC_cont(uint8_t channel, uint8_t filterConfig, uint8_t gainExponent, uint8_t spsExponent) {

uint16_t period;

// Turn on power to ADC and PortEx
ADCPower(TRUE);
//get data to write files to SD card
getBootSectorData();
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
//PORTF.PIN0CTRL = PORT_ISC_FALLING_gc | PORT_OPC_PULLUP_gc;
PORTF.INT1MASK = PIN0_bm;
PORTF.INTCTRL = PORT_INT1LVL_LO_gc;

// Configure clock for AD7767 MCLK for desired sample frequency
//f_samples = f_MCLK / 16
//f_MCLK = f_peripheral(2^25Hz) / (f_period + 1Hz)
//the TCE1 counter increments every pulse of the system clock and once it reaches a certain value it 
//toggles pin 5 (clock input to ADC). This effectively divides the system clock into a desired clock rate for 
//the ADC. 
// Set IO14 (PE5) to output
PORTE.DIRSET = PIN5_bm;
// Set Waveform generator mode(single slope) and enable the CCx output to IO14 (PE5)
TCE1.CTRLB = TC_WGMODE_SS_gc | TC1_CCBEN_bm;
// set period
period = (1 << (21 - spsExponent)) - 1;
TCE1.PER = period;
TCE1.CCBBUF = period / 2;
// Set oscillator source and frequency and start
TCE1.CTRLA = ( TCE1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_DIV1_gc;

// Enable interrupts.
PMIC.CTRL |= PMIC_LOLVLEN_bm;
sei();

sampleCount = 0;
TotalSampleCount = 0;
discardCount = 0;
}


void ADC_Pause_Sampling(){
		//ignore interrupts from the ADC...don't turn it off to avoid the boot up time
	PORTF.INT1MASK = 0x00;
}

void ADC_Resume_Sampling(){
	//re-enable interrupt on port F which the ADC uses
	PORTF.INT1MASK = PIN0_bm;
}	
ISR(PORTF_INT1_vect) {
	// skip first samples because cannot perform recommended reset
	if(TotalSampleCount>=10){
		SD_disable();
		while(1){
			nop();
		}		
	}		
	if (discardCount < ADC_DISCARD) {
		discardCount++;
	}		
	else {
		// collect data from offchip ADC
		SPIInit(SPI_MODE_1_gc);
		SPIC.CTRL = ADC_SPI_CONFIG_gc;
		SPICS(TRUE); // CS SPI-SS
		PORTF.OUTCLR = PIN1_bm; // pull ADC_CS down to enable data read
		for(uint8_t bufIndex = 0; bufIndex < 3; bufIndex++) {
			SPIC.DATA = 0xAA; // dummy data to start SPI clock
			while(!(SPIC.STATUS & SPI_IF_bm));
			SPIBuffer[bufIndex] = SPIC.DATA;
		}
		PORTF.OUTSET = PIN1_bm; // pull ADC_CS up to end data read
		SPICS(FALSE);

		if(SPIBuffer[0] & BIT7_bm) *(((uint8_t*)&data24Bit[0]) + 3) = 0xFF; // sign extension if negative
		else *(((uint8_t*)&data24Bit[0]) + 3) = 0x00;				
		*(((uint8_t*)&data24Bit[0]) + 2) = SPIBuffer[0];
		*(((uint8_t*)&data24Bit[0]) + 1) = SPIBuffer[1];
		*(((uint8_t*)&data24Bit[0]) + 0) = SPIBuffer[2];
		
		//convert ADC reading to voltage here (in uV)
		var = data24Bit[0];
		*((int32_t*)&FRAMReadBuffer[sampleCount*4]) = (int32_t) -(var * ADC_VREF / ADC_MAX * ADC_DRIVER_GAIN_DENOMINATOR / ADC_DRIVER_GAIN_NUMERATOR);
		
		sampleCount++;
	//after 128 samples, store the data into sd card and reset sample buffer
	if (sampleCount >= 128) { 
		//PORTF.OUTCLR = PIN1_bm;	//set ADC cs to high
		//SPIInit(SPI_MODE_0_gc);
		//SPICS(TRUE);
		//PortEx_DIRCLR(BIT3_bm, PS_BANKB);  //pull SD card CS low
		//PortEx_OUTCLR(BIT3_bm, PS_BANKB);
		//writeFile("samples");
		writeFile("data",0); 
		/*
		SD_write_block(10,FRAMReadBuffer,512);
		for (int i=0;i<512;i++) FRAMReadBuffer[i] = 0;
		SD_read_block(10,FRAMReadBuffer);
		*/
		sampleCount=0;
		TotalSampleCount++;
		//PORTF.OUTSET = PIN1_bm; //re-enable ADC for further sampling
	}	
	}	
}

ISR(PORTF_INT0_vect) {
	// skip first samples because cannot perform recommended reset
	if (discardCount < ADC_DISCARD) {
		discardCount++;
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
		if(SPIBuffer[0] & BIT7_bm) *(((uint8_t*)&data24Bit[sampleCount]) + 3) = 0xFF; // sign extension if negative
		else *(((uint8_t*)&data24Bit[sampleCount]) + 3) = 0x00;
	
		*(((uint8_t*)&data24Bit[sampleCount]) + 2) = SPIBuffer[0];
		*(((uint8_t*)&data24Bit[sampleCount]) + 1) = SPIBuffer[1];
		*(((uint8_t*)&data24Bit[sampleCount]) + 0) = SPIBuffer[2];

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
	
}

void CO_collectSeismic3Channel_continuous(uint8_t filterConfig, uint8_t gain[], uint8_t subsamplesPerSecond,
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
	
	// collect samples from ADC continuously
	while(1){
		nop();
	}
}


ISR(TCC0_CCA_vect) {
	sampleCurrentChannel();
}

ISR(TCC0_CCB_vect) {
	sampleCurrentChannel();
}

ISR(TCC0_CCC_vect) {
	sampleCurrentChannel();
}

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

ISR(TCC0_OVF_vect) {
	writeSE2FRAM();
}

void CO_collectSeismic1Channel(uint8_t channel, uint8_t filterConfig, uint8_t gain, uint8_t subsamplesPerSecond, uint8_t subsamplesPerSample, uint8_t DCPassEnable, uint16_t averagingPtA, uint16_t averagingPtB, uint16_t averagingPtC, uint16_t averagingPtD) {
	
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

	FRAMAddress = FR_BASEADD;
	sampleCount = 0;
	SPICount = 0;
	checksumADC[0] = checksumADC[1] = checksumADC[2] = 0;
	
	// Enable interrupts.
	PMIC.CTRL |= PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm;
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
	TCD0.CTRLA = ( TCD0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
	TCE1.CTRLA = ( TCE1.CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
	PMIC.CTRL &= ~(PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm);
	cli();

	SPICS(FALSE);
	SPIDisable();
	enableADCMUX(FALSE);
	ADCPower(FALSE);
	
}

ISR(TCD0_CCA_vect) {
	sampleCurrentChannel();
}

ISR(TCD0_CCB_vect) {
	sampleCurrentChannel();
}

ISR(TCD0_CCC_vect) {
	sampleCurrentChannel();
}

ISR(TCD0_CCD_vect) {
	sampleCurrentChannel();
	SPICount = 0;
}

ISR(TCD0_OVF_vect) {
	writeSE2FRAM();
}

void sampleCurrentChannel() {
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
}

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


void calcChecksumFRAM() {
	sumFRAM[0] = sumFRAM[1] = sumFRAM[2] = 0;
	checksumFRAM[0] = checksumFRAM[1] = checksumFRAM[2] = 0;
	FRAMAddress = FR_BASEADD;
	for (uint16_t bufferNum = 0; bufferNum < FR_NUM_READ_BUFFERS; bufferNum++) {
		readFRAM(FR_READ_BUFFER_SIZE);
		FRAMAddress += FR_READ_BUFFER_SIZE;
		for(uint16_t k = 0; k < FR_READ_BUFFER_SIZE; k++) {
			checksumFRAM[k%3] += FRAMReadBuffer[k];
			
			// create 64 bits from 3 sample bytes
			if(k%3 == 0) {
				if(FRAMReadBuffer[k] & BIT7_bm) *temp64 = 0xFFFFFFFFFF000000; // sign extension if negative
				else *temp64 = 0x0000000000000000;
				*(((uint8_t*)temp64) + 2) = FRAMReadBuffer[k];
			} else if(k%3 == 1) {
				*(((uint8_t*)temp64) + 1) = FRAMReadBuffer[k];
			} else {
				*(((uint8_t*)temp64) + 0) = FRAMReadBuffer[k];
			}
			
			if(k%9 == 2) sumFRAM[0] += *temp64;
			if(k%9 == 5) sumFRAM[1] += *temp64;
			if(k%9 == 8) sumFRAM[2] += *temp64;

			
		}
		
	}
	
}


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
