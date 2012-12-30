#include "E-000001-000009_firmware_rev_1_0.h"


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


void FRAMWriteKnownsCheck() {
	FRAMWriteKnowns();
	ADCPower(TRUE);

	_delay_us(250);
	calcChecksumFRAM();

	ADCPower(FALSE);

}

//random function for testing stuff	
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
//check writing and reading to file on sd card
void SD_write_and_read_knowns_FAT(){
	for (int i=0;i<24;i++) FRAMReadBuffer[i] = i;	//write 24 values to the FRAM buffer
	error = writeFile("testing");
	for (int i=0;i<24;i++) FRAMReadBuffer[i] = 0;	//clear the FRAM buffer
	error = readFile(READ,"testing");		//read the data into the buffer from file
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
//function for testing radio transmission
void chibi_test_radio(){
		
	chb_init();
	chb_set_short_addr(0x0002);
	while(1) nop();								//comment this line if testing transmission
	for (int i=0;i<333;i++) FRAMReadBuffer[i] = i%200;
	chb_write(0x0002,FRAMReadBuffer,333);
	/*
	while(1){
		Buffer[1] = chb_write(0xffff,FRAMReadBuffer,30);
		_delay_ms(10000);
	}
	*/
	//while(chb_set_state(CHB_RX_AACK_ON) != RADIO_SUCCESS);	
	/*
	for (int i=0;i<10;i++) FRAMReadBuffer[i] = 0;
	chb_read(FRAMReadBuffer);
	*/
}
//another testing function for sd card
void TestCard(){
	SD_init();
	getBootSectorData();
	for (int i=0;i<512;i++) FRAMReadBuffer[i] = i%121;
//for (int i=0;i<1;i++) {
	//FRAMReadBuffer[0] = i; 
	writeFile("testing");//}
	nop();
}