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

	for (i = 0; table[i].feedbackDivider < 0xFFFF ; i++)
		if (table[i].feedbackDivider >= feedbackDivider)
			break;

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

	uint16 pllControlReg = (pllIndex == 2) ? P2PLL_CNTL : P1PLL_CNTL;

	bool hasDccg = DCCGCLKAvailable(pllIndex);

	TRACE("%s: card has DCCG = %c\n", __func__, hasDccg ? 'y' : 'n');

	switch (command) {
		case RHD_POWER_ON:
		{
			TRACE("%s: PLL Power On\n", __func__);

			if (hasDccg)
				DCCGCLKSet(pllIndex, RV620_DCCGCLK_RESET);

			write32PLLAtMask(pllControlReg, 0, 0x02);
				// Power On
			snooze(2);
			PLLCalibrate(pllIndex);

			if (hasDccg)
				DCCGCLKSet(pllIndex, RV620_DCCGCLK_GRAB);

			return B_OK;
		}
		case RHD_POWER_RESET:
		{
			TRACE("%s: PLL Power Reset\n", __func__);

			if (hasDccg)
				DCCGCLKSet(pllIndex, RV620_DCCGCLK_RELEASE);

			write32PLLAtMask(pllControlReg, 0x01, 0x01);
				// Reset
			snooze(2);
			write32PLLAtMask(pllControlReg, 0, 0x02);
				// Power On
			snooze(2);
			return B_OK;
		}

		case RHD_POWER_SHUTDOWN:
		default:
			TRACE("%s: PLL Power Shutdown\n", __func__);

			if (hasDccg)
				DCCGCLKSet(pllIndex, RV620_DCCGCLK_RELEASE);

			write32PLLAtMask(pllControlReg, 0x01, 0x01);
				// Reset
			snooze(2);

			// Sometimes we have to keep an unused PLL running. Xorg Bug #18016
			if ((read32PLL(RV620_EXT1_DIFF_POST_DIV_CNTL)
				& RV62_EXT1_DIFF_DRIVER_ENABLE) == 0) {
				write32PLLAtMask(pllControlReg, 0x02, 0x02);
					// Power Down
			} else {
				TRACE("%s: PHYA differential clock driver not disabled\n",
					__func__);
			}

			snooze(200);

			write32PLLAtMask(pllControlReg,  0x2000, 0x2000);
				// Reset anti-glitch?
	}

	return B_OK;
}


