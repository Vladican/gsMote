/*
 * SerialUSB.h
 *
 * Created: 9/20/2013 10:25:27 AM
 *  Author: Vlad
 */ 


#ifndef SERIALUSB_H_
#define SERIALUSB_H_

#include "constants_and_globals.h"
#include "utility_functions.h"

bool StartSerial(uint32_t BaudRate);
void StopSerial();
void SerialWriteByte(uint8_t byte);
void SerialWriteBuffer(uint8_t* buffer, uint32_t length);
uint8_t SerialReadByte();



#endif /* SERIALUSB_H_ */