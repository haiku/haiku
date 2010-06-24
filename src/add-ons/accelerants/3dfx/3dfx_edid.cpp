/*
 * Copyright 2010 Haiku, Inc.  All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
 */

#include "accelerant.h"
#include "3dfx.h"

#include <string.h>
#include <ddc.h>
#include <edid.h>



static status_t
GetI2CSignals(void* cookie, int* _clock, int* data)
{
	(void)cookie;		// avoid compiler warning for unused arg

	uint32 reg = INREG32(VIDEO_SERIAL_PARALLEL_PORT);
	*_clock = (reg & VSP_SCL0_IN) ? 1 : 0;;
	*data = (reg & VSP_SDA0_IN) ? 1 : 0;
	return B_OK;
}


static status_t
SetI2CSignals(void* cookie, int _clock, int data)
{
	(void)cookie;		// avoid compiler warning for unused arg

	uint32 reg = (INREG32(VIDEO_SERIAL_PARALLEL_PORT)
		& ~(VSP_SDA0_OUT | VSP_SCL0_OUT));
	reg = (reg | (_clock ? VSP_SCL0_OUT : 0) | (data ? VSP_SDA0_OUT : 0));
	OUTREG32(VIDEO_SERIAL_PARALLEL_PORT, reg);
	return B_OK;
}



bool
TDFX_GetEdidInfo(edid1_info& edidInfo)
{
	// Get the EDID info and return true if successful.

	i2c_bus bus;

	bus.cookie = (void*)NULL;
	bus.set_signals = &SetI2CSignals;
	bus.get_signals = &GetI2CSignals;
	ddc2_init_timing(&bus);

	uint32 reg = INREG32(VIDEO_SERIAL_PARALLEL_PORT);
	OUTREG32(VIDEO_SERIAL_PARALLEL_PORT, reg | VSP_ENABLE_IIC0);

	bool bResult = (ddc2_read_edid1(&bus, &edidInfo, NULL, NULL) == B_OK);

	OUTREG32(VIDEO_SERIAL_PARALLEL_PORT, reg);

	return bResult;
}
