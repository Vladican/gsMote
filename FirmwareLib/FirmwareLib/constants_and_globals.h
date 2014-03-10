/*
 * constants_and_globals.h
 *
 * Created: 3/8/2014 1:03:38 PM
 *  Author: VLAD
 */ 


#ifndef CONSTANTS_AND_GLOBALS_H_
#define CONSTANTS_AND_GLOBALS_H_

#define F_CPU 32000000UL

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>


// Hardware Defines
// General SPI Prescaler
#define SPI_PRESCALER SPI_PRESCALER_DIV16_gc
#define SPI_PRESCALER_TIGHT SPI_PRESCALER_DIV4_gc

//SPI defines
#define ADC_SPI_CONFIG_gc 0x54

// Op codes for Ramtron FM25V05-G
#define FR_WREN 0x06  // Set Write Enable Latch
#define FR_WRDI 0x04  // Write Disable
#define FR_RDSR 0x05  // Read Status Register
#define FR_WRSR 0x01  // Write Status Register
#define FR_READ 0x03  // Read Memory Data
#define FR_FSTRD 0x0B // Fast Read Memory Data
#define FR_WRITE 0x02  // Write Memory Data
#define FR_SLEEP 0xB9  // Enter Sleep Mode
#define FR_RDID 0x9F  // Read Device ID
#define FR_BASEADD 0x0000
#define FR_CAPACITY 65536 // 64KB
#define FR_TOTAL_NUM_SAMPLES 7281 // closest multiple of 9 bytes to capacity
#define FR_TOTAL_NUM_SE_SAMPLES 21843 // closest multiple of 3 bytes per channel to capacity
#define FR_READ_BUFFER_SIZE 7281  // (bytes) fits within memory of microprocessor evenly divisible by 9
#define FR_READ_BUFFER_SAMPLES 809  // 7281 / 9 bytes per sample
#define FR_NUM_READ_BUFFERS 9 // 65536 / 7281
#define FR_SPI_CONFIG_gc 0xD0

// Codes for MCP23S17 Port Selector
#define PS_WRITE 0x40
#define PS_READ 0x41
#define PS_GPIOA 0x12
#define PS_GPIOB 0x13
#define PS_OLATA 0x14
#define PS_OLATB 0x15
#define PS_IODIRA 0x00
#define PS_IODIRB 0x01
#define PS_SPI_MODE SPI_MODE_0_gc
#define PS_BANKA 0x01
#define PS_BANKB 0x00
#define PS_HIGH 0x01
#define PS_LOW 0x00

// Software Defines
#define TRUE 1
#define FALSE 0
#define DATA_GATHERING 0x01
#define TIME_SYNCH 0x02
#define SYNCHED 0x03

// Bit masks
#define BIT0_bm 0x01
#define BIT1_bm 0x02
#define BIT2_bm 0x04
#define BIT3_bm 0x08
#define BIT4_bm 0x10
#define BIT5_bm 0x20
#define BIT6_bm 0x40
#define BIT7_bm 0x80



#define LSBYTE_MASK 0xFF
#define SPI_LOWEST_CLOCKRATE_PRESCALAR 0x03
#define FILLER_BYTE 0x00
#define ENABLE_ALL_INTERRUPT_LEVELS 0x07

// Global Variables
volatile uint8_t bankA_DIR, bankA_OUT, bankB_DIR, bankB_OUT;  // status of port expander current configurations to allow bit level changes
volatile uint8_t Buffer[13];
//volatile uint8_t FRAMReadBuffer[FR_READ_BUFFER_SIZE]; // storage for reading FRAM
uint8_t FRAMReadBuffer[FR_READ_BUFFER_SIZE]; // storage for reading FRAM
volatile uint32_t StartOfFreeSpace;
volatile uint8_t error;
volatile uint16_t FRAMAddress;  // address counters for FRAM write/read
volatile uint8_t RadioMonitorMode;	//used by synch module
volatile uint16_t MotesReadyToSynch;	//used by synch module
uint8_t moteID;	//used by synch module
volatile uint8_t SPIBuffer[13];
volatile uint8_t channelStatus;  // copy of channel filter configuration to allow bit level changes



#endif /* CONSTANTS_AND_GLOBALS_H_ */