/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#include "accelerant_protos.h"
#include "accelerant.h"
#include "utility.h"
#include "pll.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define TRACE_PLL
#ifdef TRACE_PLL
extern "C" void _sPrintf(const char *format, ...);
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif


/* From hardcoded values. */
static struct PLL_Control RV610PLLControl[] =
{
	{ 0x0049, 0x159F8704 },
	{ 0x006C, 0x159B8704 },
	{ 0xFFFF, 0x159EC704 }
};

/* Some tables are provided by atombios,
 * it's just that they are hidden away deliberately and not exposed */
static struct PLL_Control RV670PLLControl[] =
{
	{ 0x004A, 0x159FC704 },
	{ 0x0067, 0x159BC704 },
	{ 0x00C4, 0x159EC704 },
	{ 0x00F4, 0x1593A704 },
	{ 0x0136, 0x1595A704 },
	{ 0x01A4, 0x1596A704 },
	{ 0x022C, 0x159CE504 },
	{ 0xFFFF, 0x1591E404 }
};


static uint32
PLLControlTable(struct PLL_Control *table, uint16 feedbackDivider)
{
	int i;

	for (i = 0; table[i].feedbackDivider < 0xFFFF ; i++) {
		if (table[i].feedbackDivider >= feedbackDivider)
			break;
	}

	return table[i].control;
}


status_t
PLLCalculate(uint32 pixelClock, uint16 *reference, uint16 *feedback,
	uint16 *post)
{
	// Freaking phase-locked loops, how do they work?

	float ratio = ((float) pixelClock)
		/ ((float) gInfo->shared_info->pll_info.reference_frequency);

	uint32 bestDiff = 0xFFFFFFFF;
	uint32 postDiv;
	uint32 referenceDiv;
	uint32 feedbackDiv;

	for (postDiv = 2; postDiv < POST_DIV_LIMIT; postDiv++) {
		uint32 vcoOut = pixelClock * postDiv;

		/* we are conservative and avoid the limits */
		if (vcoOut <= gInfo->shared_info->pll_info.min_frequency)
			continue;
		if (vcoOut >= gInfo->shared_info->pll_info.max_frequency)
			break;

		for (referenceDiv = 1; referenceDiv <= REF_DIV_LIMIT; referenceDiv++) {
			feedbackDiv = (uint32)((ratio * postDiv * referenceDiv) + 0.5);

			if (feedbackDiv >= FB_DIV_LIMIT)
				break;
			if (feedbackDiv > (500 + (13 * referenceDiv))) // rv6x0 limit
				break;

			uint32 diff = abs(pixelClock - (feedbackDiv
					* gInfo->shared_info->pll_info.reference_frequency)
						/ (postDiv * referenceDiv));

			if (diff < bestDiff) {
				*feedback = feedbackDiv;
				*reference = referenceDiv;
				*post = postDiv;
				bestDiff = diff;
			}

			if (bestDiff == 0)
				break;
		}

		if (bestDiff == 0)
			break;
	}

	if (bestDiff != 0xFFFFFFFF) {
		TRACE("%s: Successful PLL Calculation: %dkHz = "
			"(((%i / 0x%X) * 0x%X) / 0x%X) (%dkHz off)\n", __func__,
			(int) pixelClock,
			(unsigned int) gInfo->shared_info->pll_info.reference_frequency,
			*reference, *feedback, *post, (int) bestDiff);
			return B_OK;
	}

	// Shouldn't ever happen
	TRACE("%s: Failed to get a valid PLL setting for %dkHz\n",
		__func__, (int) pixelClock);
	return B_ERROR;
}


