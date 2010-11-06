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

#include <unistd.h>




struct DisplayParams {
	// CRTC registers
	uint32	crtc_gen_cntl;
	uint32	crtc_h_total_disp;
	uint32	crtc_h_sync_strt_wid;
	uint32	crtc_v_total_disp;
	uint32	crtc_v_sync_strt_wid;
	uint32	crtc_pitch;

	// DDA register
	uint32	dda_config;
	uint32	dda_on_off;

	// Computed PLL values
	int		feedback_div;
	int		post_div;

	// PLL registers
	uint32	ppll_ref_div;
	uint32	ppll_div_3;
};



static inline int
DivideWithRounding(int n, int d)
{
	return (n + (d / 2)) / d;		// compute n/d with rounding
}


static int
MinimumBits(uint32 value)
{
	// Compute minimum number of bits required to contain a value (ie, log
	// base 2 of value).

	if (value == 0)
		return 1;

	int numBits = 0;

	while (value != 0) {
		value >>= 1;
		numBits++;
	}

	return numBits;
}


static bool
CalculateCrtcRegisters(const DisplayModeEx& mode, DisplayParams& params)
{
	// Define CRTC registers for requested video mode.
	// Return true if successful.

	const uint8 hSyncFudge[] = { 0x00, 0x12, 0x09, 0x09, 0x06, 0x05 };

	uint32 format;

	switch (mode.bitsPerPixel) {
	case 8:
		format = 2;
		break;
	case 15:
		format = 3;		// 555
		break;
	case 16:
		format = 4;		// 565
		break;
	case 32:
		format = 6;		// xRGB
		break;
	default:
		TRACE("Unsupported color depth: %d bits/pixel\n", mode.bitsPerPixel);
		return false;
	}

	params.crtc_gen_cntl = (R128_CRTC_EXT_DISP_EN
						   | R128_CRTC_EN
						   | (format << 8));

	params.crtc_h_total_disp = (((mode.timing.h_total / 8) - 1) & 0xffff)
							   | (((mode.timing.h_display / 8) - 1) << 16);

	int hSyncWidth = (mode.timing.h_sync_end - mode.timing.h_sync_start) / 8;
	if (hSyncWidth <= 0)
		hSyncWidth = 1;
	if (hSyncWidth > 0x3f)
		hSyncWidth = 0x3f;

	int hSyncStart = mode.timing.h_sync_start - 8 + hSyncFudge[format - 1];

	params.crtc_h_sync_strt_wid = (hSyncStart & 0xfff) | (hSyncWidth << 16)
		| ((mode.timing.flags & B_POSITIVE_HSYNC) ? 0 : R128_CRTC_H_SYNC_POL);

	params.crtc_v_total_disp = (((mode.timing.v_total - 1) & 0xffff)
		| ((mode.timing.v_display - 1) << 16));

	int vSyncWidth = mode.timing.v_sync_end - mode.timing.v_sync_start;
	if (vSyncWidth <= 0)
		vSyncWidth = 1;
	if (vSyncWidth > 0x1f)
		vSyncWidth = 0x1f;

	params.crtc_v_sync_strt_wid = ((mode.timing.v_sync_start - 1) & 0xfff)
		| (vSyncWidth << 16)
		| ((mode.timing.flags & B_POSITIVE_VSYNC) ? 0 : R128_CRTC_V_SYNC_POL);

	params.crtc_pitch = mode.timing.h_display / 8;

	return true;
}


