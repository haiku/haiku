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

#include <unistd.h>




static void SetClockRegisters(const DisplayModeEx& mode)
{
	SharedInfo& si = *gInfo.sharedInfo;
	M64_Params& params = si.m64Params;

	int p;
	int postDiv;
	bool extendedDiv = false;
	uint32 pixelClock = mode.timing.pixel_clock;

	if (pixelClock > params.maxPixelClock)
		pixelClock = params.maxPixelClock;

	double q = ((pixelClock / 10.0) * params.refDivider) / (2.0 * params.refFreq);

	if (si.chipType >= MACH64_264VTB) {
		if (q > 255) {
			TRACE("SetClockRegisters(): Warning: q > 255\n");
			q = 255;
			p = 0;
			postDiv = 1;
		} else if (q > 127.5) {
			p = 0;
			postDiv = 1;
		} else if (q > 85) {
			p = 1;
			postDiv = 2;
		} else if (q > 63.75) {
			p = 0;
			postDiv = 3;
			extendedDiv = true;
		} else if (q > 42.5) {
			p = 2;
			postDiv = 4;
		} else if (q > 31.875) {
			p = 2;
			postDiv = 6;
			extendedDiv = true;
		} else if (q > 21.25) {
			p = 3;
			postDiv = 8;
		} else if (q >= 10.6666666667) {
			p = 3;
			postDiv = 12;
			extendedDiv = true;
		} else {
			TRACE("SetClockRegisters(): Warning: q < 10.66666667\n");
			p = 3;
			postDiv = 12;
			extendedDiv = true;
		}
	} else {
		if (q > 255) {
			TRACE("SetClockRegisters(): Warning: q > 255\n");
			q = 255;
			p = 0;
		} else if (q > 127.5)
			p = 0;
		else if (q > 63.75)
			p = 1;
		else if (q > 31.875)
			p = 2;
		else if (q >= 16)
			p = 3;
		else {
			TRACE("SetClockRegisters(): Warning: q < 16\n");
			p = 3;
		}
		postDiv = 1 << p;
	}

	uint8 fbDiv = uint8(q * postDiv);

	// With some chips such as those with ID's 4750 & 475A, the display has
	// ripples when using resolution 1440x900 at 60 Hz refresh rate.
	// Decrementing fbDiv by 1 seems to fix this problem.

	if (mode.timing.h_display == 1440 && pixelClock < 108000)
		fbDiv--;

	int clkNum = params.clockNumberToProgram;

	OUTREG8(CLOCK_CNTL, clkNum | CLOCK_STROBE);

	// Temporarily switch to accelerator mode.
	uint32 crtc_gen_cntl = INREG(CRTC_GEN_CNTL);
	if (!(crtc_gen_cntl & CRTC_EXT_DISP_EN))
		OUTREG(CRTC_GEN_CNTL, crtc_gen_cntl | CRTC_EXT_DISP_EN);

	// Reset VCLK generator.
	uint8 vclkCntl = Mach64_GetPLLReg(PLL_VCLK_CNTL);
	Mach64_SetPLLReg(PLL_VCLK_CNTL, vclkCntl | PLL_VCLK_RESET);

	// Set post-divider.
	uint8 tmp = Mach64_GetPLLReg(PLL_VCLK_POST_DIV);
	Mach64_SetPLLReg(PLL_VCLK_POST_DIV,
		(tmp & ~(0x03 << (2 * clkNum))) | (p << (2 * clkNum)));

	// Set feedback divider.
	Mach64_SetPLLReg(PLL_VCLK0_FB_DIV + clkNum, fbDiv);

	// Set extended post-divider.
	if (si.chipType >= MACH64_264VTB) {
		tmp = Mach64_GetPLLReg(PLL_XCLK_CNTL);
		if (extendedDiv)
			Mach64_SetPLLReg(PLL_XCLK_CNTL, tmp | (0x10 << clkNum));
		else
			Mach64_SetPLLReg(PLL_XCLK_CNTL, tmp & ~(0x10 << clkNum));
	}

	// End VCLK generator reset.
	Mach64_SetPLLReg(PLL_VCLK_CNTL, vclkCntl & ~PLL_VCLK_RESET);

	snooze(5000);
	INREG8(DAC_W_INDEX);    // Clear DAC counter

	if (!(crtc_gen_cntl & CRTC_EXT_DISP_EN))
		OUTREG(CRTC_GEN_CNTL, crtc_gen_cntl);	// Restore register

	// Save parameters that will be used for computing the DSP parameters.

	params.vClkPostDivider = postDiv;
	params.vClkFeedbackDivider = fbDiv;

	return;
}