status_t
PLLPower(uint8 pllIndex, int command)
{
	uint16 pllControlReg = (pllIndex == 1) ? P2PLL_CNTL : P1PLL_CNTL;

	bool hasDccg = DCCGCLKAvailable(pllIndex);

	TRACE("%s: card has DCCG = %c\n", __func__, hasDccg ? 'y' : 'n');

	switch (command) {
		case RHD_POWER_ON:
		{
			TRACE("%s: PLL %d Power On\n", __func__, pllIndex);

			if (hasDccg)
				DCCGCLKSet(pllIndex, RV620_DCCGCLK_RESET);

			Write32Mask(PLL, pllControlReg, 0, 0x02);
				// Power On
			snooze(2);
			PLLCalibrate(pllIndex);

			if (hasDccg)
				DCCGCLKSet(pllIndex, RV620_DCCGCLK_GRAB);

			return B_OK;
		}
		case RHD_POWER_RESET:
		{
			TRACE("%s: PLL %d Power Reset\n", __func__, pllIndex);

			if (hasDccg)
				DCCGCLKSet(pllIndex, RV620_DCCGCLK_RELEASE);

			Write32Mask(PLL, pllControlReg, 0x01, 0x01);
				// Reset
			snooze(2);
			Write32Mask(PLL, pllControlReg, 0, 0x02);
				// Power On
			snooze(2);
			return B_OK;
		}

		case RHD_POWER_SHUTDOWN:
		default:
			TRACE("%s: PLL %d Power Shutdown\n", __func__, pllIndex);

			radeon_shared_info &info = *gInfo->shared_info;

			if (hasDccg)
				DCCGCLKSet(pllIndex, RV620_DCCGCLK_RELEASE);

			Write32Mask(PLL, pllControlReg, 0x01, 0x01);
				// Reset
			snooze(2);

			if (info.device_chipset >= (RADEON_R600 | 0x20)) {
				uint16 pllDiffPostReg
					= (pllIndex == 1) ? RV620_EXT2_DIFF_POST_DIV_CNTL
						: RV620_EXT1_DIFF_POST_DIV_CNTL;
				uint16 pllDiffDriverEnable
					= (pllIndex == 1) ? (uint16)RV62_EXT2_DIFF_DRIVER_ENABLE
						: (uint16)RV62_EXT1_DIFF_DRIVER_ENABLE;

				// Sometimes we have to keep an unused PLL running. X Bug #18016
				if ((Read32(PLL, pllDiffPostReg)
					& pllDiffDriverEnable) == 0) {
					Write32Mask(PLL, pllControlReg, 0x02, 0x02);
						// Power Down
				} else {
					TRACE("%s: PHYA differential clock driver not disabled\n",
						__func__);
				}

				snooze(200);

				Write32Mask(PLL, pllControlReg,  0x2000, 0x2000);
					// Reset anti-glitch?

			} else {
				Write32Mask(PLL, pllControlReg, 0x02, 0x02);
					// Power Down
				snooze(200);
			}
	}

	return B_OK;
}


status_t
PLLSet(uint8 pllIndex, uint32 pixelClock)
{
	radeon_shared_info &info = *gInfo->shared_info;

	uint16 reference = 0;
	uint16 feedback = 0;
	uint16 post = 0;

	PLLCalculate(pixelClock, &reference, &feedback, &post);

	if (info.device_chipset >= (RADEON_R600 | 0x20)) {
		TRACE("%s : setting pixel clock %d on r620+\n", __func__,
			(int)pixelClock);
		PLLSetLowR620(pllIndex, pixelClock, reference,
			feedback, post);
	} else if (info.device_chipset < (RADEON_R600 | 0x20)) {
		TRACE("%s : setting pixel clock %d on r600-r610\n", __func__,
			(int)pixelClock);
		PLLSetLowLegacy(pllIndex, pixelClock, reference,
			feedback, post);
	}

	return B_OK;
}


