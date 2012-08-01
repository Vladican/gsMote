#include "E-000001-000009_firmware_rev_1_0.h"

// Global Variables
volatile uint8_t channelStatus;  // copy of channel filter configuration to allow bit level changes
volatile uint16_t sampleCount;  // sample and discard counter for array offset
volatile int32_t data24Bit[NUM_SAMPLES];  // storage for ADC samples
volatile uint8_t SPIBuffer[13];  // space for 24-bit ADC conversion plus dummy byte
volatile uint16_t FRAMAddress;  // address counters for FRAM write/read
//volatile uint8_t FRAMReadBuffer[FR_READ_BUFFER_SIZE]; // storage for reading FRAM
volatile uint8_t newFile[15] = {'n','e','w','F','I','L','E','.',' ',' ',' ',' ',' ',' ',' '}; //must be no more than 8 letters before the extension "." 
volatile uint8_t newFileR[15] = {'n','e','w','F','I','L','E','.',' ',' ',' ',' ',' ',' ',' '};
volatile uint8_t SPICount, discardCount;
volatile int32_t *temp32;  // for parsing SPI transactions
volatile int64_t *temp64; // for parsing SPI transactions from 8bit pieces to 64bit whole
volatile uint8_t checksumADC[3] = {0};  // checksum for FRAM test
volatile uint8_t checksumFRAM[3] = {0};  // checksum for FRAM test
volatile int64_t sumFRAM[3];  // sum of all FRAM samples for averaging
volatile uint8_t bankA_DIR, bankA_OUT, bankB_DIR, bankB_OUT;  // status of port expander current configurations to allow bit level changes
volatile uint8_t Buffer[13];
//volatile uint8_t seq;
//volatile uint8_t readTestData[512];
//uint8_t TestData[8] = {0xAA,0xBB,0xCC,0xDD,0xEE,0x11,0x22,0x44};
// Sets the external 16MHz crystal on XTAL1 and XTAL2 as the system clock.
// There was a problem with some of the hardware modules not having the crystal
// so this function will fail until the oscillator is installed.
//  - LS900-SI-02 does not contain the crystal
//  - SFLX02-A03 does contain the crystal
// The high frequency crystal is required to clock the ADC without jitter
void setXOSC_32MHz() {
	// configure the crystal to match the chip 
	CLKSYS_XOSC_Config( OSC_FRQRANGE_12TO16_gc,
		                    false,
		                    OSC_XOSCSEL_XTAL_16KCLK_gc );
	CLKSYS_Enable(OSC_XOSCEN_bm);
	// wait for signal to stabilize
	do {} while (CLKSYS_IsReady( OSC_XOSCRDY_bm ) == 0 );
	// configure PLL to use the crystal and turn on
	CLKSYS_PLL_Config( OSC_PLLSRC_XOSC_gc, 2);
	CLKSYS_Enable( OSC_PLLEN_bm );
	// wait for signal to stabilize
	do {} while (CLKSYS_IsReady( OSC_PLLRDY_bm ) == 0 );
	// set as system clock and disable unused RC oscillator
	CLKSYS_Main_ClockSource_Select( CLK_SCLKSEL_PLL_gc );
	CLKSYS_Disable( OSC_RC2MEN_bm );
}

// produces consistent but inaccurate clock period.
void set_16MHz() {
	// select 32MHz Oscillator and prescale by 1/2
	CLKSYS_Enable(OSC_RC32MEN_bm);
	CLKSYS_Prescalers_Config( CLK_PSADIV_1_gc, CLK_PSBCDIV_1_2_gc );
	// wait for signal to stabilize
	do {} while (CLKSYS_IsReady(OSC_RC32MRDY_bm) == 0);
	
	CLKSYS_Main_ClockSource_Select( CLK_SCLKSEL_RC32M_gc );
	CLKSYS_Disable( OSC_RC2MEN_bm );
	
}


// produces consistent but inaccurate clock period.
void set_32MHz() {
	// select 32MHz Oscillator and prescale by 1
	CLKSYS_Enable(OSC_RC32MEN_bm);
	CLKSYS_Prescalers_Config( CLK_PSADIV_1_gc, CLK_PSBCDIV_1_1_gc );
	// wait for signal to stabilize
	do {} while (CLKSYS_IsReady(OSC_RC32MRDY_bm) == 0);
	
	CLKSYS_Main_ClockSource_Select( CLK_SCLKSEL_RC32M_gc );
	CLKSYS_Disable( OSC_RC2MEN_bm );
	
}

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



void portExCS(uint8_t write) {
	if (write) PORTA.OUTCLR = PIN3_bm;
	else {
		PORTA.OUTSET = PIN3_bm;
	}
	_delay_us(10);
}

// 1 in bitmap sets the selected pins to output
// Port Expander must be powered on (VDC-2)
// all other pins are unaffected
void PortEx_DIRSET(uint8_t pins, uint8_t bank) {
	SPIInit(PS_SPI_MODE);
	SPICS(TRUE);
	portExCS(TRUE);

	if(bank) bankA_DIR = (uint8_t) (pins | bankA_DIR);
	else bankB_DIR = (uint8_t) (pins | bankB_DIR);
	
	SPIBuffer[0] = PS_WRITE;
	if(bank) {
		SPIBuffer[1]=PS_IODIRA;
		SPIBuffer[2] = ~bankA_DIR; 
	} else {
		SPIBuffer[1]=PS_IODIRB;
		SPIBuffer[2] = ~bankB_DIR;
	}	

	for(uint8_t bufIndex = 0; bufIndex < 3; bufIndex++) {
		SPIC.DATA = SPIBuffer[bufIndex];
		while(!(SPIC.STATUS & SPI_IF_bm));	//wait for the byte to be sent
		SPIBuffer[12] = SPIC.DATA;			//read the data register to clear status flag
	}

	portExCS(FALSE);
	SPICS(FALSE);
	SPIDisable();


}

