/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_PLL_H
#define RADEON_HD_PLL_H


#define RHD_PLL_MIN_DEFAULT 16000
#define RHD_PLL_MAX_DEFAULT 400000
#define RHD_PLL_REFERENCE_DEFAULT 27000

// xorg default is 0x100000 which seems a little much.
#define PLL_CALIBRATE_WAIT 0x010000

/* limited by the number of bits available */
#define FB_DIV_LIMIT 2048
#define REF_DIV_LIMIT 1024
#define POST_DIV_LIMIT 128

// DCCGClk Operation Modes
#define RV620_DCCGCLK_RESET   0
#define RV620_DCCGCLK_GRAB    1
#define RV620_DCCGCLK_RELEASE 2


struct PLL_Control {
	uint16 feedbackDivider; // 0xFFFF is the endmarker
	uint32 control;
};


status_t PLLCalculate(uint32 pixelClock, uint16 *reference, uint16 *feedback,
	uint16 *post);
status_t PLLSet(uint8 pllIndex, uint32 pixelClock);
void PLLSetLowLegacy(uint8 pllIndex, uint32 pixelClock, uint16 reference,
	uint16 feedback, uint16 post);
void PLLSetLowR620(uint8 pllIndex, uint32 pixelClock, uint16 reference,
	uint16 feedback, uint16 post);
status_t PLLPower(uint8 pllIndex, int command);
status_t PLLCalibrate(uint8 pllIndex);
void PLLCRTCGrab(uint8 pllIndex, bool crt2);
bool DCCGCLKAvailable(uint8 pllIndex);
void DCCGCLKSet(uint8 pllIndex, int set);


#endif /* RADEON_HD_PLL_H */
