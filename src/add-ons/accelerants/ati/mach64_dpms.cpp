/*
	Haiku ATI video driver adapted from the X.org ATI driver.

	Copyright 1992,1993,1994,1995,1996,1997 by Kevin E. Martin, Chapel Hill, North Carolina.
	Copyright 1997 through 2004 by Marc Aurele La France (TSI @ UQV), tsi@xfree86.org

	Copyright 2009 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2009
*/


#include "accelerant.h"
#include "mach64.h"



uint32
Mach64_DPMSCapabilities(void)
{
	// Return DPMS modes supported by this device.

	return B_DPMS_ON | B_DPMS_STAND_BY | B_DPMS_SUSPEND | B_DPMS_OFF;
}


uint32
Mach64_GetDPMSMode(void)
{
	// Return the current DPMS mode.

	uint32 tmp = INREG(CRTC_GEN_CNTL);
	uint32 mode;

	if( (tmp & CRTC_DISPLAY_DIS) == 0 )
		mode = B_DPMS_ON;
	else if( (tmp & CRTC_VSYNC_DIS) == 0 )
		mode = B_DPMS_STAND_BY;
	else if( (tmp & CRTC_HSYNC_DIS) == 0 )
		mode = B_DPMS_SUSPEND;
	else
		mode = B_DPMS_OFF;

	TRACE("Mach64_DPMSMode() mode: %d\n", mode);
	return mode;
}


status_t
Mach64_SetDPMSMode(uint32 dpmsMode)
{
	// Set the display into one of the Display Power Management modes,
	// and return B_OK if successful, else return B_ERROR.

	TRACE("Mach64_SetDPMSMode() mode: %d, display type: %d\n", dpmsMode, gInfo.sharedInfo->displayType);

	int mask = (CRTC_HSYNC_DIS | CRTC_VSYNC_DIS);

	switch (dpmsMode) {
		case B_DPMS_ON:
			// Screen: On; HSync: On, VSync: On.
			OUTREGM(CRTC_GEN_CNTL, 0, mask);
			break;

		case B_DPMS_STAND_BY:
			// Screen: Off; HSync: Off, VSync: On.
			OUTREGM(CRTC_GEN_CNTL, CRTC_HSYNC_DIS, mask);
			break;

		case B_DPMS_SUSPEND:
			// Screen: Off; HSync: On, VSync: Off.
			OUTREGM(CRTC_GEN_CNTL, CRTC_VSYNC_DIS, mask);
			break;

		case B_DPMS_OFF:
			// Screen: Off; HSync: Off, VSync: Off.
			OUTREGM(CRTC_GEN_CNTL, mask, mask);
			break;

		default:
			TRACE("Invalid DPMS mode %d\n", dpmsMode);
			return B_ERROR;
	}

	if (gInfo.sharedInfo->displayType == MT_LAPTOP) {
		uint32 powerMgmt = (Mach64_GetLCDReg(LCD_POWER_MANAGEMENT)
			& ~(STANDBY_NOW | SUSPEND_NOW | POWER_BLON | AUTO_POWER_UP));

		switch (dpmsMode) {
		case B_DPMS_ON:
			powerMgmt |= (POWER_BLON | AUTO_POWER_UP);
			break;

		case B_DPMS_STAND_BY:
			powerMgmt |= STANDBY_NOW;
			break;

		case B_DPMS_SUSPEND:
			powerMgmt |= SUSPEND_NOW;
			break;

		case B_DPMS_OFF:
			powerMgmt |= STANDBY_NOW | SUSPEND_NOW;
			break;
		}

		Mach64_PutLCDReg(LCD_POWER_MANAGEMENT, powerMgmt);
	}

	return B_OK;
}