void
PLLSetLowLegacy(uint8 pllIndex, uint32 pixelClock, uint16 reference,
	uint16 feedback, uint16 post)
{
	uint32 feedbackTemp = feedback << 16;
	uint32 referenceTemp = reference;

	/* Internal PLL Registers */
	uint16 pllCntl = (pllIndex == 1) ? P2PLL_CNTL : P1PLL_CNTL;
	uint16 pllIntSSCntl
		= (pllIndex == 1) ? P2PLL_INT_SS_CNTL : P1PLL_INT_SS_CNTL;

	/* External PLL Registers */
	uint16 pllExtCntl
		= (pllIndex == 1) ? EXT2_PPLL_CNTL : EXT1_PPLL_CNTL;
	uint16 pllExtUpdateCntl
		= (pllIndex == 1) ? EXT2_PPLL_UPDATE_CNTL : EXT1_PPLL_UPDATE_CNTL;
	uint16 pllExtUpdateLock
		= (pllIndex == 1) ? EXT2_PPLL_UPDATE_LOCK : EXT1_PPLL_UPDATE_LOCK;
	uint16 pllExtPostDiv
		= (pllIndex == 1) ? EXT2_PPLL_POST_DIV : EXT1_PPLL_POST_DIV;
	uint16 pllExtPostDivSrc
		= (pllIndex == 1) ? EXT2_PPLL_POST_DIV_SRC : EXT1_PPLL_POST_DIV_SRC;
	uint16 pllExtFeedbackDiv
		= (pllIndex == 1) ? EXT2_PPLL_FB_DIV : EXT1_PPLL_FB_DIV;
	uint16 pllExtRefDiv
		= (pllIndex == 1) ? EXT2_PPLL_REF_DIV : EXT1_PPLL_REF_DIV;
	uint16 pllExtRefDivSrc
		= (pllIndex == 1) ? EXT2_PPLL_REF_DIV_SRC : EXT1_PPLL_REF_DIV_SRC;

	radeon_shared_info &info = *gInfo->shared_info;

	if (info.device_chipset <= RADEON_R600)
		feedbackTemp |= 0x00000030;
	else {
		if (feedback <= 0x24)
			feedbackTemp |= 0x00000030;
		else if (feedback <= 0x3F)
			feedbackTemp |= 0x00000020;
	}

	uint32 postTemp = Read32(PLL, pllExtPostDiv) & ~0x0000007F;
	postTemp |= post & 0x0000007F;

	uint32 control;
	if (info.device_chipset == RADEON_R600)
		control = 0x01130704;
	else {
		control = PLLControlTable(RV610PLLControl, feedback);
		if (!control)
			control = Read32(PLL, pllExtCntl);
	}

	Write32Mask(PLL, pllIntSSCntl, 0, 0x00000001);
		// Disable Spread Spectrum

	Write32(PLL, pllExtRefDivSrc, 0x01); /* XTAL */
	Write32(PLL, pllExtPostDivSrc, 0x00); /* source = reference */

	Write32(PLL, pllExtUpdateLock, 0x01); /* lock */

	Write32(PLL, pllExtRefDiv, referenceTemp);
	Write32(PLL, pllExtFeedbackDiv, feedbackTemp);
	Write32(PLL, pllExtPostDiv, postTemp);
	Write32(PLL, pllExtCntl, control);

	Write32Mask(PLL, pllExtUpdateCntl, 0x00010000, 0x00010000);
		// No autoreset
	Write32Mask(PLL, pllCntl, 0, 0x04);
		// Don't bypass calibration

	/* We need to reset the anti glitch logic */
	Write32Mask(PLL, pllCntl, 0, 0x00000002);
		// Power up

	/* reset anti glitch logic */
	Write32Mask(PLL, pllCntl, 0x00002000, 0x00002000);
	snooze(2);
	Write32Mask(PLL, pllCntl, 0, 0x00002000);

	/* powerdown and reset */
	Write32Mask(PLL, pllCntl, 0x00000003, 0x00000003);
	snooze(2);

	Write32(PLL, pllExtUpdateLock, 0);
		// Unlock
	Write32Mask(PLL, pllExtUpdateCntl, 0, 0x01);
		// Done updating

	Write32Mask(PLL, pllCntl, 0, 0x02);
		// Power up PLL
	snooze(2);

	PLLCalibrate(pllIndex);

	Write32(PLL, pllExtPostDivSrc, 0x01);
		// Set source as PLL

	// TODO : better way to grab crt to work on?
	PLLCRTCGrab(pllIndex, gRegister->crtid);
}


