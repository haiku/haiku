/*
	Haiku S3 Savage driver adapted from the X.org Savage driver.

	Copyright (C) 1994-2000 The XFree86 Project, Inc.	All Rights Reserved.
	Copyright (c) 2003-2006, X.Org Foundation

	Copyright 2007-2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2007-2008
*/

#include "accel.h"
#include "savage.h"

#include <string.h>
#include <ddc.h>
#include <edid.h>



static status_t
GetI2CSignals(void* cookie, int* _clock, int* data)
{
	uint32 index = (uint32)(addr_t)cookie;
	uint8 value = ReadCrtcReg(index);

	*_clock = (value & 0x4) != 0;
	*data = (value & 0x8) != 0;
	return B_OK;
}


static status_t
SetI2CSignals(void* cookie, int _clock, int data)
{
	uint32 index = (uint32)(addr_t)cookie;
	uint8 value = 0x10;

	if (_clock)
		value |= 0x1;
	if (data)
		value |= 0x2;

	WriteCrtcReg(index, value);
	return B_OK;
}



bool
Savage_GetEdidInfo(edid1_info& edidInfo)
{
	// Get the EDID info and return true if successful.

	SharedInfo& si = *gInfo.sharedInfo;

	uint32 DDCPort = 0;

    switch (si.chipType) {
	case S3_SAVAGE_3D:
	case S3_SAVAGE_MX:
	case S3_SUPERSAVAGE:
	case S3_SAVAGE2000:
		DDCPort = 0xAA;
		break;

	case S3_SAVAGE4:
	case S3_PROSAVAGE:
	case S3_TWISTER:
	case S3_PROSAVAGE_DDR:
		DDCPort = 0xB1;
		break;
    }

	i2c_bus bus;
	bus.cookie = (void*)(addr_t)DDCPort;
	bus.set_signals = &SetI2CSignals;
	bus.get_signals = &GetI2CSignals;
	ddc2_init_timing(&bus);

	uint8 tmp = ReadCrtcReg(DDCPort);
	WriteCrtcReg(DDCPort, tmp | 0x13);

	bool bResult = (ddc2_read_edid1(&bus, &edidInfo, NULL, NULL) == B_OK);
	WriteCrtcReg(DDCPort, tmp);

	return bResult;
}