// 1 in bitmap sets the selected pins to input
// Port Expander must be powered on (VDC-2)
// all other pins are unaffected
void PortEx_DIRCLR(uint8_t pins, uint8_t bank) {
	SPIInit(PS_SPI_MODE);
	SPICS(TRUE);
	portExCS(TRUE);
	
	if(bank) bankA_DIR = (uint8_t) (pins & ~bankA_DIR);
	else bankB_DIR = (uint8_t) (pins & ~bankB_DIR);
	
	SPIBuffer[0] = PS_WRITE;
	if(bank) {
		SPIBuffer[1]=PS_IODIRA;
		SPIBuffer[2] = ~bankA_DIR; 
	} else {
		SPIBuffer[1]=PS_IODIRB;
		SPIBuffer[2] = ~bankB_DIR;
	}	

	for(uint8_t bufIndex = 0; bufIndex < 3; bufIndex++) {
		SPIC.DATA = SPIBuffer[bufIndex];
		while(!(SPIC.STATUS & SPI_IF_bm));
		SPIBuffer[12] = SPIC.DATA;
	}

	SPICS(FALSE);
	portExCS(FALSE);
	SPIDisable();
}

void PortEx_OUTSET(uint8_t pins, uint8_t bank) {
	SPIInit(PS_SPI_MODE);
	SPICS(TRUE);
	portExCS(TRUE);
	
	if(bank) bankA_OUT = (uint8_t) (pins | bankA_OUT);
	else bankB_OUT = (uint8_t) (pins | bankB_OUT);
	
	SPIBuffer[0] = PS_WRITE;
	if(bank) {
		SPIBuffer[1]=PS_OLATA;
		SPIBuffer[2] = bankA_OUT;
	} else {
		SPIBuffer[1]=PS_OLATB;
		SPIBuffer[2] = bankB_OUT; 
	}

	for(uint8_t bufIndex = 0; bufIndex < 3; bufIndex++) {
		SPIC.DATA = SPIBuffer[bufIndex];
		while(!(SPIC.STATUS & SPI_IF_bm));
		SPIBuffer[12] = SPIC.DATA;
	}

	SPICS(FALSE);
	portExCS(FALSE);
	SPIDisable();
}


void PortEx_OUTCLR(uint8_t pins, uint8_t bank) {
	SPIInit(PS_SPI_MODE);
	SPICS(TRUE);
	portExCS(TRUE);
	
	if(bank) bankA_OUT = (uint8_t) (bankA_OUT & ~pins);
	else bankB_OUT = (uint8_t) (bankB_OUT & ~pins);
	
	SPIBuffer[0] = PS_WRITE;
	if(bank) {
		SPIBuffer[1]=PS_OLATA;
		SPIBuffer[2] = bankA_OUT;
	} else {
		SPIBuffer[1]=PS_OLATB;
		SPIBuffer[2] = bankB_OUT; 
	}

	for(uint8_t bufIndex = 0; bufIndex < 3; bufIndex++) {
		SPIC.DATA = SPIBuffer[bufIndex];
		while(!(SPIC.STATUS & SPI_IF_bm));
		SPIBuffer[12] = SPIC.DATA;
	}

	SPICS(FALSE);
	portExCS(FALSE);
	SPIDisable();
}

/* replace with functions above for greater flexibility
// turns on bits of A or B bank according to mask and bank indicator
// Port Expander must be powered on (VDC-2)
void setPortEx(uint8_t portMask, uint8_t bank) {
			
	SPIInit(PS_SPI_MODE);
	
	SPIBuffer[0] = PS_WRITE;
	if(bank) SPIBuffer[1]=PS_IODIRA;
	else SPIBuffer[1]=PS_IODIRB;
	SPIBuffer[2] = (uint8_t) ~portMask; 
	nop();
	
	SPICS(TRUE);
	portExCS(TRUE);

	for(uint8_t bufIndex = 0; bufIndex < 3; bufIndex++) {
		SPIC.DATA = SPIBuffer[bufIndex];
		while(!(SPIC.STATUS & SPI_IF_bm));
		SPIBuffer[12] = SPIC.DATA;
	}

	SPICS(FALSE);
	portExCS(FALSE);
	
	SPIBuffer[0] = PS_WRITE;
	if(bank) SPIBuffer[1]=PS_OLATA;
	else SPIBuffer[1]=PS_OLATB;
	SPIBuffer[2] = portMask; 
	nop();

	SPICS(TRUE);
	portExCS(TRUE);

	for(uint8_t bufIndex = 0; bufIndex < 3; bufIndex++) {
		SPIC.DATA = SPIBuffer[bufIndex];
		while(!(SPIC.STATUS & SPI_IF_bm));
		SPIBuffer[12] = SPIC.DATA;
	}

	portExCS(FALSE);	
	SPIDisable();
}
*/

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

void Ext1Power(uint8_t on) {
	if (on) {
		PORTF.DIRSET = PIN5_bm;
		PORTF.OUTSET = PIN5_bm;
		//PortEx_DIRSET(PIN3_bm, PS_BANKB);
		//PortEx_OUTSET(PIN3_bm, PS_BANKB);  //write protect SDHC
		_delay_ms(100);
		
	} else {
		PORTF.OUTCLR = PIN5_bm;
		PORTF.DIRCLR = PIN5_bm;
		//PortEx_OUTCLR(PIN3_bm, PS_BANKB);  //no need to write protect SDHC
	}
}

void Ext2Power(uint8_t on) {
	if (on) {
		PORTF.DIRSET = PIN6_bm;
		PORTF.OUTSET = PIN6_bm;
		_delay_ms(100);
	} else {
		PORTF.OUTCLR = PIN6_bm;
		PORTF.DIRCLR = PIN6_bm;
	}
}