static bool
CalculateDDARegisters(const DisplayModeEx& mode, DisplayParams& params)
{
	// Compute and write DDA registers for requested video mode.
	// Return true if successful.

	SharedInfo& si = *gInfo.sharedInfo;
	R128_RAMSpec& memSpec = si.r128MemSpec;
	R128_PLLParams& pll = si.r128PLLParams;

	int displayFifoWidth = 128;
	int displayFifoDepth = 32;
	int xClkFreq = pll.xclk;

	int vClkFreq = DivideWithRounding(pll.reference_freq * params.feedback_div,
		pll.reference_div * params.post_div);

	int bytesPerPixel = (mode.bitsPerPixel + 7) / 8;

	int xClksPerTransfer = DivideWithRounding(xClkFreq * displayFifoWidth,
		vClkFreq * bytesPerPixel * 8);

	int useablePrecision = MinimumBits(xClksPerTransfer) + 1;

	int xClksPerTransferPrecise = DivideWithRounding(
		(xClkFreq * displayFifoWidth) << (11 - useablePrecision),
		vClkFreq * bytesPerPixel * 8);

	int rOff = xClksPerTransferPrecise * (displayFifoDepth - 4);

	int rOn = (4 * memSpec.memBurstLen
		+ 3 * MAX(memSpec.rasToCasDelay - 2, 0)
		+ 2 * memSpec.rasPercentage
		+ memSpec.writeRecovery
		+ memSpec.casLatency
		+ memSpec.readToWriteDelay
		+ xClksPerTransfer) << (11 - useablePrecision);

	if (rOn + memSpec.loopLatency >= rOff) {
		TRACE("Error:  (rOn = %d) + (loopLatency = %d) >= (rOff = %d)\n",
			rOn, memSpec.loopLatency, rOff);
		return false;
	}

	params.dda_config = xClksPerTransferPrecise | (useablePrecision << 16)
			| (memSpec.loopLatency << 20);
	params.dda_on_off = (rOn << 16) | rOff;

	return true;
}


static bool
CalculatePLLRegisters(const DisplayModeEx& mode, DisplayParams& params)
{
	// Define PLL registers for requested video mode.

	struct Divider {
		int divider;
		int bitValue;
	};

	// The following data is from RAGE 128 VR/RAGE 128 GL Register Reference
	// Manual (Technical Reference Manual P/N RRG-G04100-C Rev. 0.04), page
	// 3-17 (PLL_DIV_[3:0]).

	const Divider postDividers[] = {
		{ 1, 0 },		// VCLK_SRC
		{ 2, 1 },		// VCLK_SRC/2
		{ 4, 2 },		// VCLK_SRC/4
		{ 8, 3 },		// VCLK_SRC/8
		{ 3, 4 },		// VCLK_SRC/3
						// bitValue = 5 is reserved
		{ 6, 6 },		// VCLK_SRC/6
		{ 12, 7 }		// VCLK_SRC/12
	};

	R128_PLLParams& pll = gInfo.sharedInfo->r128PLLParams;
	uint32 freq = mode.timing.pixel_clock / 10;

	if (freq > pll.max_pll_freq)
		freq = pll.max_pll_freq;
	if (freq * 12 < pll.min_pll_freq)
		freq = pll.min_pll_freq / 12;

	int bitValue = -1;
	uint32 output_freq;

	for (int j = 0; j < ARRAY_SIZE(postDividers); j++) {
		output_freq = postDividers[j].divider * freq;
		if (output_freq >= pll.min_pll_freq && output_freq <= pll.max_pll_freq) {
			params.feedback_div = DivideWithRounding(pll.reference_div * output_freq,
				pll.reference_freq);
			params.post_div = postDividers[j].divider;
			bitValue = postDividers[j].bitValue;
			break;
		}
	}

	if (bitValue < 0) {
		TRACE("CalculatePLLRegisters(), acceptable divider not found\n");
		return false;
	}

	params.ppll_ref_div = pll.reference_div;
	params.ppll_div_3 = (params.feedback_div | (bitValue << 16));

	return true;
}


static void
PLLWaitForReadUpdateComplete()
{
	while (GetPLLReg(R128_PPLL_REF_DIV) & R128_PPLL_ATOMIC_UPDATE_R)
		;
}

