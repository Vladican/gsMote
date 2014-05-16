/*
 * utility_functions.h
 *
 * Created: 3/8/2014 1:24:56 PM
 *  Author: VLAD
 */ 


#ifndef UTILITY_FUNCTIONS_H_
#define UTILITY_FUNCTIONS_H_

#include "constants_and_globals.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "clksys_driver.h"
#include <string.h>
#include <stdio.h>

uint8_t ADC_POWER_ON;
// Utility functions
void set_16MHz();
void set_32MHz();
void set_32MHz_Calibrated();
void setXOSC_32MHz();
void PortEx_DIRSET(uint8_t portMask, uint8_t bank);
void PortEx_DIRCLR(uint8_t portMask, uint8_t bank);
void PortEx_OUTSET(uint8_t portMask, uint8_t bank);
void PortEx_OUTCLR(uint8_t portMask, uint8_t bank);

void portExCS(uint8_t write);

void Ext1Power(uint8_t on);
void Ext2Power(uint8_t on);
void HVPower(uint8_t on);
void lowerMuxCS(uint8_t write);
void upperMuxCS(uint8_t write);
void ADCPower(uint8_t on);
void set_filter(uint8_t filterConfig);

uint8_t readPortEx(uint8_t readRegister);
void SPIInit(uint8_t mode);
void SPIInit2(uint8_t mode, uint8_t prescalar);
void SPICS(uint8_t enable);
void SPIDisable();
uint8_t SPI_write(uint8_t byteToSend);

void DeciToString(int32_t* DecimalArray, uint32_t length, char* ReturnString);

void init();

#endif /* UTILITY_FUNCTIONS_H_ */