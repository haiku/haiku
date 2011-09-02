/*
 *	Beceem WiMax USB Driver.
 *	Copyright (c) 2010 Alexander von Gluck <kallisti5@unixzen.com>
 *	Distributed under the terms of the MIT license.
 *
 */

#include "Driver.h"

void convertEndian(bool write, unsigned int uiByteCount, unsigned int* buffer);
uint16_t CalculateHWChecksum(uint8_t* pu8Buffer, uint32_t u32Size);

