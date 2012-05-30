/*
 * Copyright 2012 Haiku, Inc.  All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
*/

/*!
	Haiku Intel-810 video driver was adapted from the X.org intel driver which
	has the following copyright.

	Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
	All Rights Reserved.
 */


#include "accelerant.h"
#include "i810_regs.h"

#include <create_display_modes.h>		// common accelerant header file
#include <math.h>
#include <unistd.h>


// I810_CalcVCLK -- Determine closest clock frequency to the one requested.
#define MAX_VCO_FREQ 600.0
#define TARGET_MAX_N 30
#define REF_FREQ	 24.0

#define CALC_VCLK(m,n,p) (double)m / ((double)n * (1 << p)) * 4 * REF_FREQ


static void
CalcVCLK(double freq, uint16& clkM, uint16& clkN, uint16& clkP) {
	int m, n, p;
	double f_out, f_best;
	double f_err;
	double f_vco;
	int m_best = 0, n_best = 0, p_best = 0;
	double f_target = freq;
	double errMax = 0.005;
	double errTarget = 0.001;
	double errBest = 999999.0;

	p_best = p = int(log(MAX_VCO_FREQ / f_target) / log((double)2));
	// Make sure p is within range.
	if (p_best > 5) {
		p_best = p = 5;
	}

	f_vco = f_target * (1 << p);

	n = 2;
	do {
		n++;
		m = int(f_vco / (REF_FREQ / (double)n) / (double)4.0 + 0.5);
		if (m < 3)
			m = 3;
		f_out = CALC_VCLK(m, n, p);
		f_err = 1.0 - (f_target / f_out);
		if (fabs(f_err) < errMax) {
			m_best = m;
			n_best = n;
			f_best = f_out;
			errBest = f_err;
		}
	} while ((fabs(f_err) >= errTarget) && ((n <= TARGET_MAX_N)
		|| (fabs(errBest) > errMax)));

	if (fabs(f_err) < errTarget) {
		m_best = m;
		n_best = n;
	}

	clkM = (m_best - 2) & 0x3FF;
	clkN = (n_best - 2) & 0x3FF;
	clkP = (p_best << 4);

	TRACE("Setting dot clock to %.1f MHz [ 0x%x 0x%x 0x%x ] [ %d %d %d ]\n",
		CALC_VCLK(m_best, n_best, p_best),
		clkM, clkN, clkP, m_best, n_best, p_best);
}


static void
SetCrtcTimingValues(const DisplayModeEx& mode)
{
	// Set the timing values for CRTC registers cr00 to cr18, and some extended
	// CRTC registers.

	int hTotal = mode.timing.h_total / 8 - 5;
	int hDisp_e = mode.timing.h_display / 8 - 1;
	int hSync_s = mode.timing.h_sync_start / 8;
	int hSync_e = mode.timing.h_sync_end / 8;
	int hBlank_s = hDisp_e + 1;		// start of horizontal blanking
	int hBlank_e = hTotal;			// end of horizontal blanking

	int vTotal = mode.timing.v_total - 2;
	int vDisp_e = mode.timing.v_display - 1;
	int vSync_s = mode.timing.v_sync_start;
	int vSync_e = mode.timing.v_sync_end;
	int vBlank_s = vDisp_e;			// start of vertical blanking
	int vBlank_e = vTotal;			// end of vertical blanking

	uint16 offset = mode.bytesPerRow / 8;

	// CRTC Controller values

	uint8 crtc[25];
	crtc[0x00] = hTotal;
	crtc[0x01] = hDisp_e;
	crtc[0x02] = hBlank_s;
	crtc[0x03] = (hBlank_e & 0x1f) | 0x80;
	crtc[0x04] = hSync_s;
	crtc[0x05] = ((hSync_e & 0x1f) | ((hBlank_e & 0x20) << 2));
	crtc[0x06] = vTotal;
	crtc[0x07] = (((vTotal & 0x100) >> 8)
		| ((vDisp_e & 0x100) >> 7)
		| ((vSync_s & 0x100) >> 6)
		| ((vBlank_s & 0x100) >> 5)
		| 0x10
		| ((vTotal & 0x200) >> 4)
		| ((vDisp_e & 0x200) >> 3)
		| ((vSync_s & 0x200) >> 2));

	crtc[0x08] = 0x00;
	crtc[0x09] = ((vBlank_s & 0x200) >> 4) | 0x40;
	crtc[0x0a] = 0x00;
	crtc[0x0b] = 0x00;
	crtc[0x0c] = 0x00;
	crtc[0x0d] = 0x00;
	crtc[0x0e] = 0x00;
	crtc[0x0f] = 0x00;
	crtc[0x10] = vSync_s;
	crtc[0x11] = (vSync_e & 0x0f) | 0x20;
	crtc[0x12] = vDisp_e;
	crtc[0x13] = offset;
	crtc[0x14] = 0x00;
	crtc[0x15] = vBlank_s;
	crtc[0x16] = vBlank_e;
	crtc[0x17] = 0xc3;
	crtc[0x18] = 0xff;

	// Set the standard CRTC vga regs;  however, before setting them, unlock
	// CRTC reg's 0-7 by clearing bit 7 of cr11

	WriteCrtcReg(0x11, crtc[0x11] & ~0x80);

	for (uint8 j = 0; j <= 0x18; j++)
		WriteCrtcReg(j, crtc[j]);

	// Set the extended CRTC reg's.

	WriteCrtcReg(EXT_VERT_TOTAL, vTotal >> 8);
	WriteCrtcReg(EXT_VERT_DISPLAY, vDisp_e >> 8);
	WriteCrtcReg(EXT_VERT_SYNC_START, vSync_s >> 8);
	WriteCrtcReg(EXT_VERT_BLANK_START, vBlank_s >> 8);
	WriteCrtcReg(EXT_HORIZ_TOTAL, hTotal >> 8);
	WriteCrtcReg(EXT_HORIZ_BLANK, (hBlank_e & 0x40) >> 6);
	WriteCrtcReg(EXT_OFFSET, offset >> 8);

	WriteCrtcReg(INTERLACE_CNTL, INTERLACE_DISABLE);	// turn off interlace

	// Enable high resolution mode.
	WriteCrtcReg(IO_CTNL, ReadCrtcReg(IO_CTNL) | EXTENDED_CRTC_CNTL);
}


