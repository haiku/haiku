/*
	Copyright 2010 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2010
*/


#include "accelerant.h"
#include "3dfx.h"



#define H_SYNC_OFF	BIT(3)
#define V_SYNC_OFF	BIT(1)


uint32
TDFX_DPMSCapabilities(void)
{
	// Return DPMS modes supported by this device.

	return B_DPMS_ON | B_DPMS_STAND_BY | B_DPMS_SUSPEND | B_DPMS_OFF;
}


uint32
TDFX_GetDPMSMode(void)
{
	// Return the current DPMS mode.

	uint32 tmp = INREG32(DAC_MODE) & (H_SYNC_OFF | V_SYNC_OFF);
	uint32 mode;

	if (tmp == 0 )
		mode = B_DPMS_ON;
	else if (tmp == H_SYNC_OFF)
		mode = B_DPMS_STAND_BY;
	else if (tmp == V_SYNC_OFF)
		mode = B_DPMS_SUSPEND;
	else
		mode = B_DPMS_OFF;

	TRACE("TDFX_DPMSMode() mode: %d\n", mode);
	return mode;
}


status_t
TDFX_SetDPMSMode(uint32 dpmsMode)
{
	// Set the display into one of the Display Power Management modes,
	// and return B_OK if successful, else return B_ERROR.

	TRACE("TDFX_SetDPMSMode() mode: %d\n", dpmsMode);

	uint32 dacMode = INREG32(DAC_MODE) & ~(H_SYNC_OFF | V_SYNC_OFF);

	switch (dpmsMode) {
		case B_DPMS_ON:
			// Screen: On; HSync: On, VSync: On.
			break;

		case B_DPMS_STAND_BY:
			// Screen: Off; HSync: Off, VSync: On.
			dacMode |= H_SYNC_OFF;
			break;

		case B_DPMS_SUSPEND:
			// Screen: Off; HSync: On, VSync: Off.
			dacMode |= V_SYNC_OFF;
			break;

		case B_DPMS_OFF:
			// Screen: Off; HSync: Off, VSync: Off.
			dacMode |= H_SYNC_OFF | V_SYNC_OFF;
			break;

		default:
			TRACE("Invalid DPMS mode %d\n", dpmsMode);
			return B_ERROR;
	}

	OUTREG32(DAC_MODE, dacMode);
	return B_OK;
}
