/*
	Haiku S3 Savage driver adapted from the X.org Savage driver.

	Copyright (C) 1994-2000 The XFree86 Project, Inc.  All Rights Reserved.
	Copyright (c) 2003-2006, X.Org Foundation

	Copyright 2007-2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2006-2008
*/


#include "accel.h"
#include "savage.h"



uint32 
Savage_DPMSCapabilities(void)
{
	// Return DPMS modes supported by this device.

	return B_DPMS_ON | B_DPMS_STAND_BY | B_DPMS_SUSPEND | B_DPMS_OFF;
}


uint32 
Savage_GetDPMSMode(void)
{
	// Return the current DPMS mode.

	// Note:  I do not know whether the following code is correctly reading
	// the current DPMS mode.  I'm assuming that reading back the bits that
	// were set by function SET_DPMS_MODE will give the current DPMS mode.

	SharedInfo& si = *gInfo.sharedInfo;
	uint32 mode = B_DPMS_ON;

	if (si.displayType == MT_CRT) {
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
	} else if (si.displayType == MT_LCD || si.displayType == MT_DFP) {
		mode = ((ReadSeqReg(0x31) & 0x10) ? B_DPMS_ON : B_DPMS_OFF);
	}

	TRACE("Savage_GetDPMSMode() mode: %d\n", mode);
	return mode;
}


status_t 
Savage_SetDPMSMode(uint32 dpmsMode)
{
	// Set the display into one of the Display Power Management modes,
	// and return B_OK if successful, else return B_ERROR.

	SharedInfo& si = *gInfo.sharedInfo;

	TRACE("Savage_SetDPMSMode() mode: %d, display type: %d\n", dpmsMode, si.displayType);

	if (si.displayType == MT_CRT) {
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
	} else if (si.displayType == MT_LCD || si.displayType == MT_DFP) {
		switch (dpmsMode) {
			case B_DPMS_ON:
				WriteSeqReg(0x31, 0x10, 0x10);	// SR31 bit 4 - FP enable
				break;
			case B_DPMS_STAND_BY:
			case B_DPMS_SUSPEND:
			case B_DPMS_OFF:
				WriteSeqReg(0x31, 0x00, 0x10);	// SR31 bit 4 - FP enable
				break;
			default:
				TRACE("Invalid DPMS mode %d\n", dpmsMode);
				return B_ERROR;
		}
	}

	return B_OK;
}