void
PLLSetLowR620(uint8 pllIndex, uint32 pixelClock, uint16 reference,
	uint16 feedback, uint16 post)
{
	radeon_shared_info &info = *gInfo->shared_info;

	bool hasDccg = DCCGCLKAvailable(pllIndex);

	TRACE("%s: card has DCCG = %c\n", __func__, hasDccg ? 'y' : 'n');

	if (hasDccg)
		DCCGCLKSet(pllIndex, RV620_DCCGCLK_RESET);

	/* Internal PLL Registers */
	uint16 pllCntl = (pllIndex == 1) ? P2PLL_CNTL : P1PLL_CNTL;
	uint16 pllIntSSCntl
		= (pllIndex == 1) ? P2PLL_INT_SS_CNTL : P1PLL_INT_SS_CNTL;

	/* External PLL Registers */
	uint16 pllExtCntl
		= (pllIndex == 1) ? EXT2_PPLL_CNTL : EXT1_PPLL_CNTL;
	//uint16 pllExtUpdateCntl
	//	= (pllIndex == 1) ? EXT2_PPLL_UPDATE_CNTL : EXT1_PPLL_UPDATE_CNTL;
	uint16 pllExtUpdateLock
		= (pllIndex == 1) ? EXT2_PPLL_UPDATE_LOCK : EXT1_PPLL_UPDATE_LOCK;
	uint16 pllExtPostDiv
		= (pllIndex == 1) ? EXT2_PPLL_POST_DIV : EXT1_PPLL_POST_DIV;
	uint16 pllExtPostDivSrc
		= (pllIndex == 1) ? EXT2_PPLL_POST_DIV_SRC : EXT1_PPLL_POST_DIV_SRC;
	uint16 pllExtPostDivSym
		= (pllIndex == 1) ? EXT2_SYM_PPLL_POST_DIV : EXT1_SYM_PPLL_POST_DIV;
	uint16 pllExtFeedbackDiv
		= (pllIndex == 1) ? EXT2_PPLL_FB_DIV : EXT1_PPLL_FB_DIV;
	uint16 pllExtRefDiv
		= (pllIndex == 1) ? EXT2_PPLL_REF_DIV : EXT1_PPLL_REF_DIV;
	//uint16 pllExtRefDivSrc
	//	= (pllIndex == 1) ? EXT2_PPLL_REF_DIV_SRC : EXT1_PPLL_REF_DIV_SRC;
	uint16 pllExtDispClkCntl
		= (pllIndex == 1) ? P2PLL_DISP_CLK_CNTL : P1PLL_DISP_CLK_CNTL;

	Write32Mask(PLL, pllIntSSCntl, 0, 0x00000001);
		// Disable Spread Spectrum

	uint32 referenceDivider = reference;

	uint32 feedbackDivider = Read32(PLL, pllExtFeedbackDiv) & ~0x07FF003F;
	feedbackDivider |= ((feedback << 16) | 0x0030) & 0x07FF003F;

	uint32 postDivider = Read32(PLL, pllExtPostDiv) & ~0x0000007F;
	postDivider |= post & 0x0000007F;

	uint32 control;

	if (info.device_chipset >= (RADEON_R600 | 0x70))
		control = PLLControlTable(RV670PLLControl, feedback);
	else
		control = PLLControlTable(RV610PLLControl, feedback);

	uint8 symPostDiv = post & 0x0000007F;

	/* switch to external */
	Write32(PLL, pllExtPostDivSrc, 0);
	Write32Mask(PLL, pllExtDispClkCntl, 0x00000200, 0x00000300);
	Write32Mask(PLL, pllExtPostDiv, 0, 0x00000100);

	Write32Mask(PLL, pllCntl, 0x00000001, 0x00000001);
		// reset
	snooze(2);
	Write32Mask(PLL, pllCntl, 0x00000002, 0x00000002);
		// power down
	snooze(10);
	Write32Mask(PLL, pllCntl, 0x00002000, 0x00002000);
		// reset antiglitch

	Write32(PLL, pllExtCntl, control);

	Write32Mask(PLL, pllExtDispClkCntl, 2, 0x0000003F);
		// Scalar Divider 2

	Write32(PLL, pllExtUpdateLock, 1);
		// Lock PLL

	/* Write PLL clocks */
	Write32(PLL, pllExtPostDivSrc, 0x00000001);
	Write32(PLL, pllExtRefDiv, referenceDivider);
	Write32(PLL, pllExtFeedbackDiv, feedbackDivider);
	Write32Mask(PLL, pllExtPostDiv, postDivider, 0x0000007F);
	Write32Mask(PLL, pllExtPostDivSym, symPostDiv, 0x0000007F);

	snooze(10);

	Write32(PLL, pllExtUpdateLock, 0);
		// Unlock PLL

	Write32Mask(PLL, pllCntl, 0, 0x00000002);
		// power up
	snooze(10);

	Write32Mask(PLL, pllCntl, 0, 0x00002000);
		// undo reset antiglitch

	PLLCalibrate(pllIndex);

	/* Switch back to PLL */
	Write32Mask(PLL, pllExtDispClkCntl, 0, 0x00000300);
	Write32Mask(PLL, pllExtPostDivSym, 0x00000100, 0x00000100);
	Write32(PLL, pllExtPostDivSrc, 0x00000001);

	Write32Mask(PLL, pllCntl, 0, 0x80000000);
		// needed and undocumented

	// TODO : better way to grab crt to work on?
	PLLCRTCGrab(pllIndex, gRegister->crtid);

	if (hasDccg)
		DCCGCLKSet(pllIndex, RV620_DCCGCLK_GRAB);

	TRACE("%s: PLLSet exit\n", __func__);
}


