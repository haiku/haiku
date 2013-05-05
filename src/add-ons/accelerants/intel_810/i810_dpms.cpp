/*
 * Copyright 2012 Haiku, Inc.  All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
 */

/*!
	Haiku Intel-810 video driver was adapted from the X.org intel driver which
	has the following copyright.

	Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
	All Rights Reserved.
 */


#include "accelerant.h"
#include "i810_regs.h"


#define DPMS_SYNC_SELECT	0x5002
#define H_SYNC_OFF			0x02
#define V_SYNC_OFF			0x08


uint32
I810_DPMSCapabilities(void)
{
	// Return DPMS modes supported by this device.

	return B_DPMS_ON | B_DPMS_STAND_BY | B_DPMS_SUSPEND | B_DPMS_OFF;
}


uint32
I810_GetDPMSMode(void)
{
	// Return the current DPMS mode.

	uint32 tmp = INREG8(DPMS_SYNC_SELECT) & (H_SYNC_OFF | V_SYNC_OFF);
	uint32 mode;

	if (tmp == 0 )
		mode = B_DPMS_ON;
	else if (tmp == H_SYNC_OFF)
		mode = B_DPMS_STAND_BY;
	else if (tmp == V_SYNC_OFF)
		mode = B_DPMS_SUSPEND;
	else
		mode = B_DPMS_OFF;

	TRACE("I810_DPMSMode() mode: %d\n", mode);
	return mode;
}


status_t
I810_SetDPMSMode(uint32 dpmsMode)
{
	// Set the display into one of the Display Power Management modes,
	// and return B_OK if successful, else return B_ERROR.

	TRACE("I810_SetDPMSMode() mode: %d\n", dpmsMode);

	uint8 seq01 = ReadSeqReg(1) & ~0x20;
	uint8 dpmsSyncSelect = 0;

	switch (dpmsMode) {
		case B_DPMS_ON:
			// Screen: On; HSync: On, VSync: On.
			break;

		case B_DPMS_STAND_BY:
			// Screen: Off; HSync: Off, VSync: On.
			seq01 |= 0x20;
			dpmsSyncSelect = H_SYNC_OFF;
			break;

		case B_DPMS_SUSPEND:
			// Screen: Off; HSync: On, VSync: Off.
			seq01 |= 0x20;
			dpmsSyncSelect = V_SYNC_OFF;
			break;

		case B_DPMS_OFF:
			// Screen: Off; HSync: Off, VSync: Off.
			seq01 |= 0x20;
			dpmsSyncSelect = H_SYNC_OFF | V_SYNC_OFF;
			break;

		default:
			TRACE("Invalid DPMS mode %d\n", dpmsMode);
			return B_ERROR;
	}

	WriteSeqReg(1, seq01);		// turn the screen on/off
	OUTREG8(DPMS_SYNC_SELECT, dpmsSyncSelect);		// set DPMS mode

	return B_OK;
}