static void
SetDSPRegisters(const DisplayModeEx& mode)
{
	// Set up DSP register values for a VTB or later.

	SharedInfo& si = *gInfo.sharedInfo;
	M64_Params& params = si.m64Params;

#define Maximum_DSP_PRECISION	7

	uint8 mClkFeedbackDivider = Mach64_GetPLLReg(PLL_MCLK_FB_DIV);

	/* Compute a memory-to-screen bandwidth ratio */
	uint32 multiplier = uint32(mClkFeedbackDivider) * params.vClkPostDivider;
	uint32 divider = uint32(params.vClkFeedbackDivider) * params.xClkRefDivider;
	divider *= ((mode.bitsPerPixel + 1) / 4);

	// Start by assuming a display FIFO width of 64 bits.

	int vshift = (6 - 2) - params.xClkPostDivider;

	int RASMultiplier = params.xClkMaxRASDelay;
	int RASDivider = 1;

	// Determine dsp_precision first.

	int tmp = Mach64_Divide(multiplier * params.displayFIFODepth, divider, vshift, -1);
	int dsp_precision;

	for (dsp_precision = -5; tmp; dsp_precision++)
		tmp >>= 1;
	if (dsp_precision < 0)
		dsp_precision = 0;
	else if (dsp_precision > Maximum_DSP_PRECISION)
		dsp_precision = Maximum_DSP_PRECISION;

	int xshift = 6 - dsp_precision;
	vshift += xshift;

	int dsp_off = Mach64_Divide(multiplier * (params.displayFIFODepth - 1),
			divider, vshift, -1) - Mach64_Divide(1, 1, vshift - xshift, 1);

	int dsp_on = Mach64_Divide(multiplier, divider, vshift, 1);
	tmp = Mach64_Divide(RASMultiplier, RASDivider, xshift, 1);
	if (dsp_on < tmp)
		dsp_on = tmp;
	dsp_on += (tmp * 2) +
			  Mach64_Divide(params.xClkPageFaultDelay, 1, xshift, 1);

	/* Calculate rounding factor and apply it to dsp_on */
	tmp = ((1 << (Maximum_DSP_PRECISION - dsp_precision)) - 1) >> 1;
	dsp_on = ((dsp_on + tmp) / (tmp + 1)) * (tmp + 1);

	if (dsp_on >= ((dsp_off / (tmp + 1)) * (tmp + 1))) {
		dsp_on = dsp_off - Mach64_Divide(multiplier, divider, vshift, -1);
		dsp_on = (dsp_on / (tmp + 1)) * (tmp + 1);
	}

	int dsp_xclks = Mach64_Divide(multiplier, divider, vshift + 5, 1);

	// Build DSP register contents.

	uint32 dsp_on_off = SetBits(dsp_on, DSP_ON)
						| SetBits(dsp_off, DSP_OFF);
	uint32 dsp_config = SetBits(dsp_precision, DSP_PRECISION)
						| SetBits(dsp_xclks, DSP_XCLKS_PER_QW)
						| SetBits(params.displayLoopLatency, DSP_LOOP_LATENCY);

	OUTREG(DSP_ON_OFF, dsp_on_off);
	OUTREG(DSP_CONFIG, dsp_config);
}


static void
SetCrtcRegisters(const DisplayModeEx& mode)
{
	// Calculate the CRTC register values for requested video mode, and then set
	// set the registers to the calculated values.

	SharedInfo& si = *gInfo.sharedInfo;

	uint32 crtc_h_total_disp = ((mode.timing.h_total / 8) - 1)
							   | (((mode.timing.h_display / 8) - 1) << 16);

	int hSyncWidth = (mode.timing.h_sync_end - mode.timing.h_sync_start) / 8;
	if (hSyncWidth > 0x3f)
		hSyncWidth = 0x3f;

	int hSyncStart = mode.timing.h_sync_start / 8 - 1;

	uint32 crtc_h_sync_strt_wid = (hSyncWidth << 16)
		| (hSyncStart & 0xff) | ((hSyncStart & 0x100) << 4)
		| ((mode.timing.flags & B_POSITIVE_HSYNC) ? 0 : CRTC_H_SYNC_NEG);

	uint32 crtc_v_total_disp = ((mode.timing.v_total - 1)
		| ((mode.timing.v_display - 1) << 16));

	int vSyncWidth = mode.timing.v_sync_end - mode.timing.v_sync_start;
	if (vSyncWidth > 0x1f)
		vSyncWidth = 0x1f;

	uint32 crtc_v_sync_strt_wid = (mode.timing.v_sync_start - 1)
		| (vSyncWidth << 16)
		| ((mode.timing.flags & B_POSITIVE_VSYNC) ? 0 : CRTC_V_SYNC_NEG);

	uint32 crtc_off_pitch = SetBits(mode.timing.h_display >> 3, CRTC_PITCH);

	uint32 crtc_gen_cntl = INREG(CRTC_GEN_CNTL) &
		~(CRTC_DBL_SCAN_EN | CRTC_INTERLACE_EN |
		  CRTC_HSYNC_DIS | CRTC_VSYNC_DIS | CRTC_CSYNC_EN |
		  CRTC_PIX_BY_2_EN | CRTC_VGA_XOVERSCAN |
		  CRTC_PIX_WIDTH | CRTC_BYTE_PIX_ORDER |
		  CRTC_VGA_128KAP_PAGING | CRTC_VFC_SYNC_TRISTATE |
		  CRTC_LOCK_REGS |               // already off, but ...
		  CRTC_SYNC_TRISTATE | CRTC_DISP_REQ_EN |
		  CRTC_VGA_TEXT_132 | CRTC_CUR_B_TEST);

	crtc_gen_cntl |= CRTC_EXT_DISP_EN | CRTC_EN | CRTC_VGA_LINEAR | CRTC_CNT_EN;

	switch (mode.bitsPerPixel) {
	case 8:
		crtc_gen_cntl |= CRTC_PIX_WIDTH_8BPP;
		break;
	case 15:
		crtc_gen_cntl |= CRTC_PIX_WIDTH_15BPP;
		break;
	case 16:
		crtc_gen_cntl |= CRTC_PIX_WIDTH_16BPP;
		break;
	case 32:
		crtc_gen_cntl |= CRTC_PIX_WIDTH_32BPP;
		break;
	default:
		TRACE("Undefined color depth, bitsPerPixel: %d\n", mode.bitsPerPixel);
		break;
	}

	// For now, set display FIFO low water mark as high as possible.
	if (si.chipType < MACH64_264VTB)
		crtc_gen_cntl |= CRTC_FIFO_LWM;


	// Write the CRTC registers.
	//--------------------------

	// Stop CRTC.
	OUTREG(CRTC_GEN_CNTL, crtc_gen_cntl & ~(CRTC_EXT_DISP_EN | CRTC_EN));

	OUTREG(CRTC_H_TOTAL_DISP, crtc_h_total_disp);
	OUTREG(CRTC_H_SYNC_STRT_WID, crtc_h_sync_strt_wid);
	OUTREG(CRTC_V_TOTAL_DISP, crtc_v_total_disp);
	OUTREG(CRTC_V_SYNC_STRT_WID, crtc_v_sync_strt_wid);

	OUTREG(CRTC_OFF_PITCH, crtc_off_pitch);

	// Clear overscan registers.
	OUTREG(OVR_CLR, 0);
	OUTREG(OVR_WID_LEFT_RIGHT, 0);
	OUTREG(OVR_WID_TOP_BOTTOM, 0);

	// Finalise CRTC setup and turn on the screen.
	OUTREG(CRTC_GEN_CNTL, crtc_gen_cntl);

	return;
}