status_t
I810_SetDisplayMode(const DisplayModeEx& mode)
{
	if (mode.bitsPerPixel != 8 && mode.bitsPerPixel != 16) {
		// Only 8 & 16 bits/pixel are suppoted.
		TRACE("Unsupported color depth: %d bpp\n", mode.bitsPerPixel);
		return B_ERROR;
	}

	snooze(50000);

	// Turn off DRAM refresh.
	uint8 temp = INREG8(DRAM_ROW_CNTL_HI) & ~DRAM_REFRESH_RATE;
	OUTREG8(DRAM_ROW_CNTL_HI, temp | DRAM_REFRESH_DISABLE);

	snooze(1000);			// wait 1 ms

	// Calculate the VCLK that most closely matches the requested pixel clock,
	// and then set the M, N, and P values.

	uint16 m, n, p;
	CalcVCLK(mode.timing.pixel_clock / 1000.0, m, n, p);

	OUTREG16(VCLK2_VCO_M, m);
	OUTREG16(VCLK2_VCO_N, n);
	OUTREG8(VCLK2_VCO_DIV_SEL, p);

	// Setup HSYNC & VSYNC polarity and select clock source 2 (0x08) for
	// programmable PLL.

	uint8 miscOutReg = 0x08 | 0x01;
	if (!(mode.timing.flags & B_POSITIVE_HSYNC))
		miscOutReg |= 0x40;
	if (!(mode.timing.flags & B_POSITIVE_VSYNC))
		miscOutReg |= 0x80;

	OUTREG8(MISC_OUT_W, miscOutReg);

	SetCrtcTimingValues(mode);

	OUTREG32(MEM_MODE, INREG32(MEM_MODE) | 4);

	// Set the address mapping to use the frame buffer memory mapped via the
	// GTT table instead of the VGA buffer.

	uint8 addrMapping = ReadGraphReg(ADDRESS_MAPPING);
	addrMapping &= 0xE0;		// preserve reserved bits 7:5
	addrMapping |= (GTT_MEM_MAP_ENABLE | LINEAR_MODE_ENABLE);
	WriteGraphReg(ADDRESS_MAPPING, addrMapping);

	// Turn on DRAM refresh.
	temp = INREG8(DRAM_ROW_CNTL_HI) & ~DRAM_REFRESH_RATE;
	OUTREG8(DRAM_ROW_CNTL_HI, temp | DRAM_REFRESH_60HZ);

	temp = INREG8(BITBLT_CNTL) & ~COLEXP_MODE;
	temp |= (mode.bitsPerPixel == 8 ? COLEXP_8BPP : COLEXP_16BPP);
	OUTREG8(BITBLT_CNTL, temp);

	// Turn on 8 bit dac mode so that the indexed colors are displayed properly,
	// and put display in high resolution mode.

	uint32 temp32 = INREG32(PIXPIPE_CONFIG) & 0xF3E062FC;
	temp32 |= (DAC_8_BIT | HIRES_MODE | NO_BLANK_DELAY |
		(mode.bitsPerPixel == 8 ? DISPLAY_8BPP_MODE : DISPLAY_16BPP_MODE));
	OUTREG32(PIXPIPE_CONFIG, temp32);

	OUTREG16(EIR, 0);

	temp32 = INREG32(FWATER_BLC);
	temp32 &= ~(LM_BURST_LENGTH | LM_FIFO_WATERMARK
		| MM_BURST_LENGTH | MM_FIFO_WATERMARK);
	temp32 |= I810_GetWatermark(mode);
	OUTREG32(FWATER_BLC, temp32);

	// Enable high resolution mode.
	WriteCrtcReg(IO_CTNL, ReadCrtcReg(IO_CTNL) | EXTENDED_CRTC_CNTL);

	I810_AdjustFrame(mode);
	return B_OK;
}


void
I810_AdjustFrame(const DisplayModeEx& mode)
{
	// Adjust start address in frame buffer.

	uint32 address = ((mode.v_display_start * mode.virtual_width
		+ mode.h_display_start) * mode.bytesPerPixel) >> 2;

	WriteCrtcReg(START_ADDR_LO, address & 0xff);
	WriteCrtcReg(START_ADDR_HI, (address >> 8) & 0xff);
	WriteCrtcReg(EXT_START_ADDR_HI, (address >> 22) & 0xff);
	WriteCrtcReg(EXT_START_ADDR,
		((address >> 16) & 0x3f) | EXT_START_ADDR_ENABLE);
}


void
I810_SetIndexedColors(uint count, uint8 first, uint8* colorData, uint32 flags)
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
