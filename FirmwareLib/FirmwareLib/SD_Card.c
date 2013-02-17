# include "E-000001-000009_firmware_rev_1_0.h"

//the following function turns on power to the sd card and port expander and initializes the sdhc card in spi mode
//returns 0 if successful or 1 if not
uint8_t SD_init(void){
	ADCPower(TRUE);				//power up portEX
	Ext1Power(TRUE);			//power up SD card
	_delay_ms(100);				//wait for bootup
	uint8_t errorCode = 0;

	
	PortEx_DIRSET(BIT3_bm, PS_BANKB);  //SD card CS	
	PortEx_OUTSET(BIT3_bm, PS_BANKB);	//pull SD cs high

	SPIInit2(SPI_MODE_0_gc,SPI_LOWEST_CLOCKRATE_PRESCALAR);
	SPICS(TRUE);
	
	for(int i=0; i<10; i++){		// idle for 10 bytes / 80 clocks
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
			errorCode = 1;
			break;
		}
	}
	_delay_ms(100);	
	
	for(int i=0;SD_command(SDHC_CHECK_VOLTAGE_CMD,SDHC_CHECK_VOLTAGE_ARGUMENT,SDHC_CHECK_VOLTAGE_CRC,8) != SDHC_IDLE_STATE; i++){		//check voltage range (used to indicate to sd card that we know it is an sdhc card)
		if (i >= 10) {
			//there was no response to the command
			errorCode = 1;
			break;
		}			
	}		
	for(int i=0;i<4;i++){
		Buffer[i+2] = SPI_write(SDHC_DUMMY_BYTE);
	}
	if((Buffer[4] != 0x01) || (Buffer[5] != 0xAA)){			//check that the response is the same as the argument sent in
		//broken card or voltage out of operating range bounds
		errorCode = 1;
	}
	//send second initialization command
	do{
		SD_command(SDHC_ADV_COMMAND,SDHC_NO_ARGUMENTS,SDHC_DUMMY_BYTE,8);	//next command will be advanced
		SD_command(SDHC_INITIALIZATION_CMD,SDHC_INITIALIZATION_CMD_ARGUMENT,SDHC_DUMMY_BYTE,8);	//initialize the SDHC card in SPI mode
	} while(Buffer[1]!= 0x00);
	
	for(int i=0;SD_command(SDHC_CMD_READ_OCR,SDHC_NO_ARGUMENTS,SDHC_DUMMY_BYTE,8) != SDHC_CMD_SUCCESS; i++){		//check OCR register
		if (i >= 10) {
			//there was no response to the command
			errorCode = 1;
			break;
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
	PortEx_OUTSET(BIT3_bm, PS_BANKB);	//pull SD cs high
	return errorCode;	
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
	PortEx_OUTCLR(BIT3_bm, PS_BANKB);	//pull SD cs low
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
	PortEx_OUTSET(BIT3_bm, PS_BANKB);	//pull SD cs high	
}

//the following command reads one sector from the sdhc card
void SD_read_block(uint32_t sector,uint8_t* arrayOf512Bytes){
	PortEx_OUTCLR(BIT3_bm, PS_BANKB);	//pull SD cs low
	SPIInit(SPI_MODE_0_gc);
	SPICS(TRUE);
	
	for(int i=0;SD_command(SDHC_CMD_READ_SINGLE_BLOCK,sector,SDHC_DUMMY_BYTE,8) != SDHC_CMD_SUCCESS; i++) {	//send command to read data
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
	PortEx_OUTSET(BIT3_bm, PS_BANKB);	//pull SD cs high
}

//the following command writes multiple blocks/sectors to the sd card starting at a specified sector (in the sd card)
void SD_write_multiple_blocks(uint32_t sector,uint8_t* data,int lengthOfData){
	PortEx_OUTCLR(BIT3_bm, PS_BANKB);	//pull SD cs low
	SPIInit(SPI_MODE_0_gc);
	SPICS(TRUE);
	int numSectors = lengthOfData/SDHC_SECTOR_SIZE;
	int fillerBytes = SDHC_SECTOR_SIZE - lengthOfData%SDHC_SECTOR_SIZE;
	if (fillerBytes==SDHC_SECTOR_SIZE) fillerBytes = 0;
	else numSectors++;
	while(SD_command(SDHC_CMD_WRITE_MULTIPLE_BLOCKS,sector,SDHC_DUMMY_BYTE,8) != SDHC_CMD_SUCCESS);	//write starting at specified sector
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
	PortEx_OUTSET(BIT3_bm, PS_BANKB);	//pull SD cs high
}
//the following command reads multiple blocks from the sd card starting at the specified block/sector
void SD_read_multiple_blocks(uint32_t sector,uint8_t* data,int numOfBlocks){
	PortEx_OUTCLR(BIT3_bm, PS_BANKB);	//pull SD cs low
	SPIInit(SPI_MODE_0_gc);
	SPICS(TRUE);
	while(SD_command(SDHC_CMD_READ_MULTIPLE_BLOCKS,sector,SDHC_DUMMY_BYTE,8) != SDHC_CMD_SUCCESS);	//send command to read data
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
	PortEx_OUTSET(BIT3_bm, PS_BANKB);	//pull SD cs high
}
//this function deselects the sd card and turns off power to the port expander and the sd card
void SD_disable(){
	PortEx_DIRSET(BIT3_bm, PS_BANKB);  //pull SD card CS high
	PortEx_OUTSET(BIT3_bm, PS_BANKB);
	SPIInit(SPI_MODE_0_gc);
	SPICS(TRUE);
	SPI_write(SDHC_DUMMY_BYTE);	//must write a byte to spi when cd card cs is high to have sd card release MISO line
	SPICS(FALSE);	//stop spi
	SPIDisable();
	
	ADCPower(FALSE);		//turn off portEX power
	Ext1Power(FALSE);			//power down SD card
}

