/*
 *	Beceem WiMax USB Driver.
 *	Copyright (c) 2010 Alexander von Gluck <kallisti5@unixzen.com>
 *	Distributed under the terms of the MIT license.
 *
 */

#include <ByteOrder.h>

#include "util.h"

void
convertEndian(bool write, unsigned int uiByteCount, unsigned int* buffer)
{
	unsigned int uiIndex = 0;

	if( write == true ) {
		for(uiIndex = 0; uiIndex < (uiByteCount/sizeof(unsigned int)); uiIndex++) {
			buffer[uiIndex] = htonl(buffer[uiIndex]);
		}
	} else {
		for(uiIndex = 0; uiIndex < (uiByteCount/sizeof(unsigned int)); uiIndex++) {
			buffer[uiIndex] = ntohl(buffer[uiIndex]);
		}
	}
}

uint16_t
CalculateHWChecksum(uint8_t* pu8Buffer, uint32_t u32Size)
{
	uint16_t u16CheckSum=0;
	while(u32Size--) {
		u16CheckSum += (uint8_t)~(*pu8Buffer);
		pu8Buffer++;
	}
	return u16CheckSum;
}

