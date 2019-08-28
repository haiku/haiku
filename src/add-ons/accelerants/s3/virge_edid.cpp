/*
	Haiku S3 Virge driver adapted from the X.org Virge and Savage driver.

	Copyright (C) 1994-2000 The XFree86 Project, Inc.  All Rights Reserved.

	Copyright 2007-2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2007-2008
*/

#include "accel.h"
#include "virge.h"

#include <string.h>
#include <ddc.h>
#include <edid.h>



static status_t
GetI2CSignals_Alt(void* cookie, int* _clock, int* data)
{
	uint32 index = (uint32)(addr_t)cookie;
	uint8 value = ReadCrtcReg(index);

	*_clock = (value & 0x4) != 0;
	*data = (value & 0x8) != 0;
	return B_OK;
}


static status_t
SetI2CSignals_Alt(void* cookie, int _clock, int data)
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



static status_t
GetI2CSignals(void* cookie, int* _clock, int* data)
{
	(void)cookie;		// avoid compiler warning for unused arg

	uint8 reg = ReadReg8(DDC_REG);

	*_clock = (reg & 0x4) != 0;
	*data = (reg & 0x8) != 0;
	return B_OK;
}


static status_t
SetI2CSignals(void* cookie, int _clock, int data)
{
	(void)cookie;		// avoid compiler warning for unused arg

	uint8 reg = 0x10;

	if (_clock)
		reg |= 0x1;
	if (data)
		reg |= 0x2;

	WriteReg8(DDC_REG, reg);
	return B_OK;
}



bool
Virge_GetEdidInfo(edid1_info& edidInfo)
{
	// Get the EDID info and return true if successful.

	SharedInfo& si = *gInfo.sharedInfo;

	bool bResult = false;

	if (si.chipType == S3_TRIO_3D_2X) {
		uint32 DDCPort = 0xAA;

		i2c_bus bus;
		bus.cookie = (void*)(addr_t)DDCPort;
		bus.set_signals = &SetI2CSignals_Alt;
		bus.get_signals = &GetI2CSignals_Alt;
		ddc2_init_timing(&bus);

		uint8 tmp = ReadCrtcReg(DDCPort);
		WriteCrtcReg(DDCPort, tmp | 0x13);

		bResult = (ddc2_read_edid1(&bus, &edidInfo, NULL, NULL) == B_OK);

		WriteCrtcReg(DDCPort, tmp);
	} else {
		i2c_bus bus;
		bus.cookie = (void*)DDC_REG;
		bus.set_signals = &SetI2CSignals;
		bus.get_signals = &GetI2CSignals;
		ddc2_init_timing(&bus);

		uint8 tmp = ReadReg8(DDC_REG);
		WriteReg8(DDC_REG, tmp | 0x13);

		bResult = (ddc2_read_edid1(&bus, &edidInfo, NULL, NULL) == B_OK);

		WriteReg8(DDC_REG, tmp);
	}

	return bResult;
}
