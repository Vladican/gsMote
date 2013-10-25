/*
 * SD_Card.h
 *
 * Created: 2/16/2013 9:12:05 PM
 *  Author: Vlad
 */ 


#ifndef SD_CARD_H_
#define SD_CARD_H_

#include "E-000001-000009_firmware_rev_1_0.h"

//SD card defines
#define SDHC_SECTOR_SIZE 512
#define SDHC_CMD_RESET 0x00
#define SDHC_CMD_START_INITIALIZATION 0x01
#define SDHC_CMD_SEND_CSD 0x09
#define SDHC_CMD_SEND_CID 0x0A
#define SDHC_CMD_STOP_TRANSMISSION 12
#define SDHC_CMD_SEND_STATUS 0x0           13
#define SDHC_CMD_SET_BLOCK_SIZE        16
#define SDHC_CMD_READ_SINGLE_BLOCK     17
#define SDHC_CMD_READ_MULTIPLE_BLOCKS  18
#define SDHC_CMD_WRITE_SINGLE_BLOCK    24
#define SDHC_CMD_WRITE_MULTIPLE_BLOCKS 25
#define SDHC_CMD_PROGRAM_CSD           27
#define SDHC_CMD_SET_WRITE_PROT        28
#define SDHC_CMD_CLR_WRITE_PROT        29
#define SDHC_CMD_SEND_WRITE_PROT       30
#define SDHC_CMD_TAG_SECTOR_START      32
#define SDHC_CMD_TAG_SECTOR_END        33
#define SDHC_CMD_UNTAG_SECTOR          34
#define SDHC_CMD_TAG_ERASE_GROUP_START 35
#define SDHC_CMD_TAG_ERASE_GROUP_END   36
#define SDHC_CMD_UNTAG_ERASE_GROUP     37
#define SDHC_CMD_ERASE                 38
#define SDHC_CMD_LOCK_UNLOCK           42
#define SDHC_CMD_READ_OCR              58
#define SDHC_CMD_CRC_ON_OFF            59
#define SDHC_DATA_TOKEN               0xFE
#define SDHC_RESPONSE_OK              0x05
#define SDHC_RESPONSE_CRC_ERROR       0x0B
#define SDHC_RESPONSE_WRITE_ERROR     0x0D
#define SDHC_RESPONSE_MASK            0x1F

#define SDHC_COMMAND_START	0x40
#define SDHC_DUMMY_BYTE 0xFF
#define SDHC_RESPONSE_STATUS_MASK 0x0E
#define SDHC_CMD_RESET_CRC 0x95
#define SDHC_NO_ARGUMENTS 0x00000000
#define SDHC_IDLE_STATE 0x01
#define SDHC_CHECK_VOLTAGE_CMD 8
#define SDHC_CHECK_VOLTAGE_ARGUMENT 0x000001AA
#define SDHC_CHECK_VOLTAGE_CRC 0x87
#define SDHC_ADV_COMMAND 55
#define SDHC_INITIALIZATION_CMD 1
#define SDHC_INITIALIZATION_CMD_ARGUMENT 0x40000000
#define SDHC_MULT_WRITE_DATA_TOKEN 0xFC
#define SDHC_MULT_WRITE_STOP_TOKEN 0xFD
#define SDHC_CMD_SUCCESS 0x00

//global variables for SD card
uint8_t SDBuffer[512];

//SD functions
uint8_t SD_command(uint8_t cmd, uint32_t arg, uint8_t crc, int read);
void SD_write_block(uint32_t sector,uint8_t* data, int lengthOfData);
void SD_read_block(uint32_t sector,uint8_t* arrayOf512Bytes);
uint8_t SD_init(void);
void SD_write_multiple_blocks(uint32_t sector,uint8_t* data,int lengthOfData);
void SD_read_multiple_blocks(uint32_t sector,uint8_t* data,int numOfBlocks);
void SD_disable();
void SD_write_and_read_knowns();
void SD_write_and_read_knowns_FAT();


#endif /* SD_CARD_H_ */