status_t
PLLCalibrate(uint8 pllIndex)
{
	uint16 pllControlReg = (pllIndex == 1) ? P2PLL_CNTL : P1PLL_CNTL;

	Write32Mask(PLL, pllControlReg, 1, 0x01);
		// PLL Reset

	snooze(2);

	Write32Mask(PLL, pllControlReg, 0, 0x01);
		// PLL Set

	int i;

	for (i = 0; i < PLL_CALIBRATE_WAIT; i++) {
		if (((Read32(PLL, pllControlReg) >> 20) & 0x03) == 0x03)
			break;
	}

	if (i >= PLL_CALIBRATE_WAIT) {
		if (Read32(PLL, pllControlReg) & 0x00100000) /* Calibration done? */
			TRACE("%s: Calibration Failed\n", __func__);
		if (Read32(PLL, pllControlReg) & 0x00200000) /* PLL locked? */
			TRACE("%s: Locking Failed\n", __func__);
		TRACE("%s: We encountered a problem calibrating the PLL.\n", __func__);
		return B_ERROR;
	} else
		TRACE("%s: pll calibrated and locked in %d loops\n", __func__, i);

	return B_OK;
}


void
PLLCRTCGrab(uint8 pllIndex, uint8 crtid)
{
	bool pll2IsCurrent;

	if (crtid == 0) {
		pll2IsCurrent = Read32(PLL, PCLK_CRTC1_CNTL) & 0x00010000;

		Write32Mask(PLL, PCLK_CRTC1_CNTL, (pllIndex == 0) ? 0x00010000 : 0,
			0x00010000);
	} else {
		pll2IsCurrent = Read32(PLL, PCLK_CRTC2_CNTL) & 0x00010000;

		Write32Mask(PLL, PCLK_CRTC2_CNTL, (pllIndex == 0) ? 0x00010000 : 0,
			0x00010000);
	}

	/* if the current pll is not active, then poke it just enough to flip
	* owners */
	if (!pll2IsCurrent) {
		uint32 stored = Read32(PLL, P1PLL_CNTL);

		if (stored & 0x03) {
			Write32Mask(PLL, P1PLL_CNTL, 0, 0x03);
			snooze(10);
			Write32Mask(PLL, P1PLL_CNTL, stored, 0x03);
		}

	} else {
		uint32 stored = Read32(PLL, P2PLL_CNTL);

		if (stored & 0x03) {
			Write32Mask(PLL, P2PLL_CNTL, 0, 0x03);
			snooze(10);
			Write32Mask(PLL, P2PLL_CNTL, stored, 0x03);
		}
	}
}


