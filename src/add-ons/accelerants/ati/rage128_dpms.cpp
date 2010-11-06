/*
	Haiku ATI video driver adapted from the X.org ATI driver.

	Copyright 1999, 2000 ATI Technologies Inc., Markham, Ontario,
						 Precision Insight, Inc., Cedar Park, Texas, and
						 VA Linux Systems Inc., Fremont, California.

	Copyright 2009 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2009
*/


#include "accelerant.h"
#include "rage128.h"



uint32
Rage128_DPMSCapabilities(void)
{
	// Return DPMS modes supported by this device.

	return B_DPMS_ON | B_DPMS_STAND_BY | B_DPMS_SUSPEND | B_DPMS_OFF;
}


uint32
Rage128_GetDPMSMode(void)
{
	// Return the current DPMS mode.

	uint32 tmp = INREG(R128_CRTC_EXT_CNTL);
	uint32 mode;

	if( (tmp & R128_CRTC_DISPLAY_DIS) == 0 )
		mode = B_DPMS_ON;
	else if( (tmp & R128_CRTC_VSYNC_DIS) == 0 )
		mode = B_DPMS_STAND_BY;
	else if( (tmp & R128_CRTC_HSYNC_DIS) == 0 )
		mode = B_DPMS_SUSPEND;
	else
		mode = B_DPMS_OFF;

	TRACE("Rage128_DPMSMode() mode: %d\n", mode);
	return mode;
}


status_t
Rage128_SetDPMSMode(uint32 dpmsMode)
{
	// Set the display into one of the Display Power Management modes,
	// and return B_OK if successful, else return B_ERROR.

	SharedInfo& si = *gInfo.sharedInfo;

	TRACE("Rage128_SetDPMSMode() mode: %d, display type: %d\n", dpmsMode, si.displayType);

	int mask = (R128_CRTC_DISPLAY_DIS
				| R128_CRTC_HSYNC_DIS
				| R128_CRTC_VSYNC_DIS);

	switch (dpmsMode) {
		case B_DPMS_ON:
			// Screen: On; HSync: On, VSync: On.
			OUTREGM(R128_CRTC_EXT_CNTL, 0, mask);
			break;

		case B_DPMS_STAND_BY:
			// Screen: Off; HSync: Off, VSync: On.
			OUTREGM(R128_CRTC_EXT_CNTL,
				R128_CRTC_DISPLAY_DIS | R128_CRTC_HSYNC_DIS, mask);
			break;

		case B_DPMS_SUSPEND:
			// Screen: Off; HSync: On, VSync: Off.
			OUTREGM(R128_CRTC_EXT_CNTL,
				R128_CRTC_DISPLAY_DIS | R128_CRTC_VSYNC_DIS, mask);
			break;

		case B_DPMS_OFF:
			// Screen: Off; HSync: Off, VSync: Off.
			OUTREGM(R128_CRTC_EXT_CNTL, mask, mask);
			break;

		default:
			TRACE("Invalid DPMS mode %d\n", dpmsMode);
			return B_ERROR;
	}

	if (si.displayType == MT_LAPTOP) {
		uint32 genCtrl;

		switch (dpmsMode) {
			case B_DPMS_ON:
				genCtrl = INREG(R128_LVDS_GEN_CNTL);
				genCtrl |= R128_LVDS_ON | R128_LVDS_EN | R128_LVDS_BLON | R128_LVDS_DIGON;
				genCtrl &= ~R128_LVDS_DISPLAY_DIS;
				OUTREG(R128_LVDS_GEN_CNTL, genCtrl);
				break;

			case B_DPMS_STAND_BY:
			case B_DPMS_SUSPEND:
			case B_DPMS_OFF:
				genCtrl = INREG(R128_LVDS_GEN_CNTL);
				genCtrl |= R128_LVDS_DISPLAY_DIS;
				OUTREG(R128_LVDS_GEN_CNTL, genCtrl);
				snooze(10);
				genCtrl &= ~(R128_LVDS_ON | R128_LVDS_EN | R128_LVDS_BLON | R128_LVDS_DIGON);
				OUTREG(R128_LVDS_GEN_CNTL, genCtrl);
				break;

			default:
				TRACE("Invalid DPMS mode %d\n", dpmsMode);
				return B_ERROR;
		}
	}

	if (gInfo.sharedInfo->displayType == MT_DVI) {
		switch (dpmsMode) {
			case B_DPMS_ON:
				OUTREG(R128_FP_GEN_CNTL, INREG(R128_FP_GEN_CNTL)
					| (R128_FP_FPON | R128_FP_TDMS_EN));
				break;

			case B_DPMS_STAND_BY:
			case B_DPMS_SUSPEND:
			case B_DPMS_OFF:
				OUTREG(R128_FP_GEN_CNTL, INREG(R128_FP_GEN_CNTL)
					& ~(R128_FP_FPON | R128_FP_TDMS_EN));
				break;

			default:
				TRACE("Invalid DPMS mode %d\n", dpmsMode);
				return B_ERROR;
		}
	}

	return B_OK;
}