static void
PLLWriteUpdate()
{
	PLLWaitForReadUpdateComplete();

	SetPLLReg(R128_PPLL_REF_DIV, R128_PPLL_ATOMIC_UPDATE_W, R128_PPLL_ATOMIC_UPDATE_W);
}


static void
SetRegisters(DisplayParams& params)
{
	// Write the common registers (most will be set to zero).
	//-------------------------------------------------------

	OUTREGM(R128_FP_GEN_CNTL, R128_FP_BLANK_DIS, R128_FP_BLANK_DIS);

	OUTREG(R128_OVR_CLR, 0);
	OUTREG(R128_OVR_WID_LEFT_RIGHT, 0);
	OUTREG(R128_OVR_WID_TOP_BOTTOM, 0);
	OUTREG(R128_OV0_SCALE_CNTL, 0);
	OUTREG(R128_MPP_TB_CONFIG, 0);
	OUTREG(R128_MPP_GP_CONFIG, 0);
	OUTREG(R128_SUBPIC_CNTL, 0);
	OUTREG(R128_VIPH_CONTROL, 0);
	OUTREG(R128_I2C_CNTL_1, 0);
	OUTREG(R128_GEN_INT_CNTL, 0);
	OUTREG(R128_CAP0_TRIG_CNTL, 0);
	OUTREG(R128_CAP1_TRIG_CNTL, 0);

	// If bursts are enabled, turn on discards and aborts.

	uint32 busCntl = INREG(R128_BUS_CNTL);
	if (busCntl & (R128_BUS_WRT_BURST | R128_BUS_READ_BURST)) {
		busCntl |= R128_BUS_RD_DISCARD_EN | R128_BUS_RD_ABORT_EN;
		OUTREG(R128_BUS_CNTL, busCntl);
	}

	// Write the DDA registers.
	//-------------------------

	OUTREG(R128_DDA_CONFIG, params.dda_config);
	OUTREG(R128_DDA_ON_OFF, params.dda_on_off);

	// Write the CRTC registers.
	//--------------------------

	OUTREG(R128_CRTC_GEN_CNTL, params.crtc_gen_cntl);

	OUTREGM(R128_DAC_CNTL, R128_DAC_MASK_ALL | R128_DAC_8BIT_EN,
			~(R128_DAC_RANGE_CNTL | R128_DAC_BLANKING));

	OUTREG(R128_CRTC_H_TOTAL_DISP, params.crtc_h_total_disp);
	OUTREG(R128_CRTC_H_SYNC_STRT_WID, params.crtc_h_sync_strt_wid);
	OUTREG(R128_CRTC_V_TOTAL_DISP, params.crtc_v_total_disp);
	OUTREG(R128_CRTC_V_SYNC_STRT_WID, params.crtc_v_sync_strt_wid);
	OUTREG(R128_CRTC_OFFSET, 0);
	OUTREG(R128_CRTC_OFFSET_CNTL, 0);
	OUTREG(R128_CRTC_PITCH, params.crtc_pitch);

	// Write the PLL registers.
	//-------------------------

	OUTREGM(R128_CLOCK_CNTL_INDEX, R128_PLL_DIV_SEL, R128_PLL_DIV_SEL);

	SetPLLReg(R128_VCLK_ECP_CNTL, R128_VCLK_SRC_SEL_CPUCLK, R128_VCLK_SRC_SEL_MASK);

	SetPLLReg(R128_PPLL_CNTL, 0xffffffff,
		R128_PPLL_RESET | R128_PPLL_ATOMIC_UPDATE_EN | R128_PPLL_VGA_ATOMIC_UPDATE_EN);

	PLLWaitForReadUpdateComplete();
	SetPLLReg(R128_PPLL_REF_DIV, params.ppll_ref_div, R128_PPLL_REF_DIV_MASK);
	PLLWriteUpdate();

	PLLWaitForReadUpdateComplete();
	SetPLLReg(R128_PPLL_DIV_3, params.ppll_div_3,
		R128_PPLL_FB3_DIV_MASK | R128_PPLL_POST3_DIV_MASK);
	PLLWriteUpdate();

	PLLWaitForReadUpdateComplete();
	SetPLLReg(R128_HTOTAL_CNTL, 0);
	PLLWriteUpdate();

	SetPLLReg(R128_PPLL_CNTL, 0, R128_PPLL_RESET
								 | R128_PPLL_SLEEP
								 | R128_PPLL_ATOMIC_UPDATE_EN
								 | R128_PPLL_VGA_ATOMIC_UPDATE_EN);

	snooze(5000);

	SetPLLReg(R128_VCLK_ECP_CNTL, R128_VCLK_SRC_SEL_PPLLCLK,
				R128_VCLK_SRC_SEL_MASK);
}



