
#ifndef FIRMWARE_HEADER
#define FIRMWARE_HEADER

// #define HIGHEST_STACK_LAYER MAC
// #define PAL_GENERIC_TYPE XMEGA
// #define PAL_TYPE ATXMEGA256A3
// //#define BOARD_TYPE REB_5_0_STK600
// #define TAL_TYPE AT86RF212
// #define DISABLE_IEEE_ADDR_CHECK
// #define FFD

#include "constants_and_globals.h"
#include "utility_functions.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "FAT32.h"
//#include "chb.h"
//#include "chb_drvr.h"
#include "ADC.h"
#include "SD_Card.h"
#include "SerialUSB.h"
#include "FRAM.h"
#include "app_config.h"
#include "return_val.h"
#include "pal.h"
#include "tal.h"
#include "mac_api.h"



// //Hardware Defines
// //General SPI Prescaler
// #define SPI_PRESCALER SPI_PRESCALER_DIV16_gc
// #define SPI_PRESCALER_TIGHT SPI_PRESCALER_DIV4_gc
// 
// 
// //Op codes for Ramtron FM25V05-G
// #define FR_WREN 0x06  // Set Write Enable Latch
// #define FR_WRDI 0x04  // Write Disable
// #define FR_RDSR 0x05  // Read Status Register
// #define FR_WRSR 0x01  // Write Status Register
// #define FR_READ 0x03  // Read Memory Data
// #define FR_FSTRD 0x0B // Fast Read Memory Data
// #define FR_WRITE 0x02  // Write Memory Data
// #define FR_SLEEP 0xB9  // Enter Sleep Mode
// #define FR_RDID 0x9F  // Read Device ID
// #define FR_BASEADD 0x0000
// #define FR_CAPACITY 65536 // 64KB
// #define FR_TOTAL_NUM_SAMPLES 7281 // closest multiple of 9 bytes to capacity
// #define FR_TOTAL_NUM_SE_SAMPLES 21843 // closest multiple of 3 bytes per channel to capacity
// #define FR_READ_BUFFER_SIZE 7281  // (bytes) fits within memory of microprocessor evenly divisible by 9
// #define FR_READ_BUFFER_SAMPLES 809  // 7281 / 9 bytes per sample
// #define FR_NUM_READ_BUFFERS 9 // 65536 / 7281
// #define FR_SPI_CONFIG_gc 0xD0
// 
// //Codes for MCP23S17 Port Selector
// #define PS_WRITE 0x40
// #define PS_READ 0x41
// #define PS_GPIOA 0x12
// #define PS_GPIOB 0x13
// #define PS_OLATA 0x14
// #define PS_OLATB 0x15
// #define PS_IODIRA 0x00
// #define PS_IODIRB 0x01
// #define PS_SPI_MODE SPI_MODE_0_gc
// #define PS_BANKA 0x01
// #define PS_BANKB 0x00
// #define PS_HIGH 0x01
// #define PS_LOW 0x00
// 
// //Software Defines
// #define TRUE 1
// #define FALSE 0
// #define DATA_GATHERING 0x01
// #define TIME_SYNCH 0x02
// #define SYNCHED 0x03
// 
// //Bit masks
// #define BIT0_bm 0x01
// #define BIT1_bm 0x02
// #define BIT2_bm 0x04
// #define BIT3_bm 0x08
// #define BIT4_bm 0x10
// #define BIT5_bm 0x20
// #define BIT6_bm 0x40
// #define BIT7_bm 0x80
// 
// 
// 
// #define LSBYTE_MASK 0xFF
// #define SPI_LOWEST_CLOCKRATE_PRESCALAR 0x03
// #define FILLER_BYTE 0x00
// #define ENABLE_ALL_INTERRUPT_LEVELS 0x07
// 
// //Global Variables
// volatile uint8_t bankA_DIR, bankA_OUT, bankB_DIR, bankB_OUT;  // status of port expander current configurations to allow bit level changes
// volatile uint8_t Buffer[13];
// //volatile uint8_t FRAMReadBuffer[FR_READ_BUFFER_SIZE]; // storage for reading FRAM
// uint8_t FRAMReadBuffer[FR_READ_BUFFER_SIZE]; // storage for reading FRAM
// volatile uint32_t StartOfFreeSpace;
// volatile uint8_t error;
// volatile uint16_t FRAMAddress;  // address counters for FRAM write/read
// volatile uint8_t RadioMonitorMode;	//used by synch module
// volatile uint16_t MotesReadyToSynch;	//used by synch module
// uint8_t moteID;	//used by synch module


// void FRAMTest3Channel();
// void FRAMTest1Channel();
// void FRAMWriteKnownsCheck();
// 
// 
// // Utility functions
// void set_16MHz();
// void set_32MHz();
// void setXOSC_32MHz();
// void PortEx_DIRSET(uint8_t portMask, uint8_t bank);
// void PortEx_DIRCLR(uint8_t portMask, uint8_t bank);
// void PortEx_OUTSET(uint8_t portMask, uint8_t bank);
// void PortEx_OUTCLR(uint8_t portMask, uint8_t bank);
// 
// void portExCS(uint8_t write);
// 
// void Ext1Power(uint8_t on);
// void Ext2Power(uint8_t on);
// void HVPower(uint8_t on);
// void lowerMuxCS(uint8_t write);
// void upperMuxCS(uint8_t write);
// 
// uint8_t readPortEx(uint8_t readRegister);
// void SPIInit(uint8_t mode);
// void SPIInit2(uint8_t mode, uint8_t prescalar);
// void SPICS(uint8_t enable);
// void SPIDisable();
// 
// void readFRAM (uint16_t numBytes);
// void calcChecksumFRAM(void);
// void FRAMWriteKnowns();
// 
// 
// //function prototypes for SD card
// uint8_t SPI_write(uint8_t byteToSend);
// 
// //miscellaneous testing functions
// void checkMote();
// void chibi_test_radio();
// 
// 
// void TestCard();
// 
// void DeciToString(int32_t* DecimalArray, uint32_t length, char* ReturnString);

#endif