status_t
PLLSet(uint8 pllIndex, uint32 pixelClock)
{

	TRACE("%s: enter to set pixel clock %d\n", __func__,
		(int)pixelClock);

	radeon_shared_info &info = *gInfo->shared_info;

	bool hasDccg = DCCGCLKAvailable(pllIndex);

	TRACE("%s: card has DCCG = %c\n", __func__, hasDccg ? 'y' : 'n');

	uint16 reference = 0;
	uint16 feedback = 0;
	uint16 post = 0;

	PLLCalculate(pixelClock, &reference, &feedback, &post);

	if (hasDccg)
		DCCGCLKSet(pllIndex, RV620_DCCGCLK_RESET);

	uint16 pllLockReg
		= (pllIndex == 2) ? EXT2_PPLL_UPDATE_LOCK : EXT1_PPLL_UPDATE_LOCK;
	uint16 pllControlReg = (pllIndex == 2) ? P2PLL_CNTL : P1PLL_CNTL;
	uint16 pllExtControlReg = (pllIndex == 2) ? EXT2_PPLL_CNTL : EXT1_PPLL_CNTL;
	uint16 pllDisplayClockControlReg
		= (pllIndex == 2) ? P2PLL_DISP_CLK_CNTL : P1PLL_DISP_CLK_CNTL;
	uint16 pllIntSSControlReg
		= (pllIndex == 2) ? P2PLL_INT_SS_CNTL : P1PLL_INT_SS_CNTL;
	uint16 pllReferenceDividerReg
		= (pllIndex == 2) ? EXT2_PPLL_REF_DIV : EXT1_PPLL_REF_DIV;
	uint16 pllFeedbackDividerReg
		= (pllIndex == 2) ? EXT2_PPLL_FB_DIV : EXT1_PPLL_FB_DIV;
	uint16 pllPostDividerReg
		= (pllIndex == 2) ? EXT2_PPLL_POST_DIV : EXT1_PPLL_POST_DIV;
	uint16 pplPostDividerSymReg
		= (pllIndex == 2) ? EXT2_SYM_PPLL_POST_DIV : EXT1_SYM_PPLL_POST_DIV;
	uint16 pllPostDividerSrcReg
		= (pllIndex == 2) ? EXT2_PPLL_POST_DIV_SRC : EXT1_PPLL_POST_DIV_SRC;

	write32PLLAtMask(pllIntSSControlReg, 0, 0x00000001);
		// Disable Spread Spectrum

	uint32 referenceDivider = reference;

	uint32 feedbackDivider = read32PLL(pllFeedbackDividerReg) & ~0x07FF003F;
	feedbackDivider |= ((feedback << 16) | 0x0030) & 0x07FF003F;

	uint32 postDivider = read32PLL(pllPostDividerReg) & ~0x0000007F;
	postDivider |= post & 0x0000007F;

	uint32 control;
	if (info.device_chipset >= (RADEON_R600 & 0x70))
		control = PLLControlTable(RV670PLLControl, feedback);
	else
		control = PLLControlTable(RV610PLLControl, feedback);

	uint8 symPostDiv = post & 0x0000007F;

	/* switch to external */
	write32PLL(pllPostDividerSrcReg, 0);
	write32PLLAtMask(pllDisplayClockControlReg, 0x00000200, 0x00000300);
	write32PLLAtMask(pllPostDividerReg, 0, 0x00000100);

	write32PLLAtMask(pllControlReg, 0x00000001, 0x00000001);
		// reset
	snooze(2);
	write32PLLAtMask(pllControlReg, 0x00000002, 0x00000002);
		// power down
	snooze(10);
	write32PLLAtMask(pllControlReg, 0x00002000, 0x00002000);
		// reset antiglitch

	write32PLL(pllExtControlReg, control);

	write32PLLAtMask(pllDisplayClockControlReg, 2, 0x0000003F);
		// Scalar Divider 2

	write32PLL(pllLockReg, 1);
		// Lock PLL

	/* Write PLL clocks */
	write32PLL(pllPostDividerSrcReg, 0x00000001);
	write32PLL(pllReferenceDividerReg, referenceDivider);
	write32PLL(pllFeedbackDividerReg, feedbackDivider);
	write32PLLAtMask(pllPostDividerReg, postDivider, 0x0000007F);
	write32PLLAtMask(pplPostDividerSymReg, symPostDiv, 0x0000007F);

	snooze(10);

	write32PLL(pllLockReg, 0);
		// Unlock PLL

	write32PLLAtMask(pllControlReg, 0, 0x00000002);
		// power up
	snooze(10);

	write32PLLAtMask(pllControlReg, 0, 0x00002000);
		// undo reset antiglitch

	PLLCalibrate(pllIndex);

	/* Switch back to PLL */
	write32PLLAtMask(pllDisplayClockControlReg, 0, 0x00000300);
	write32PLLAtMask(pplPostDividerSymReg, 0x00000100, 0x00000100);
	write32PLL(pllPostDividerSrcReg, 0x00000001);

	write32PLLAtMask(pllControlReg, 0, 0x80000000);
		// needed and undocumented

	// TODO : If CRT2 ah-la R500PLLCRTCGrab
	PLLCRTCGrab(pllIndex, false);

	if (hasDccg)
		DCCGCLKSet(pllIndex, RV620_DCCGCLK_GRAB);

	TRACE("%s: PLLSet exit\n", __func__);
	return B_OK;
}


status_t
PLLCalibrate(uint8 pllIndex)
{

	uint16 pllControlReg = (pllIndex == 2) ? P2PLL_CNTL : P1PLL_CNTL;

	write32PLLAtMask(pllControlReg, 1, 0x01);
		// PLL Reset

	snooze(2);

	write32PLLAtMask(pllControlReg, 0, 0x01);
		// PLL Set

	int i;

	for (i = 0; i < PLL_CALIBRATE_WAIT; i++)
		if (((read32PLL(pllControlReg) >> 20) & 0x03) == 0x03)
			break;

	if (i == PLL_CALIBRATE_WAIT) {
		if (read32PLL(pllControlReg) & 0x00100000) /* Calibration done? */
			TRACE("%s: Calibration Failed\n");
		if (read32PLL(pllControlReg) & 0x00200000) /* PLL locked? */
			TRACE("%s: Locking Failed\n");
	} else
		TRACE("%s: pll calibrated and locked in %d loops\n", __func__, i);

	return B_OK;
}


