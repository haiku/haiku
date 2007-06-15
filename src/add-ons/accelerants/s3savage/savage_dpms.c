/*
	Haiku S3 Savage driver adapted from the X.org Savage driver.

	Copyright (C) 1994-2000 The XFree86 Project, Inc.  All Rights Reserved.
	Copyright (c) 2003-2006, X.Org Foundation

	Copyright 2007 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2006-2007
*/


#include "GlobalData.h"
#include "AccelerantPrototypes.h"
#include "savage.h"



status_t 
SET_DPMS_MODE(uint32 dpmsMode)
{
	// Set the display into one of the Display Power Management modes,
	// and return B_OK if successful, else return B_ERROR.

	uint8 sr8 = 0x00, srd = 0x00;

	TRACE(("SET_DPMS_MODE, mode: %d, display type: %d\n", dpmsMode, si->displayType));

	if (si->displayType == MT_CRT) {
		OUTREG8(VGA_SEQ_INDEX, 0x08);
		sr8 = INREG8(VGA_SEQ_DATA);
		sr8 |= 0x06;
		OUTREG8(VGA_SEQ_DATA, sr8);

		OUTREG8(VGA_SEQ_INDEX, 0x0d);
		srd = INREG8(VGA_SEQ_DATA);

		srd &= 0x03;

		switch (dpmsMode) {
		case B_DPMS_ON:
			break;
		case B_DPMS_STAND_BY:
			srd |= 0x10;
			break;
		case B_DPMS_SUSPEND:
			srd |= 0x40;
			break;
		case B_DPMS_OFF:
			srd |= 0x50;
			break;
		default:
			TRACE(("Invalid DPMS mode %d\n", dpmsMode));
			return B_ERROR;
		}

		OUTREG8(VGA_SEQ_INDEX, 0x0d);
		OUTREG8(VGA_SEQ_DATA, srd);
	} else if (si->displayType == MT_LCD || si->displayType == MT_DFP) {
		switch (dpmsMode) {
		case B_DPMS_ON:
			OUTREG8(VGA_SEQ_INDEX, 0x31); /* SR31 bit 4 - FP enable */
			OUTREG8(VGA_SEQ_DATA, INREG8(VGA_SEQ_DATA) | 0x10);
			break;
		case B_DPMS_STAND_BY:
		case B_DPMS_SUSPEND:
		case B_DPMS_OFF:
			OUTREG8(VGA_SEQ_INDEX, 0x31); /* SR31 bit 4 - FP enable */
			OUTREG8(VGA_SEQ_DATA, INREG8(VGA_SEQ_DATA) & ~0x10);
			break;
		default:
			TRACE(("Invalid DPMS mode %d\n", dpmsMode));
			return B_ERROR;
		}
	}

	return B_OK;
}


uint32 
DPMS_CAPABILITIES (void)
{
	// Return DPMS modes supported by this device.

	return B_DPMS_ON | B_DPMS_STAND_BY | B_DPMS_SUSPEND | B_DPMS_OFF;
}


uint32 
DPMS_MODE (void)
{
	// Return the current DPMS mode.

	// Note:  I do not know whether the following code is correctly reading
	// the current DPMS mode.  I'm assuming that reading back the bits that
	// were set by function SET_DPMS_MODE will give the current DPMS mode.

	uint32 mode = B_DPMS_ON;
	uint8 srd;

	if (si->displayType == MT_CRT) {
		OUTREG8(VGA_SEQ_INDEX, 0x0d);
		srd = INREG8(VGA_SEQ_DATA);

		switch (srd & 0x70) {
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
			TRACE(("Unknown DPMS mode read from device %X\n", srd));
		}
	} else if (si->displayType == MT_LCD || si->displayType == MT_DFP) {
		OUTREG8(VGA_SEQ_INDEX, 0x31); /* SR31 bit 4 - FP enable */
		srd = INREG8(VGA_SEQ_DATA);
		mode = ((srd & 0x10) ? B_DPMS_ON : B_DPMS_OFF);
	}

	TRACE(("DPMS_MODE = %d\n", mode));
	return mode;
}