status_t
Mach64_SetDisplayMode(const DisplayModeEx& mode)
{
	// The code to actually configure the display.
	// All the error checking must be done in ProposeDisplayMode(),
	// and assume that the mode values we get here are acceptable.

	SharedInfo& si = *gInfo.sharedInfo;

	if (si.displayType == MT_VGA) {
		// Chip is connected to a monitor via a VGA connector.

		SetCrtcRegisters(mode);
		SetClockRegisters(mode);

		if (si.chipType >= MACH64_264VTB)
			SetDSPRegisters(mode);

	} else {
		// Chip is connected to a laptop LCD monitor; or via a DVI interface.

		uint16 vesaMode = GetVesaModeNumber(display_mode(mode), mode.bitsPerPixel);
		if (vesaMode == 0)
			return B_BAD_VALUE;

		status_t status = ioctl(gInfo.deviceFileDesc, ATI_SET_VESA_DISPLAY_MODE,
				&vesaMode, sizeof(vesaMode));
		if (status != B_OK)
			return status;
	}

	Mach64_AdjustFrame(mode);

	// Initialize the palette so that the various color depths will display
	// the correct colors.

	OUTREGM(DAC_CNTL, DAC_8BIT_EN, DAC_8BIT_EN);
	OUTREG8(DAC_MASK, 0xff);
	OUTREG8(DAC_W_INDEX, 0);		// initial color index

	for (int i = 0; i < 256; i++) {
		OUTREG8(DAC_DATA, i);
		OUTREG8(DAC_DATA, i);
		OUTREG8(DAC_DATA, i);
	}

	Mach64_EngineInit(mode);

	return B_OK;
}



void
Mach64_AdjustFrame(const DisplayModeEx& mode)
{
	// Adjust start address in frame buffer.

	SharedInfo& si = *gInfo.sharedInfo;

	int address = (mode.v_display_start * mode.virtual_width
			+ mode.h_display_start) * ((mode.bitsPerPixel + 1) / 8);

	address &= ~0x07;
	address += si.frameBufferOffset;

	OUTREGM(CRTC_OFF_PITCH, address, 0xfffff);
	return;
}


void
Mach64_SetIndexedColors(uint count, uint8 first, uint8* colorData, uint32 flags)
{
	// Set the indexed color palette for 8-bit color depth mode.

	(void)flags;		// avoid compiler warning for unused arg

	if (gInfo.sharedInfo->displayMode.space != B_CMAP8)
		return ;

	OUTREG8(DAC_MASK, 0xff);
	OUTREG8(DAC_W_INDEX, first);		// initial color index

	while (count--) {
		OUTREG8(DAC_DATA, colorData[0]);	// red
		OUTREG8(DAC_DATA, colorData[1]);	// green
		OUTREG8(DAC_DATA, colorData[2]);	// blue

		colorData += 3;
	}
}