status_t
Rage128_SetDisplayMode(const DisplayModeEx& mode)
{
	// The code to actually configure the display.
	// All the error checking must be done in ProposeDisplayMode(),
	// and assume that the mode values we get here are acceptable.

	DisplayParams params;		// where computed parameters are saved

	if (gInfo.sharedInfo->displayType == MT_VGA) {
		// Chip is connected to a monitor via a VGA connector.

		if ( ! CalculateCrtcRegisters(mode, params))
			return B_BAD_VALUE;

		if ( ! CalculatePLLRegisters(mode, params))
			return B_BAD_VALUE;

		if ( ! CalculateDDARegisters(mode, params))
			return B_BAD_VALUE;

		SetRegisters(params);

	} else {
		// Chip is connected to a laptop LCD monitor; or via a DVI interface.

		uint16 vesaMode = GetVesaModeNumber(display_mode(mode), mode.bitsPerPixel);
		if (vesaMode == 0)
			return B_BAD_VALUE;

		if (ioctl(gInfo.deviceFileDesc, ATI_SET_VESA_DISPLAY_MODE,
				&vesaMode, sizeof(vesaMode)) != B_OK)
			return B_ERROR;
	}

	Rage128_AdjustFrame(mode);

	// Initialize the palette so that color depths > 8 bits/pixel will display
	// the correct colors.

	// Select primary monitor and enable 8-bit color.
	OUTREGM(R128_DAC_CNTL, R128_DAC_8BIT_EN,
		R128_DAC_PALETTE_ACC_CTL | R128_DAC_8BIT_EN);
	OUTREG8(R128_PALETTE_INDEX, 0);		// set first color index

	for (int i = 0; i < 256; i++)
		OUTREG(R128_PALETTE_DATA, (i << 16) | (i << 8) | i );

	Rage128_EngineInit(mode);

	return B_OK;
}



void
Rage128_AdjustFrame(const DisplayModeEx& mode)
{
	// Adjust start address in frame buffer.

	SharedInfo& si = *gInfo.sharedInfo;

	int address = (mode.v_display_start * mode.virtual_width
			+ mode.h_display_start) * ((mode.bitsPerPixel + 1) / 8);

	address &= ~0x07;
	address += si.frameBufferOffset;

	OUTREG(R128_CRTC_OFFSET, address);
	return;
}


void
Rage128_SetIndexedColors(uint count, uint8 first, uint8* colorData, uint32 flags)
{
	// Set the indexed color palette for 8-bit color depth mode.

	(void)flags;		// avoid compiler warning for unused arg

	if (gInfo.sharedInfo->displayMode.space != B_CMAP8)
		return ;

	// Select primary monitor and enable 8-bit color.
	OUTREGM(R128_DAC_CNTL, R128_DAC_8BIT_EN,
		R128_DAC_PALETTE_ACC_CTL | R128_DAC_8BIT_EN);
	OUTREG8(R128_PALETTE_INDEX, first);		// set first color index

	while (count--) {
		OUTREG(R128_PALETTE_DATA, ((colorData[0] << 16)	// red
								 | (colorData[1] << 8)	// green
								 |  colorData[2]));		// blue
		colorData += 3;
	}
}
