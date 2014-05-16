/*
 * utility_functions.c
 *
 * Created: 3/8/2014 1:25:15 PM
 *  Author: VLAD
 */ 

#include "utility_functions.h"

// Sets the external 16MHz crystal on XTAL1 and XTAL2 as the system clock.
// There was a problem with some of the hardware modules not having the crystal
// so this function will fail until the oscillator is installed.
//  - LS900-SI-02 does not contain the crystal
//  - SFLX02-A03 does contain the crystal
// The high frequency crystal is required to clock the ADC without jitter

void init(){
	ADC_POWER_ON = 0;
}

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

void set_32MHz_Calibrated() {
	
	#define F_CPU 32000000UL
	// select 32MHz Oscillator and prescale by 1
	CLKSYS_Enable(OSC_RC32MEN_bm);
	CLKSYS_Prescalers_Config( CLK_PSADIV_1_gc, CLK_PSBCDIV_1_1_gc );
	// wait for signal to stabilize
	do {} while (CLKSYS_IsReady(OSC_RC32MRDY_bm) == 0);
	// Select 32kHz crystal and low power mode
	CLKSYS_XOSC_Config( 0, true, OSC_XOSCSEL_32KHz_gc );
	CLKSYS_Enable( OSC_XOSCEN_bm );
	//wait for the 32kHz crystal to stabilize
	do {} while (CLKSYS_IsReady(OSC_XOSCRDY_bm) == 0);
	//set the 32kHz crystal to calibrate the 32MHz RC oscillator
	CLKSYS_AutoCalibration_Enable( OSC_RC32MCREF_bm, true );
	//set the calibrated 32MHz RC oscillator as system clock
	CLKSYS_Main_ClockSource_Select( CLK_SCLKSEL_RC32M_gc );
	CLKSYS_Disable( OSC_RC2MEN_bm );
}


// produces consistent but inaccurate clock period.
void set_32MHz() {
	#define F_CPU 32000000UL
	// select 32MHz Oscillator and prescale by 1
	CLKSYS_Enable(OSC_RC32MEN_bm);
	CLKSYS_Prescalers_Config( CLK_PSADIV_1_gc, CLK_PSBCDIV_1_1_gc );
	// wait for signal to stabilize
	do {} while (CLKSYS_IsReady(OSC_RC32MRDY_bm) == 0);
	
	CLKSYS_Main_ClockSource_Select( CLK_SCLKSEL_RC32M_gc );
	CLKSYS_Disable( OSC_RC2MEN_bm );
	
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

void ADCPower(uint8_t on) {
	
	if (on && !ADC_POWER_ON) {
		PORTA.DIRSET = PIN1_bm| PIN2_bm | PIN3_bm | PIN4_bm | PIN6_bm | PIN7_bm; // portEx-CS and HV1/HV2 and A0
		PORTB.DIRSET = PIN1_bm| PIN2_bm | PIN3_bm; // FRAM-CS and A1/A2
		PORTC.DIRSET = PIN0_bm | PIN1_bm;// VDCA and VDC-2 and MUX-SYNC1
		PORTE.DIRSET = PIN4_bm; // MUX-SYNC2
		PORTF.DIRSET = PIN1_bm | PIN2_bm | PIN3_bm; // DAC LDAC and CS

		// high signal to write protect
		PORTA.OUTSET = PIN1_bm| PIN2_bm | PIN3_bm | PIN4_bm | PIN7_bm; // portEx-CS
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
		ADC_POWER_ON = TRUE;

	} else if(!on && ADC_POWER_ON) {
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
		ADC_POWER_ON = FALSE;
	}
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

void DeciToString(int32_t* DecimalArray, uint32_t length, char* ReturnString){
	char b[20];
	//int written;
	uint32_t i;
	ReturnString[0] = 0;
	for(i=0;i<length;i++){
		//written = sprintf(b,"%ld",DecimalArray[i]);
		sprintf(b,"%ld",DecimalArray[i]);
		strcat(ReturnString,b);
		//add a space between each value
		strcat(ReturnString,"\n");
	}
}

//the following command writes/reads a byte via spi
uint8_t SPI_write(uint8_t byteToSend){
	uint8_t data;
	SPIC.DATA = byteToSend;
	while(!(SPIC.STATUS & SPI_IF_bm)); //wait for byte to be sent
	data = SPIC.DATA; //read SPI data register to reset status flag
	return data;
}