void
PLLCRTCGrab(uint8 pllIndex, bool crt2)
{
	bool pll2IsCurrent;

	if (!crt2) {
		pll2IsCurrent = read32PLL(PCLK_CRTC1_CNTL) & 0x00010000;

		write32PLLAtMask(PCLK_CRTC1_CNTL, (pllIndex == 2) ? 0x00010000 : 0,
			0x00010000);
	} else {
		pll2IsCurrent = read32PLL(PCLK_CRTC2_CNTL) & 0x00010000;

		write32PLLAtMask(PCLK_CRTC2_CNTL, (pllIndex == 2) ? 0x00010000 : 0,
			0x00010000);
	}

	/* if the current pll is not active, then poke it just enough to flip
	* owners */
	if (!pll2IsCurrent) {
		uint32 stored = read32PLL(P1PLL_CNTL);

		if (stored & 0x03) {
			write32PLLAtMask(P1PLL_CNTL, 0, 0x03);
			snooze(10);
			write32PLLAtMask(P1PLL_CNTL, stored, 0x03);
		}

	} else {
		uint32 stored = read32PLL(P2PLL_CNTL);

		if (stored & 0x03) {
			write32PLLAtMask(P2PLL_CNTL, 0, 0x03);
			snooze(10);
			write32PLLAtMask(P2PLL_CNTL, stored, 0x03);
		}
	}
}


// See if card has a DCCG available that we need to lock to
// the PLL clock. No one seems really sure what DCCG is.
bool
DCCGCLKAvailable(uint8 pllIndex)
{
	uint32 dccg = read32PLL(DCCG_DISP_CLK_SRCSEL) & 0x03;

	if (dccg & 0x02)
		return true;

	if ((pllIndex == 1) && (dccg == 0))
		return true;
	if ((pllIndex == 2) && (dccg == 1))
		return true;

	return false;
}


void
DCCGCLKSet(uint8 pllIndex, int set)
{
	uint32 buffer;

	switch(set) {
		case RV620_DCCGCLK_GRAB:
			if (pllIndex == 1)
				write32PLLAtMask(DCCG_DISP_CLK_SRCSEL, 0, 0x00000003);
			else if (pllIndex == 2)
				write32PLLAtMask(DCCG_DISP_CLK_SRCSEL, 1, 0x00000003);
			else
				write32PLLAtMask(DCCG_DISP_CLK_SRCSEL, 3, 0x00000003);
			break;
		case RV620_DCCGCLK_RELEASE:
			buffer = read32PLL(DCCG_DISP_CLK_SRCSEL) & 0x03;

			if ((pllIndex == 1) && (buffer == 0)) {
				/* set to other PLL or external */
				buffer = read32PLL(P2PLL_CNTL);
				// if powered and not in reset, and calibrated and locked
				if (!(buffer & 0x03) && ((buffer & 0x00300000) == 0x00300000))
					write32PLLAtMask(DCCG_DISP_CLK_SRCSEL, 1, 0x00000003);
				else
					write32PLLAtMask(DCCG_DISP_CLK_SRCSEL, 3, 0x00000003);

			} else if ((pllIndex == 2) && (buffer == 1)) {
				/* set to other PLL or external */
				buffer = read32PLL(P1PLL_CNTL);
				// if powered and not in reset, and calibrated and locked
				if (!(buffer & 0x03) && ((buffer & 0x00300000) == 0x00300000))
					write32PLLAtMask(DCCG_DISP_CLK_SRCSEL, 0, 0x00000003);
				else
					write32PLLAtMask(DCCG_DISP_CLK_SRCSEL, 3, 0x00000003);

			} // no other action needed
			break;
		case RV620_DCCGCLK_RESET:
			buffer = read32PLL(DCCG_DISP_CLK_SRCSEL) & 0x03;

			if (((pllIndex == 1) && (buffer == 0))
				|| ((pllIndex == 2) && (buffer == 1)))
				write32PLLAtMask(DCCG_DISP_CLK_SRCSEL, 3, 0x00000003);
			break;
		default:
			break;
	}
}


void
DACPower(uint8 dacIndex, int mode)
{
	uint32 dacOffset = (dacIndex == 2) ? REG_DACB_OFFSET : REG_DACA_OFFSET;
	uint32 powerdown;

	switch (mode) {
		case RHD_POWER_ON:
			// TODO : SensedType Detection?
			powerdown = 0;
			write32(dacOffset + DACA_ENABLE, 1);
			write32(dacOffset + DACA_POWERDOWN, 0);
			snooze(14);
			write32AtMask(dacOffset + DACA_POWERDOWN, powerdown, 0xFFFFFF00);
			snooze(2);
			write32(dacOffset + DACA_FORCE_OUTPUT_CNTL, 0);
			write32AtMask(dacOffset + DACA_SYNC_SELECT, 0, 0x00000101);
			write32(dacOffset + DACA_SYNC_TRISTATE_CONTROL, 0);
			return;
	}
}

