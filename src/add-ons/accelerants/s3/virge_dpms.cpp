/*
	Haiku S3 Virge driver adapted from the X.org Virge driver.

	Copyright (C) 1994-1999 The XFree86 Project, Inc.  All Rights Reserved.

	Copyright 2007-2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2007-2008
*/


#include "accel.h"
#include "virge.h"



uint32 
Virge_DPMSCapabilities(void)
{
	// Return DPMS modes supported by this device.

	return B_DPMS_ON | B_DPMS_STAND_BY | B_DPMS_SUSPEND | B_DPMS_OFF;
}


uint32 
Virge_GetDPMSMode(void)
{
	// Return the current DPMS mode.

	// Note:  I do not know whether the following code is correctly reading
	// the current DPMS mode.  I'm assuming that reading back the bits that
	// were set by function Virge_SetDPMSMode will give the current DPMS mode.

	uint32 mode = B_DPMS_ON;

	switch (ReadSeqReg(0x0d) & 0x70) {
		case 0:
			mode = B_DPMS_ON;
			break;
		case 0x10:
			mode = B_DPMS_STAND_BY;
			break;
		case 0x40:
			mode = B_DPMS_SUSPEND;
			break;
		case 0x50:
			mode = B_DPMS_OFF;
			break;
		default:
			TRACE("Unknown DPMS mode, reg sr0D: 0x%X\n", ReadSeqReg(0x0d));
	}

	TRACE("Virge_GetDPMSMode() mode: %d\n", mode);
	return mode;
}


status_t 
Virge_SetDPMSMode(uint32 dpmsMode)
{
	// Set the display into one of the Display Power Management modes,
	// and return B_OK if successful, else return B_ERROR.

	TRACE("Virge_SetDPMSMode() mode: %d\n", dpmsMode);

	WriteSeqReg(0x08, 0x06);		// unlock extended sequencer regs

	uint8 sr0D = ReadSeqReg(0x0d) & 0x03;

	switch (dpmsMode) {
		case B_DPMS_ON:
			break;
		case B_DPMS_STAND_BY:
			sr0D |= 0x10;
			break;
		case B_DPMS_SUSPEND:
			sr0D |= 0x40;
			break;
		case B_DPMS_OFF:
			sr0D |= 0x50;
			break;
		default:
			TRACE("Invalid DPMS mode %d\n", dpmsMode);
			return B_ERROR;
	}

	WriteSeqReg(0x0d, sr0D);

	return B_OK;
}
