/*

*/

#ifndef FRAM_H
#define FRAM_H

#include "utility_functions.h"

void writeFRAM(uint8_t* buffer, uint16_t length);
void readFRAM (uint16_t numBytes);

#endif