// See if card has a DCCG available that we need to lock to
// the PLL clock. No one seems really sure what DCCG is.
bool
DCCGCLKAvailable(uint8 pllIndex)
{
	radeon_shared_info &info = *gInfo->shared_info;

	if (info.device_chipset < (RADEON_R600 | 0x20))
		return false;

	uint32 dccg = Read32(PLL, DCCG_DISP_CLK_SRCSEL) & 0x03;

	if (dccg & 0x02)
		return true;

	if ((pllIndex == 0) && (dccg == 0))
		return true;
	if ((pllIndex == 1) && (dccg == 1))
		return true;

	return false;
}


void
DCCGCLKSet(uint8 pllIndex, int set)
{
	uint32 buffer;

	switch(set) {
		case RV620_DCCGCLK_GRAB:
			if (pllIndex == 0)
				Write32Mask(PLL, DCCG_DISP_CLK_SRCSEL, 0, 0x00000003);
			else if (pllIndex == 1)
				Write32Mask(PLL, DCCG_DISP_CLK_SRCSEL, 1, 0x00000003);
			else
				Write32Mask(PLL, DCCG_DISP_CLK_SRCSEL, 3, 0x00000003);
			break;
		case RV620_DCCGCLK_RELEASE:
			buffer = Read32(PLL, DCCG_DISP_CLK_SRCSEL) & 0x03;

			if ((pllIndex == 0) && (buffer == 0)) {
				/* set to other PLL or external */
				buffer = Read32(PLL, P2PLL_CNTL);
				// if powered and not in reset, and calibrated and locked
				if (!(buffer & 0x03) && ((buffer & 0x00300000) == 0x00300000))
					Write32Mask(PLL, DCCG_DISP_CLK_SRCSEL, 1, 0x00000003);
				else
					Write32Mask(PLL, DCCG_DISP_CLK_SRCSEL, 3, 0x00000003);

			} else if ((pllIndex == 1) && (buffer == 1)) {
				/* set to other PLL or external */
				buffer = Read32(PLL, P1PLL_CNTL);
				// if powered and not in reset, and calibrated and locked
				if (!(buffer & 0x03) && ((buffer & 0x00300000) == 0x00300000))
					Write32Mask(PLL, DCCG_DISP_CLK_SRCSEL, 0, 0x00000003);
				else
					Write32Mask(PLL, DCCG_DISP_CLK_SRCSEL, 3, 0x00000003);

			} // no other action needed
			break;
		case RV620_DCCGCLK_RESET:
			buffer = Read32(PLL, DCCG_DISP_CLK_SRCSEL) & 0x03;

			if (((pllIndex == 0) && (buffer == 0))
				|| ((pllIndex == 1) && (buffer == 1)))
				Write32Mask(PLL, DCCG_DISP_CLK_SRCSEL, 3, 0x00000003);
			break;
		default:
			break;
	}
}