void HVPower(uint8_t on) {
	if (on) {
		PORTF.DIRSET = PIN7_bm;
		PORTF.OUTSET = PIN7_bm;
		_delay_ms(100);
	} else {
		PORTF.OUTCLR = PIN7_bm;
		PORTF.DIRCLR = PIN7_bm;
	}
	_delay_us(1000);
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

void lowerMuxCS(uint8_t write) {
	// take IO15(PE4) low to enable write
	if (write) PORTE.OUTCLR = PIN4_bm;
	else PORTE.OUTSET = PIN4_bm;
}
void upperMuxCS(uint8_t write) {
	// take IO16(PC
	if (write) PORTC.OUTCLR = PIN1_bm;
	else PORTC.OUTSET = PIN1_bm;
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


void SPIInit(uint8_t mode) {
	// init SPI SS pin
	PORTC.DIRSET = PIN4_bm;
	PORTC.PIN4CTRL = PORT_OPC_WIREDANDPULL_gc;
	PORTC.OUTSET = PIN4_bm;

	// init SPI
	SPIC.CTRL =	SPI_PRESCALER |  // set clock speed
	            0x00 |  // disable clock double
	            SPI_ENABLE_bm | // Enable SPI module
	            0x00 |  // set data order msb first
	            SPI_MASTER_bm | // set SPI master
	            mode; // set SPI mode

	// disable SPI Interrupts
	SPIC.INTCTRL = SPI_INTLVL_OFF_gc;

 	// set SPI-MOSI and SPI-SCK as output
	PORTC.DIRSET  = PIN5_bm | PIN7_bm;

	
}
//use this if using a prescalar
void SPIInit2(uint8_t mode, uint8_t prescalar) {
	// init SPI SS pin
	PORTC.DIRSET = PIN4_bm;
	PORTC.PIN4CTRL = PORT_OPC_WIREDANDPULL_gc;
	PORTC.OUTSET = PIN4_bm;
	

	// init SPI
	SPIC.CTRL =	prescalar |  // set clock speed
	0x00 |  // disable clock double
	SPI_ENABLE_bm | // Enable SPI module
	0x00 |  // set data order msb first
	SPI_MASTER_bm | // set SPI master
	mode; // set SPI mode

	// disable SPI Interrupts
	SPIC.INTCTRL = SPI_INTLVL_OFF_gc;

	// set SPI-MOSI and SPI-SCK as output
	PORTC.DIRSET  = PIN5_bm | PIN7_bm;
}

void SPICS(uint8_t enable) {
	if (enable) PORTC.OUTCLR = PIN4_bm;
	else {
		PORTC.OUTSET = PIN4_bm;
	}	
}

void SPIDisable() {
	PORTC.OUTSET = PIN4_bm;
	SPIC.CTRL = 0x00;
	PORTC.OUTCLR = PIN4_bm;
	PORTC.DIRCLR = PIN4_bm | PIN5_bm | PIN7_bm;

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
void CO_collectADC(uint8_t channel, uint8_t filterConfig, int32_t *avgV,
		int32_t *minV, int32_t *maxV, uint8_t gainExponent, uint8_t spsExponent) {

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

void CO_collectSeismic1Channel(uint8_t channel, uint8_t filterConfig, uint8_t gain,
	uint8_t subsamplesPerSecond, uint8_t subsamplesPerSample, uint8_t DCPassEnable,
	uint16_t averagingPtA, uint16_t averagingPtB, uint16_t averagingPtC,
	uint16_t averagingPtD) {
				
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

// Read from FRAM
// FRAM power (VDC-2) must be on with CS_FRAM pulled high to write protect
void readFRAM (uint16_t numBytes) {
	SPIInit(SPI_MODE_0_gc);
	SPIC.CTRL = FR_SPI_CONFIG_gc;
	SPICS(TRUE);
	PORTB.OUTCLR = PIN3_bm;  // pull down CS_FRAM to write enable
	nop();
								
	SPIC.DATA = FR_READ;
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	SPIC.DATA = *(((uint8_t*)&FRAMAddress) + 1);;
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	SPIC.DATA = *(((uint8_t*)&FRAMAddress) + 0);;
	while(!(SPIC.STATUS & SPI_IF_bm));
	SPIBuffer[12] = SPIC.DATA;
	
	for(uint16_t i = 0; i < numBytes; i++) { 
		SPIC.DATA = 0xAA;
		while(!(SPIC.STATUS & SPI_IF_bm));
		FRAMReadBuffer[i] = SPIC.DATA;
	}

	PORTB.OUTSET = PIN3_bm;  // CS_FRAM write protect
	SPICS(FALSE);
	SPIDisable();

}

void FRAMTest3Channel(void) {
	uint8_t gains[3] = { GAIN_1_gc, GAIN_1_gc, GAIN_1_gc };
			
	CO_collectSeismic3Channel(FILTER_CH_2AND6_bm | FILTER_CH_3AND7_bm |
		FILTER_CH_4AND8_bm | FILTER_HP_0_bm | FILTER_LP_600_gc,
		gains, SSPS_SE_64K_gc, 21, TRUE, 13, 14, 15, 16);
	ADCPower(TRUE);
	_delay_us(250);
	
	calcChecksumFRAM();

	ADCPower(FALSE);
	
}

void FRAMTest1Channel(void) {

	CO_collectSeismic1Channel(ADC_CH_8_gc, FILTER_CH_4AND8_bm | FILTER_HP_0_bm | FILTER_LP_600_gc,
	GAIN_1_gc, SSPS_SE_64K_gc, 21, TRUE, 13, 14, 15, 16);
	ADCPower(TRUE);
	_delay_us(250);
	
	calcChecksumFRAM();

	ADCPower(FALSE);

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


void FRAMWriteKnownsCheck() {
	FRAMWriteKnowns();
	ADCPower(TRUE);

	_delay_us(250);
	calcChecksumFRAM();

	ADCPower(FALSE);

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

void SDHC_init(void);
void SDHC_send_command(uint8_t command, uint32_t arg);
void SDHC_read_sector(void);
void SDHC_write_sector(void);
uint8_t SDHC_get_response(void);
uint16_t SDHC_CRC16(uint8_t *data, uint16_t bytes);
void SDHC_read_block(uint8_t *buffer, uint16_t address, uint16_t numBlocks);
void SDHC_write_block(uint8_t *buffer, uint16_t address, uint16_t numBlocks);
void SDHC_read_register(uint8_t *buffer, uint8_t cmd);

void SDHC_CS(uint8_t enable) {
		
}



//the following command writes/reads a byte via spi
uint8_t SPI_write(uint8_t byteToSend){
	uint8_t data;
	SPIC.DATA = byteToSend;
	while(!(SPIC.STATUS & SPI_IF_bm)); //wait for byte to be sent
	data = SPIC.DATA; //read SPI data register to reset status flag
	return data;
}
//the following command writes a command to the sd card and returns a response (if any) or 0xFF if no response
uint8_t SD_command(uint8_t cmd, uint32_t arg, uint8_t crc, int read) {
	
	SPI_write(SDHC_COMMAND_START | cmd);
	SPI_write(arg>>24 & LSBYTE_MASK);
	SPI_write(arg>>16 & LSBYTE_MASK);
	SPI_write(arg>>8 & LSBYTE_MASK);
	SPI_write(arg & LSBYTE_MASK);
	SPI_write(crc);
	
	for(int i=0; i<read; i++){
		Buffer[i%13] = SPI_write(SDHC_DUMMY_BYTE);
		if (Buffer[i%13] != SDHC_DUMMY_BYTE){
			 Buffer[1] = Buffer[i%13];
			 return Buffer[1];
		}			
	}
	return SDHC_DUMMY_BYTE;	
}
//the following command writes one sector to the sdhc card
void SD_write_block(uint32_t sector,uint8_t* data, int lengthOfData){
	SPIInit(SPI_MODE_0_gc);
	SPICS(TRUE);
	int fillerBytes = SDHC_SECTOR_SIZE - lengthOfData;
	if (fillerBytes==SDHC_SECTOR_SIZE) fillerBytes = 0;
	for(int i=0;SD_command(SDHC_CMD_WRITE_SINGLE_BLOCK,sector,SDHC_DUMMY_BYTE,8) != SDHC_CMD_SUCCESS; i++){		//write to specified sector
		if (i >= 10) {
			//there was no response to the command
			while(1);
		}
	}
	Buffer[0] = SPI_write(SDHC_DUMMY_BYTE);	//send 1 dummy byte as spacer
	SPI_write(SDHC_DATA_TOKEN);	//send data token
	for (int i=0;i<lengthOfData;i++){	//write the data segment 1 byte at a time
		Buffer[i%13] = SPI_write(data[i]);
	}
	for (int i=0;i<fillerBytes;i++){	//fill the rest of the sector with filler bytes
	Buffer[i%13] = SPI_write(FILLER_BYTE);
	} 
	Buffer[0] = SDHC_DUMMY_BYTE;
	for(int i=0; (i<2) || (Buffer[0] == SDHC_DUMMY_BYTE);i++){
		Buffer[0] = SPI_write(SDHC_DUMMY_BYTE);	//send 2 CRC dummy bytes and keep reading bytes until a response is seen 	
	}
	if ((Buffer[0] & SDHC_RESPONSE_STATUS_MASK) == 0x02){
		//data was written successfully
	}
	while(Buffer[0] != SDHC_DUMMY_BYTE) Buffer[0] = SPI_write(SDHC_DUMMY_BYTE);	//wait for card to finish internal processes
	SPICS(FALSE);
	SPIDisable();	
}

//the following command reads one sector from the sdhc card
void SD_read_block(uint32_t sector,uint8_t* arrayOf512Bytes){
	SPIInit(SPI_MODE_0_gc);
	SPICS(TRUE);
	
	for(int i=0;SD_command(SDHC_CMD_READ_SINGLE_BLOCK,sector,SDHC_DUMMY_BYTE,8) != SDHC_CMD_SUCCESS; i++){	//send command to read data
		if (i >= 10) {
			//there was no response to the command
			while(1);
		}
	}
	while(Buffer[0] != SDHC_DATA_TOKEN){
		Buffer[0] = SPI_write(SDHC_DUMMY_BYTE);
	}		
	for (int i=0;i<SDHC_SECTOR_SIZE;i++){
		arrayOf512Bytes[i] = SPI_write(SDHC_DUMMY_BYTE);	//read in the data
	}
	Buffer[12] = FILLER_BYTE;
	while (Buffer[12] != SDHC_DUMMY_BYTE){
		Buffer[12] = SPI_write(SDHC_DUMMY_BYTE);	
	}

	SPICS(FALSE);
	SPIDisable();
}

void checkMote(){
	ADCPower(TRUE);
	Ext1Power(TRUE);
	_delay_ms(100);
	PortEx_DIRSET(BIT3_bm, PS_BANKB);  //SD card CS	
	while(1){
		PortEx_OUTSET(BIT3_bm, PS_BANKB);	//pull SD cs high
		_delay_ms(5000);
		PortEx_OUTCLR(BIT3_bm, PS_BANKB);	//pull SD cs low
		_delay_ms(5000);
	}
}

//the following function turns on power to the sd card and port expander and initializes the sdhc card in spi mode
void SD_init(void){
	ADCPower(TRUE);				//power up portEX
	Ext1Power(TRUE);			//power up SD card
	_delay_ms(100);				//wait for bootup

	
	PortEx_DIRSET(BIT3_bm, PS_BANKB);  //SD card CS	
	PortEx_OUTSET(BIT3_bm, PS_BANKB);	//pull SD cs high

	SPIInit2(SPI_MODE_0_gc,SPI_LOWEST_CLOCKRATE_PRESCALAR);
	SPICS(TRUE);
	
	for(int i=0; i<10; i++){ // idle for 10 bytes / 80 clocks
		SPIC.DATA=SDHC_DUMMY_BYTE;
		while(!(SPIC.STATUS & SPI_IF_bm)); //wait for byte to be sent
		Buffer[12] = SPIC.DATA; //read SPI data register to reset status flag
	}
	
	SPICS(FALSE);
	SPIDisable();
	

	PortEx_OUTCLR(BIT3_bm, PS_BANKB);	//pull SD cs low
	
	SPIInit2(SPI_MODE_0_gc,SPI_LOWEST_CLOCKRATE_PRESCALAR);
	SPICS(TRUE);
	for(int i=0;SD_command(SDHC_CMD_RESET,SDHC_NO_ARGUMENTS,SDHC_CMD_RESET_CRC,8) != SDHC_IDLE_STATE; i++){		//send command 0 to put card in idle state and read 8 next bytes sent back or until response read
		if (i >= 10) {												//try command 10 times before timing out
			//there was no response to the first command
			while(1);
		}
	}
	_delay_ms(100);	
	
	for(int i=0;SD_command(SDHC_CHECK_VOLTAGE_CMD,SDHC_CHECK_VOLTAGE_ARGUMENT,SDHC_CHECK_VOLTAGE_CRC,8) != SDHC_IDLE_STATE; i++){		//check voltage range (used to indicate to sd card that we know it is an sdhc card)
		if (i >= 10) {
			//there was no response to the command
			while(1);
		}			
	}		
	for(int i=0;i<4;i++){
		Buffer[i+2] = SPI_write(SDHC_DUMMY_BYTE);
	}
	if((Buffer[4] != 0x01) || (Buffer[5] != 0xAA)){			//check that the response is the same as the argument sent in
		//broken card or voltage out of operating range bounds
	}
	/*
	used for standard (low capacity) cards
	while(Buffer[1] != 0x00){
		SD_command(0x01,0x00000000,0xFF,8);
		_delay_ms(100);
		
		SPIC.DATA = 0x41;
		while(!(SPIC.STATUS & SPI_IF_bm)); //wait for byte to be sent
		SPIBuffer[12] = SPIC.DATA; //read SPI data register to reset status flag
		for(int i=0; i<5; i++){ // send 5 dummy bytes
			SPIC.DATA=0x00;
			while(!(SPIC.STATUS & SPI_IF_bm)); //wait for byte to be sent
			SPIBuffer[12] = SPIC.DATA; //read SPI data register to reset status flag
		}
		
	}
	*/
	//send second initialization command
	while(Buffer[1]!= 0x00){
		SD_command(SDHC_ADV_COMMAND,SDHC_NO_ARGUMENTS,SDHC_DUMMY_BYTE,8);	//next command will be advanced
		SD_command(SDHC_INITIALIZATION_CMD,SDHC_INITIALIZATION_CMD_ARGUMENT,SDHC_DUMMY_BYTE,8);	//initialize the SDHC card in SPI mode
	}
	for(int i=0;SD_command(SDHC_CMD_READ_OCR,SDHC_NO_ARGUMENTS,SDHC_DUMMY_BYTE,8) != SDHC_CMD_SUCCESS; i++){		//check OCR register
		if (i >= 10) {
			//there was no response to the command
			while(1);
		}
	}		
	for (int i=0;i<4;i++){
		Buffer[i] = SPI_write(SDHC_DUMMY_BYTE);
	}
	if (Buffer[0] & 0x40){
		//the card is addressed in 512 byte sectors
	}
	SPICS(FALSE);
	SPIDisable();	
}	
//the following command writes multiple blocks/sectors to the sd card starting at a specified sector (in the sd card)
void SD_write_multiple_blocks(uint32_t sector,uint8_t* data,int lengthOfData){
	SPIInit(SPI_MODE_0_gc);
	SPICS(TRUE);
	int numSectors = lengthOfData/SDHC_SECTOR_SIZE;
	int fillerBytes = SDHC_SECTOR_SIZE - lengthOfData%SDHC_SECTOR_SIZE;
	if (fillerBytes==SDHC_SECTOR_SIZE) fillerBytes = 0;
	else numSectors++;
	SD_command(SDHC_CMD_WRITE_MULTIPLE_BLOCKS,sector,SDHC_DUMMY_BYTE,8);	//write starting at specified sector
	for (int j=0;j<numSectors;j++){
		Buffer[1] = SPI_write(SDHC_DUMMY_BYTE);	//send dummy byte
		Buffer[1] = SPI_write(SDHC_MULT_WRITE_DATA_TOKEN);	//send data token
		if(j == (numSectors-1)){
			for (int i=0;i<(SDHC_SECTOR_SIZE-fillerBytes);i++){
				Buffer[i%12] = SPI_write(data[(i+(j*SDHC_SECTOR_SIZE))]);
			}
			for (int i=0;i<fillerBytes;i++){
				Buffer[i%12] = SPI_write(FILLER_BYTE);
			}
		}
		else{
			for (int i=0;i<SDHC_SECTOR_SIZE;i++){
				Buffer[i%12] = SPI_write(data[(i+(j*SDHC_SECTOR_SIZE))]);
			}
		}
		for (int i=0;i<2;i++) Buffer[1] = SPI_write(SDHC_DUMMY_BYTE);	//write 2 CRC token
		Buffer[1] = FILLER_BYTE;
		while(Buffer[1] != SDHC_DUMMY_BYTE) Buffer[1] = SPI_write(SDHC_DUMMY_BYTE);	//wait for card to store the data it received		
	}
	for(int i=0;i<4;i++){
		Buffer[1] = SPI_write(SDHC_DUMMY_BYTE);	//write dummy byte
	}		
	Buffer[1] = SPI_write(SDHC_MULT_WRITE_STOP_TOKEN);	//write stop token
	for(int i=0;i<4;i++){
	Buffer[1] = SPI_write(SDHC_DUMMY_BYTE);	//write dummy byte
	}	
	Buffer[1] = FILLER_BYTE;
	while (Buffer[1] != SDHC_DUMMY_BYTE) Buffer[1] = SPI_write(SDHC_DUMMY_BYTE); //wait for card to finish internal processes
	SPICS(FALSE);
	SPIDisable();		
}
//the following command reads multiple blocks from the sd card starting at the specified block/sector
void SD_read_multiple_blocks(uint32_t sector,uint8_t* data,int numOfBlocks){
	SPIInit(SPI_MODE_0_gc);
	SPICS(TRUE);
	Buffer[1] = SDHC_DUMMY_BYTE;
	while(Buffer[1] != FILLER_BYTE){
		SD_command(SDHC_CMD_READ_MULTIPLE_BLOCKS,sector,SDHC_DUMMY_BYTE,8);	//send command to read data
	}
	//do the following for however many sectors to be read in		
	for (int j=0;j<numOfBlocks;j++){
		Buffer[1]=SDHC_DUMMY_BYTE;
		while(Buffer[1] != SDHC_DATA_TOKEN){ 
			Buffer[1] = SPI_write(SDHC_DUMMY_BYTE);	//wait for start of data token
		}			
		for (int i=0;i<SDHC_SECTOR_SIZE;i++){
			data[(i+(j*SDHC_SECTOR_SIZE))] = SPI_write(SDHC_DUMMY_BYTE);	//read in the data
		}
	
		for (int i=0;i<2;i++){
			Buffer[i] = SPI_write(SDHC_DUMMY_BYTE);	//read in the 2 CRC bytes
		}
	}	
	SD_command(SDHC_CMD_STOP_TRANSMISSION,SDHC_NO_ARGUMENTS,SDHC_DUMMY_BYTE,8);	//send command to stop reading data
	Buffer[0] = SPI_write(SDHC_DUMMY_BYTE);	//read the stuff byte
	Buffer[1] = FILLER_BYTE;
	while (Buffer[1] != SDHC_DUMMY_BYTE) Buffer[1] = SPI_write(SDHC_DUMMY_BYTE); //wait for card to finish internal processes
	SPICS(FALSE);
	SPIDisable();
}
//this function deselects the sd card and turns off power to the port expander and the sd card
void SD_disable(){
	PortEx_DIRCLR(BIT3_bm, PS_BANKB);  //pull SD card CS high
	PortEx_OUTCLR(BIT3_bm, PS_BANKB);
	SPIInit(SPI_MODE_0_gc);
	SPICS(TRUE);
	SPI_write(SDHC_DUMMY_BYTE);	//must write a byte to spi when cd card cs is high to have sd card release MISO line
	SPICS(FALSE);	//stop spi
	SPIDisable();
	
	ADCPower(FALSE);		//turn off portEX power
	Ext1Power(FALSE);			//power down SD card
}
//command to check reading and writing to sd card
void SD_write_and_read_knowns(){
	for (int i=0;i<24;i++) FRAMReadBuffer[i] = i;	//write 24 values to the FRAM buffer
	SD_write_block(20,FRAMReadBuffer,24);	//write those values to the card
	for (int i=0;i<24;i++) FRAMReadBuffer[i] = 0;	//clear the FRAM buffer
	SD_read_block(20,FRAMReadBuffer);	//read into the FRAM buffer from the SD card
	for(int i=0;i<1250;i++) FRAMReadBuffer[i] = i%100;	//write 1250 values to the FRAM buffer	
	SD_write_multiple_blocks(80,FRAMReadBuffer,1250);	//write those values to sd card
	for(int i=0;i<1250;i++) FRAMReadBuffer[i] = 0;	//clear FRAM buffer
	SD_read_multiple_blocks(80,FRAMReadBuffer,3);	//read in 3 blocks of data from the memory card
}

void SD_write_and_read_knowns_FAT(){
	for (int i=0;i<24;i++) FRAMReadBuffer[i] = i;	//write 24 values to the FRAM buffer
	error = writeFile(newFile);
	for (int i=0;i<24;i++) FRAMReadBuffer[i] = 0;	//clear the FRAM buffer
	error = readFile(READ,newFileR);		//read the data into the buffer from file
}
/*
//this function takes a pointer to the data and the length of the data as arguments and sends that data via the radio
void TransmitData(uint8_t* data,uint8_t dataLength){
	//write the data that is to be sent to the radio buffer
	uint16_t headerLength = gen_hdr(header,0xFFFF,dataLength+1);
	WriteFrameBuffer(data,dataLength,headerLength+1);
	//initiate transmission by writing to control register
	WriteRadioRegister(RADIOCTRLRGSTR, STARTTRAN);
	while (ReadRadioRegister(STATREG) != PLL_ON);	//wait for transmission to finish
}

//this function takes a pointer to a data array and the length of the array (must be less than 128 bytes)
//and writes that data to the radio buffer
void WriteFrameBuffer(uint8_t* data,uint8_t dataLength,uint16_t headerLength){
	SPIDInit(SPI_MODE_0_gc);	//start the spi bus for the radio
	RadioCS(TRUE);
	//PORTD.DIRSET = PIN4_bm;
	//PORTD.OUTCLR = PIN4_bm;	//select the radio with cs
	Buffer[1] = SPID_write(WRITE_RADIO_BUFFER_CMD);
	//Buffer[1] = SPID_write(dataLength);
	//write the IEEE 802.15.4 header
	for(int i=0;i<headerLength;i++){
		Buffer[i%13] = SPID_write(header[i]);
	}
	//write the data
	for(int i=0;i<dataLength;i++){
		Buffer[i%13] = SPID_write(data[i]);
	}
	//write a dummy byte which will be overwritten by radio for auto generated CRC 
	Buffer[0] = SPID_write(0xFF);
	//Buffer[0] = SPID_write(0xFF);	
	RadioCS(FALSE);
	SPID_disable();
	//PORTD.OUTCLR = PIN4_bm;	//deselect radio
	
}
//this function takes a pointer to a data address (must have free memory for at least 128 bytes) and writes the data from the 
//radio buffer to memory starting at that address 
void ReadFrameBuffer(uint8_t* data){
	uint8_t dataLength;
	SPIDInit(SPI_MODE_0_gc);	//start the spi bus for the radio
	RadioCS(TRUE);
	//PORTD.DIRSET = PIN4_bm;
	//PORTD.OUTCLR = PIN4_bm;	//select the radio with cs
	Buffer[1] = SPID_write(READ_RADIO_BUFFER_CMD);
	dataLength = SPID_write(DUMMY_BYTE);	//write dummy byte and get data length of packet
	Buffer[2] = dataLength;
	for (int i=0;i<(dataLength+3);i++){
		data[i] = SPID_write(DUMMY_BYTE); //read in the data packet followed by LQI, ED and RX_STATUS
	} 
	RadioCS(FALSE);
	SPID_disable();
	//PORTD.OUTSET = PIN4_bm;	//deselect radio
	
}
void ReadSRAM(uint8_t* data,uint8_t address,uint8_t dataLength){
	SPIDInit(SPI_MODE_0_gc);	//start the spi bus for the radio
	RadioCS(TRUE);
	//PORTD.DIRSET = PIN4_bm;
	//PORTD.OUTCLR = PIN4_bm;	//select the radio with cs
	Buffer[1] = SPID_write(READ_RADIO_SRAM_CMD);
	Buffer[2] = SPID_write(address);
	for (int i=0;i<dataLength;i++){
		data[i] = SPID_write(DUMMY_BYTE); //read in the data packet followed by LQI, ED and RX_STATUS
	}
	RadioCS(FALSE);
	SPID_disable();
	//PORTD.OUTSET = PIN4_bm;	//deselect radio
	
}
void WriteSRAM(uint8_t* data,uint8_t address,uint8_t dataLength){
	SPIDInit(SPI_MODE_0_gc);	//start the spi bus for the radio
	RadioCS(TRUE);
	//PORTD.DIRSET = PIN4_bm;
	//PORTD.OUTCLR = PIN4_bm;	//select the radio with cs
	Buffer[1] = SPID_write(WRITE_RADIO_SRAM_CMD);
	Buffer[1] = SPID_write(address);
	for(int i=0;i<dataLength;i++){
		Buffer[i%13] = SPID_write(data[i]);
	}
	RadioCS(FALSE);
	SPID_disable();
	//PORTD.OUTCLR = PIN4_bm;	//deselect radio
	
}
//this function takes a byte address of a radio register and a byte of data which it writes to that register
void WriteRadioRegister(uint8_t registerAddress, uint8_t data){
	SPIDInit(SPI_MODE_0_gc);	//start the spi bus for the radio
	RadioCS(TRUE);
	//PORTD.DIRSET = PIN4_bm;
	//PORTD.OUTCLR = PIN4_bm;	//select the radio with cs
	Buffer[1] = SPID_write(registerAddress | RWRITECMD);
	Buffer[1] = SPID_write(data);
	RadioCS(FALSE);
	SPID_disable();
	//PORTD.OUTSET = PIN4_bm;	//deselect radio
}
//this function takes a byte address of a radio register and returns the byte of data in that register
uint8_t ReadRadioRegister(uint8_t registerAddress){
	uint8_t data;
	SPIDInit(SPI_MODE_0_gc);	//start the spi bus for the radio
	RadioCS(TRUE);
	//PORTD.DIRSET = PIN4_bm;
	//PORTD.OUTCLR = PIN4_bm;	//select the radio with cs
	Buffer[1] = SPID_write((registerAddress | RREADCMD));
	data = SPID_write(DUMMY_BYTE);	//write dummy byte to radio
	RadioCS(FALSE);
	SPID_disable();
	//PORTD.OUTSET = PIN4_bm;	//deselect radio
	return data;	//return data in register	
}	
//disable SPI on port D
void SPID_disable(){
	PORTD.OUTSET = PIN4_bm;
	SPID.CTRL = 0x00;
	PORTD.OUTCLR = PIN4_bm;
	PORTD.DIRCLR = PIN4_bm | PIN5_bm | PIN7_bm;
}
*/
//select radio SPI on port D with cs
void RadioCS(uint8_t status){
	if (status) PORTD.OUTCLR = PIN4_bm;
	else {
		PORTD.OUTSET = PIN4_bm;
	}
}
/*
//enable SPI on port D
void SPIDInit(uint8_t mode){
	// init SPI SS pin (set it to high)
	PORTD.DIRSET = PIN4_bm;
	PORTD.PIN4CTRL = PORT_OPC_WIREDANDPULL_gc;
	PORTD.OUTSET = PIN4_bm;

	// init SPI
	SPID.CTRL =	SPI_PRESCALER |  // set clock speed
	0x00 |  // disable clock double
	SPI_ENABLE_bm | // Enable SPI module
	0x00 |  // set data order msb first
	SPI_MASTER_bm | // set SPI master
	mode; // set SPI mode

	// disable SPI Interrupts
	SPID.INTCTRL = SPI_INTLVL_OFF_gc;
	// set SPI-MOSI and SPI-SCK as output
	PORTD.DIRSET  = PIN5_bm | PIN7_bm;

	
}
*/
//write a byte of data over SPI port D and read a byte of data simultaneously
uint8_t SPID_write(uint8_t byteToSend){
	uint8_t data;
	SPID.DATA = byteToSend;
	while(!(SPID.STATUS & SPI_IF_bm)); //wait for byte to be sent
	data = SPID.DATA; //read SPI data register to reset status flag
	return data;
}
/*
//the following function takes a pointer to a data array (at least 128 bytes) and stores data received over the radio into that array
void ReceiveData(uint8_t* data){
	//read the data from the buffer
	ReadFrameBuffer(data);
	//set the radio state to idle
	//WriteRadioRegister(RADIOCTRLRGSTR, SET_RADIOIDLE);
	while (ReadRadioRegister(STATREG) != RX_ON);	//wait for state to turn to idle
}

void RadioListen(){
	WriteRadioRegister(RADIOCTRLRGSTR, RX_ON);
	WriteRadioRegister(IRQ_MASK_REGISTER,ENABLE_TRX_INTERRUPTS);	//enable transmission/reception interrupts on radio module
	//WriteRadioRegister(IRQ_MASK_REGISTER,0x0D);	//enable transmission/reception interrupts on radio module
	PORTD.PIN2CTRL = RISING_EDGE;	//set interrupt pin 2 on port D
	sei();
	PMIC.CTRL = ENABLE_ALL_INTERRUPT_LEVELS;
	PORTD.INT0MASK = PIN2_bm;
	PORTD.INTCTRL = PORT_INT0LVL_MED_gc;
	//WriteRadioRegister(RADIOCTRLRGSTR, RX_ON);
	//sei();
}
*/
/*
ISR(PORTD_INT0_vect){
	//nop();
	RadioCS(FALSE);
	Buffer[1] = ReadRadioRegister(IRQ_STATUS);						//combine these two lines after end of testing
	if((Buffer[1] & TRX_END_FLAG) == TRX_END_FLAG) ReceiveData(FRAMReadBuffer);
	//nop();
}
*/
/*
void TestRadio(){
	
	
	//reset radio
	PORTD.DIRSET = PIN0_bm;
	PORTD.OUTCLR = PIN0_bm;
	_delay_ms(100);
	PORTD.OUTSET = PIN0_bm;
	_delay_ms(100);
	//WriteRadioRegister(STATREG,0x06);
	Buffer[1] = ReadRadioRegister(0x04);
	//WriteRadioRegister(0x04,0x00);
	//Buffer[1] = ReadRadioRegister(0x04);
	Buffer[1] = ReadRadioRegister(0x2C);
	WriteRadioRegister(RADIOCTRLRGSTR,TRX_OFF);	//set radio to TRX_OFF state
	Buffer[1] = ReadRadioRegister(STATREG);
	WriteRadioRegister(RADIOCTRLRGSTR,PLL_ON);	//set radio to PLL_ON state
	Buffer[1] = ReadRadioRegister(STATREG);
	for (int i=0;i<10;i++) FRAMReadBuffer[i] = i*5;
	//WriteFrameBuffer(FRAMReadBuffer,10);
	//RadioListen(FRAMReadBuffer);
	//WriteRadioRegister(RADIOCTRLRGSTR,PLL_ON);	//set radio to PLL_ON state
	//Buffer[1] = ReadRadioRegister(STATREG);
	Buffer[1] = ReadRadioRegister(TRX_CTRL_2);
	WriteRadioRegister(TRX_CTRL_2,0x00);	//set radio to PLL_ON state
	Buffer[1] = ReadRadioRegister(TRX_CTRL_2);
	//set address of mote
	WriteRadioRegister(SHRT_ADDR_REG0,0x02);
	WriteRadioRegister(SHRT_ADDR_REG1,0x00); 
	/********************************************************************************  Transmit data part of test ************************************************************************/
	/*
	while(1){
		TransmitData(FRAMReadBuffer,10);
		_delay_ms(10000);
	}
	
	/********************************************************* Receive data part of test ***************************************************************************************/
	/*
	for (int i=0;i<10;i++) FRAMReadBuffer[i] = 0;
	seq = 0;	
	RadioListen();
	int i=0;
	while(1){
		i++;
		//nop();
	}
		
	WriteRadioRegister(RADIOCTRLRGSTR, RX_ON);
	//ReceiveData(FRAMReadBuffer);
	//WriteSRAM(FRAMReadBuffer,0x00,10);
	//_delay_ms(10);
	for (int i=0;i<10;i++) FRAMReadBuffer[i] = 0;
	//ReadFrameBuffer(FRAMReadBuffer);
	//ReadSRAM(FRAMReadBuffer,0x00,10);
	
	/*
	chb_init();
	for (int i=0;i<10;i++) FRAMReadBuffer[i] = i;
	chb_write(0xFFFF,FRAMReadBuffer,10);
	for (int i=0;i<10;i++) FRAMReadBuffer[i] = 0;
	chb_read(FRAMReadBuffer);
	
}
*/
void chibi_test_radio(){
		
	chb_init();
	chb_set_short_addr(0x0001);
	//chb_set_pwr(10);
	while(1) nop();								//comment this like if testing transmission
	for (int i=0;i<30;i++) FRAMReadBuffer[i] = i;
	while(1){
		Buffer[1] = chb_write(0xffff,FRAMReadBuffer,30);
		_delay_ms(10000);
	}
	//while(chb_set_state(CHB_RX_AACK_ON) != RADIO_SUCCESS);	
	/*
	for (int i=0;i<10;i++) FRAMReadBuffer[i] = 0;
	chb_read(FRAMReadBuffer);
	*/
}

/*
uint8_t gen_hdr(uint8_t *hdr, uint16_t addr, uint8_t len)
{
	uint8_t *hdr_ptr = hdr;

	// calc frame size and put in 0 position of array
	// frame size = hdr sz + payload len + fcs len
	*hdr_ptr++ = CHB_HDR_SZ + len + CHB_FCS_LEN;
	
	// use default fcf byte 0 val but test for ack request. we won't request
	// ack if broadcast. all other cases we will.
	*hdr_ptr++ = CHB_FCF_BYTE_0 | ((addr != 0xFFFF) << CHB_ACK_REQ_POS);
	*hdr_ptr++ = CHB_FCF_BYTE_1;

	*hdr_ptr++ = seq++;

	// fill out dest pan ID, dest addr, src addr
	*(uint16_t *)hdr_ptr = CHB_PAN_ID;
	hdr_ptr += sizeof(uint16_t);
	*(uint16_t *)hdr_ptr = addr;
	hdr_ptr += sizeof(uint16_t);
	*(uint16_t *)hdr_ptr = ((ReadRadioRegister(SHRT_ADDR_REG1) << 8) | ReadRadioRegister(SHRT_ADDR_REG0));
	hdr_ptr += sizeof(uint16_t);
	
	// return the len of the header
	return hdr_ptr - hdr;
}
*/