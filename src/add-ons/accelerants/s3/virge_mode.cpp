/*
	Haiku S3 Virge driver adapted from the X.org Virge driver.

	Copyright (C) 1994-1999 The XFree86 Project, Inc.	All Rights Reserved.

	Copyright 2007-2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2007-2008
*/


#include "accel.h"
#include "virge.h"


#define BASE_FREQ		14.31818	// MHz


struct VirgeRegRec {
	uint8  CRTC[25];			// Crtc Controller reg's

	uint8  SR0A, SR0F;
	uint8  SR12, SR13, SR15, SR18; // SR9-SR1C, ext seq.
	uint8  SR29;
	uint8  SR54, SR55, SR56, SR57;
	uint8  CR31, CR33, CR34, CR3A, CR3B, CR3C;
	uint8  CR40, CR41, CR42, CR43, CR45;
	uint8  CR51, CR53, CR54, CR58, CR5D, CR5E;
	uint8  CR63, CR65, CR66, CR67, CR68, CR69, CR6D; // Video attrib.
	uint8  CR7B, CR7D;
	uint8  CR85, CR86, CR87;
	uint8  CR90, CR91, CR92, CR93;
};




static void
Virge_EngineReset(const DisplayModeEx& mode)
{
	SharedInfo& si = *gInfo.sharedInfo;

	switch (mode.bpp) {
		case 8:
			si.commonCmd = DRAW | DST_8BPP;
			break;
		case 16:
			si.commonCmd = DRAW | DST_16BPP;
			break;
		case 24:
			si.commonCmd = DRAW | DST_24BPP;
			break;
	}


	gInfo.WaitQueue(6);
	WriteReg32(CMD_SET, CMD_NOP);		// turn off auto-execute
	WriteReg32(SRC_BASE, 0);
	WriteReg32(DEST_BASE, 0);
	WriteReg32(DEST_SRC_STR, mode.bytesPerRow | (mode.bytesPerRow << 16));

	WriteReg32(CLIP_L_R, ((0) << 16) | mode.timing.h_display);
	WriteReg32(CLIP_T_B, ((0) << 16) | mode.timing.v_display);
}



static void
Virge_NopAllCmdSets()
{
	// This function should be called only for the Trio 3D chip.

	for (int i = 0; i < 1000; i++) {
		if ( (IN_SUBSYS_STAT() & 0x3f802000 & 0x20002000) == 0x20002000) {
			break;
		}
	}

	gInfo.WaitQueue(7);
	WriteReg32(CMD_SET, CMD_NOP);
}



static void
Virge_GEReset(const DisplayModeEx& mode)
{
	SharedInfo& si = *gInfo.sharedInfo;

	if (si.chipType == S3_TRIO_3D)
		Virge_NopAllCmdSets();

	gInfo.WaitIdleEmpty();

	if (si.chipType == S3_TRIO_3D) {
		bool ge_was_on = false;
		snooze(10000);

		for (int r = 1; r < 10; r++) {
			uint8  resetidx = 0x66;

			VerticalRetraceWait();
			uint8 tmp = ReadCrtcReg(resetidx);

			VerticalRetraceWait();
			IN_SUBSYS_STAT();

			// turn off the GE

			if (tmp & 0x01) {
				WriteCrtcReg(resetidx, tmp);
				ge_was_on = true;
				snooze(10000);
			}

			IN_SUBSYS_STAT();
			WriteCrtcReg(resetidx, tmp | 0x02);
			snooze(10000);

			VerticalRetraceWait();
			WriteCrtcReg(resetidx, tmp & ~0x02);
			snooze(10000);

			if (ge_was_on) {
				tmp |= 0x01;
				WriteCrtcReg(resetidx, tmp);
				snooze(10000);
			}

			VerticalRetraceWait();

			Virge_NopAllCmdSets();
			gInfo.WaitIdleEmpty();

			WriteReg32(DEST_SRC_STR, mode.bytesPerRow << 16 | mode.bytesPerRow);
			snooze(10000);

			if ((IN_SUBSYS_STAT() & 0x3f802000 & 0x20002000) != 0x20002000) {
				TRACE("Restarting S3 graphics engine reset %2d ...%lx\n",
					   r, IN_SUBSYS_STAT() );
			} else
				break;
		}
	} else {
		uint8 regIndex = (si.chipType == S3_VIRGE_VX ? 0x63 : 0x66);
		uint8 tmp = ReadCrtcReg(regIndex);
		snooze(10000);

		// try multiple times to avoid lockup of VIRGE/MX

		for (int r = 1; r < 10; r++) {
			WriteCrtcReg(regIndex, tmp | 0x02);
			snooze(10000);
			WriteCrtcReg(regIndex, tmp & ~0x02);
			snooze(10000);
			gInfo.WaitIdleEmpty();

			WriteReg32(DEST_SRC_STR, mode.bytesPerRow << 16 | mode.bytesPerRow);
			snooze(10000);

			if (((IN_SUBSYS_STAT() & 0x3f00) != 0x3000)) {
				TRACE("Restarting S3 graphics engine reset %2d ...\n", r);
			} else
				break;
		}
	}

	gInfo.WaitQueue(2);
	WriteReg32(SRC_BASE, 0);
	WriteReg32(DEST_BASE, 0);

	gInfo.WaitQueue(4);
	WriteReg32(CLIP_L_R, ((0) << 16) | mode.timing.h_display);
	WriteReg32(CLIP_T_B, ((0) << 16) | mode.timing.v_display);
	WriteReg32(MONO_PAT_0, ~0);
	WriteReg32(MONO_PAT_1, ~0);

	if (si.chipType == S3_TRIO_3D)
		Virge_NopAllCmdSets();
}



static void
Virge_CalcClock(long freq, int min_m,
				int min_n1, int max_n1,
				int min_n2, int max_n2,
				long freq_min, long freq_max,
				uint8* mdiv, uint8* ndiv)
{
	uint8 best_n1 = 16 + 2, best_n2 = 2, best_m = 125 + 2;

	double ffreq = freq / 1000.0 / BASE_FREQ;
	double ffreq_min = freq_min / 1000.0 / BASE_FREQ;
	double ffreq_max = freq_max / 1000.0 / BASE_FREQ;

	if (ffreq < ffreq_min / (1 << max_n2)) {
		TRACE("Virge_CalcClock() invalid frequency %1.3f Mhz  [freq <= %1.3f MHz]\n",
			   ffreq * BASE_FREQ, ffreq_min * BASE_FREQ / (1 << max_n2) );
		ffreq = ffreq_min / (1 << max_n2);
	}
	if (ffreq > ffreq_max / (1 << min_n2)) {
		TRACE("Virge_CalcClock() invalid frequency %1.3f Mhz  [freq >= %1.3f MHz]\n",
			   ffreq * BASE_FREQ, ffreq_max * BASE_FREQ / (1 << min_n2) );
		ffreq = ffreq_max / (1 << min_n2);
	}

	// Work out suitable timings.

	double best_diff = ffreq;

	for (uint8 n2 = min_n2; n2 <= max_n2; n2++) {
		for (uint8 n1 = min_n1 + 2; n1 <= max_n1 + 2; n1++) {
			int m = (int)(ffreq * n1 * (1 << n2) + 0.5) ;
			if (m < min_m + 2 || m > 127 + 2)
				continue;

			double div = (double)(m) / (double)(n1);
			if ((div >= ffreq_min) && (div <= ffreq_max)) {
				double diff = ffreq - div / (1 << n2);
				if (diff < 0.0)
					diff = -diff;
				if (diff < best_diff) {
					best_diff = diff;
					best_m = m;
					best_n1 = n1;
					best_n2 = n2;
				}
			}
		}
	}

	if (max_n1 == 63)
		*ndiv = (best_n1 - 2) | (best_n2 << 6);
	else
		*ndiv = (best_n1 - 2) | (best_n2 << 5);
	*mdiv = best_m - 2;
}



static void
Virge_WriteMode(const DisplayModeEx& mode, VirgeRegRec& regRec)
{
	// This function writes out all of the standard VGA and extended S3 registers
	// needed to setup a video mode.

	TRACE("Virge_WriteMode()\n");

	SharedInfo& si = *gInfo.sharedInfo;

	// First reset GE to make sure nothing is going on.

	if (ReadCrtcReg(si.chipType == S3_VIRGE_VX ? 0x63 : 0x66) & 0x01)
		Virge_GEReset(mode);

	// As per databook, always disable STREAMS before changing modes.

	if ((ReadCrtcReg(0x67) & 0x0c) == 0x0c) {
		// STREAMS running, disable it
		VerticalRetraceWait();
		WriteReg32(FIFO_CONTROL_REG, 0xC000);

		WriteCrtcReg(0x67, 0x00, 0x0c);		// disable STREAMS processor
	}

	// Restore S3 extended regs.
	WriteCrtcReg(0x63, regRec.CR63);
	WriteCrtcReg(0x66, regRec.CR66);
	WriteCrtcReg(0x3a, regRec.CR3A);
	WriteCrtcReg(0x31, regRec.CR31);
	WriteCrtcReg(0x58, regRec.CR58);

	// Extended mode timings registers.
	WriteCrtcReg(0x53, regRec.CR53);
	WriteCrtcReg(0x5d, regRec.CR5D);
	WriteCrtcReg(0x5e, regRec.CR5E);
	WriteCrtcReg(0x3b, regRec.CR3B);
	WriteCrtcReg(0x3c, regRec.CR3C);
	WriteCrtcReg(0x43, regRec.CR43);
	WriteCrtcReg(0x65, regRec.CR65);
	WriteCrtcReg(0x6d, regRec.CR6D);

	// Restore the desired video mode with CR67.

	WriteCrtcReg(0x67, 0x50, 0xf0);		// possible hardware bug on VX?
	snooze(10000);
	WriteCrtcReg(0x67, regRec.CR67 & ~0x0c);	// Don't enable STREAMS

	// Other mode timing and extended regs.

	WriteCrtcReg(0x34, regRec.CR34);
	if (si.chipType != S3_TRIO_3D && si.chipType != S3_VIRGE_MX) {
		WriteCrtcReg(0x40, regRec.CR40);
	}

	if (S3_VIRGE_MX_SERIES(si.chipType)) {
		WriteCrtcReg(0x41, regRec.CR41);
	}

	WriteCrtcReg(0x42, regRec.CR42);
	WriteCrtcReg(0x45, regRec.CR45);
	WriteCrtcReg(0x51, regRec.CR51);
	WriteCrtcReg(0x54, regRec.CR54);

	// Memory timings.
	WriteCrtcReg(0x68, regRec.CR68);
	WriteCrtcReg(0x69, regRec.CR69);

	WriteCrtcReg(0x33, regRec.CR33);
	if (si.chipType == S3_TRIO_3D_2X || S3_VIRGE_GX2_SERIES(si.chipType)
			/* MXTESTME */ ||  S3_VIRGE_MX_SERIES(si.chipType) ) {
		WriteCrtcReg(0x85, regRec.CR85);
	}

	if (si.chipType == S3_VIRGE_DXGX) {
		WriteCrtcReg(0x86, regRec.CR86);
	}

	if ( (si.chipType == S3_VIRGE_GX2) || S3_VIRGE_MX_SERIES(si.chipType) ) {
		WriteCrtcReg(0x7b, regRec.CR7B);
		WriteCrtcReg(0x7d, regRec.CR7D);
		WriteCrtcReg(0x87, regRec.CR87);
		WriteCrtcReg(0x92, regRec.CR92);
		WriteCrtcReg(0x93, regRec.CR93);
	}

	if (si.chipType == S3_VIRGE_DXGX || S3_VIRGE_GX2_SERIES(si.chipType) ||
			S3_VIRGE_MX_SERIES(si.chipType) || si.chipType == S3_TRIO_3D) {
		WriteCrtcReg(0x90, regRec.CR90);
		WriteCrtcReg(0x91, regRec.CR91);
	}

	WriteSeqReg(0x08, 0x06);	// unlock extended sequencer regs

	// Restore extended sequencer regs for DCLK.

	WriteSeqReg(0x12, regRec.SR12);
	WriteSeqReg(0x13, regRec.SR13);

	if (S3_VIRGE_GX2_SERIES(si.chipType) || S3_VIRGE_MX_SERIES(si.chipType)) {
		WriteSeqReg(0x29, regRec.SR29);
	}
	if (S3_VIRGE_MX_SERIES(si.chipType)) {
		WriteSeqReg(0x54, regRec.SR54);
		WriteSeqReg(0x55, regRec.SR55);
		WriteSeqReg(0x56, regRec.SR56);
		WriteSeqReg(0x57, regRec.SR57);
	}

	WriteSeqReg(0x18, regRec.SR18);

	// Load new m,n PLL values for DCLK & MCLK.
	uint8 tmp = ReadSeqReg(0x15) & ~0x21;
	WriteSeqReg(0x15, tmp | 0x03);
	WriteSeqReg(0x15, tmp | 0x23);
	WriteSeqReg(0x15, tmp | 0x03);
	WriteSeqReg(0x15, regRec.SR15);

	if (si.chipType == S3_TRIO_3D) {
		WriteSeqReg(0x0a, regRec.SR0A);
		WriteSeqReg(0x0f, regRec.SR0F);
	}

	// Now write out CR67 in full, possibly starting STREAMS.

	VerticalRetraceWait();
	WriteCrtcReg(0x67, 0x50);   // For possible bug on VX?!
	snooze(10000);
	WriteCrtcReg(0x67, regRec.CR67);

	uint8 cr66 = ReadCrtcReg(0x66);
	WriteCrtcReg(0x66, cr66 | 0x80);

	WriteCrtcReg(0x3a, regRec.CR3A | 0x80);

	// Now, before we continue, check if this mode has the graphic engine ON.
	// If yes, then we reset it.

	if (si.chipType == S3_VIRGE_VX) {
		if (regRec.CR63 & 0x01)
			Virge_GEReset(mode);
	} else {
		if (regRec.CR66 & 0x01)
			Virge_GEReset(mode);
	}

	VerticalRetraceWait();
	if (S3_VIRGE_GX2_SERIES(si.chipType) || S3_VIRGE_MX_SERIES(si.chipType) ) {
		WriteCrtcReg(0x85, 0x1f);	// primary stream threshold
	}

	// Set the standard CRTC vga regs.

	WriteCrtcReg(0x11, 0x00, 0x80);		// unlock CRTC reg's 0-7 by clearing bit 7 of cr11

	for (int j = 0; j < NUM_ELEMENTS(regRec.CRTC); j++) {
		WriteCrtcReg(j, regRec.CRTC[j]);
	}

	// Setup HSYNC & VSYNC polarity and select clock source 2 (0x0c) for
	// programmable PLL.

	uint8 miscOutReg = 0x23 | 0x0c;

	if (!(mode.timing.flags & B_POSITIVE_HSYNC))
		miscOutReg |= 0x40;
	if (!(mode.timing.flags & B_POSITIVE_VSYNC))
		miscOutReg |= 0x80;

	WriteMiscOutReg(miscOutReg);

	WriteCrtcReg(0x66, cr66);
	WriteCrtcReg(0x3a, regRec.CR3A);

	return ;
}



static bool
Virge_ModeInit(const DisplayModeEx& mode)
{
	SharedInfo& si = *gInfo.sharedInfo;
	VirgeRegRec regRec;

	TRACE("Virge_ModeInit(%d x %d, %d KHz)\n",
		mode.timing.h_display, mode.timing.v_display, mode.timing.pixel_clock);

	// Set scale factors for mode timings.

	int horizScaleFactor = 1;

	if (si.chipType == S3_VIRGE_VX || S3_VIRGE_GX2_SERIES(si.chipType) ||
			S3_VIRGE_MX_SERIES(si.chipType)) {
		horizScaleFactor = 1;
	} else if (mode.bpp == 8) {
		horizScaleFactor = 1;
	} else if (mode.bpp == 16) {
		if (si.chipType == S3_TRIO_3D && mode.timing.pixel_clock > 115000)
			horizScaleFactor = 1;
		else
			horizScaleFactor = 2;
	} else {
		horizScaleFactor = 1;
	}

	InitCrtcTimingValues(mode, horizScaleFactor, regRec.CRTC,
			regRec.CR3B, regRec.CR3C, regRec.CR5D, regRec.CR5E);

	// Now we fill in the rest of the stuff we need for the Virge.
	// Start with MMIO, linear address regs.

	uint8 temp = ReadCrtcReg(0x3a);
	if ( S3_VIRGE_GX2_SERIES(si.chipType) || S3_VIRGE_MX_SERIES(si.chipType) )
		regRec.CR3A = (temp & 0x7f) | 0x10;		// ENH 256, PCI burst
	else
		regRec.CR3A = (temp & 0x7f) | 0x15;		// ENH 256, PCI burst

	regRec.CR53 = ReadCrtcReg(0x53);

	if (si.chipType == S3_TRIO_3D) {
		regRec.CR31 = 0x0c;		// [trio3d] page 54
	} else {
		regRec.CR53 = 0x08;		// Enables MMIO
		regRec.CR31 = 0x8c;		// Dis. 64k window, en. ENH maps
	}

	// Enables S3D graphic engine and PCI disconnects.
	if (si.chipType == S3_VIRGE_VX) {
		regRec.CR66 = 0x90;
		regRec.CR63 = 0x09;
	} else {
		regRec.CR66 = 0x89;
		// Set display fifo.
		if ( S3_VIRGE_GX2_SERIES(si.chipType) ||
				S3_VIRGE_MX_SERIES(si.chipType) ) {
			// Changed from 0x08 based on reports that this
			// prevents MX from running properly below 1024x768.
			regRec.CR63 = 0x10;
		} else {
			regRec.CR63 = 0;
		}
	}

	// Now set linear address registers.
	// LAW size: we have 2 cases, 2MB, 4MB or >= 4MB for VX.
	regRec.CR58 = ReadCrtcReg(0x58) & 0x80;

	if (si.videoMemSize == 1 * 1024 * 1024)
		regRec.CR58 |= 0x01 | 0x10;
	else if (si.videoMemSize == 2 * 1024 * 1024)
		regRec.CR58 |= 0x02 | 0x10;
	else {
		if (si.chipType == S3_TRIO_3D_2X && si.videoMemSize == 8 * 1024 * 1024)
			regRec.CR58 |= 0x07 | 0x10; 	// 8MB window on Trio3D/2X
		else
			regRec.CR58 |= 0x03 | 0x10; 	// 4MB window on virge, 8MB on VX
	}

	if (si.chipType == S3_VIRGE_VX)
		regRec.CR58 |= 0x40;

	// ** On PCI bus, no need to reprogram the linear window base address.

	// Now do clock PLL programming. Use the s3gendac function to get m,n.
	// Also determine if we need doubling etc.

	int dclk = mode.timing.pixel_clock;

	if (si.chipType == S3_TRIO_3D) {
		regRec.SR15 = (ReadSeqReg(0x15) & 0x80) | 0x03;	// keep BIOS init defaults
		regRec.SR0A = ReadSeqReg(0x0a);
	} else
		regRec.SR15 = 0x03 | 0x80;

	regRec.SR18 = 0x00;
	regRec.CR43 = 0x00;
	regRec.CR45 = 0x00;
	// Enable MMIO to RAMDAC registers.
	regRec.CR65 = 0x00;		// CR65_2 must be zero, doc seems to be wrong
	regRec.CR54 = 0x00;

	if (si.chipType != S3_TRIO_3D && si.chipType != S3_VIRGE_MX) {
		regRec.CR40 = ReadCrtcReg(0x40) & ~0x01;
	}

	if (S3_VIRGE_MX_SERIES(si.chipType)) {
		// Fix problems with APM suspend/resume trashing CR90/91.
		switch (mode.bpp) {
			case 8:
				regRec.CR41 = 0x38;
				break;
			case 16:
				regRec.CR41 = 0x48;
				break;
			default:
				regRec.CR41 = 0x77;
		}
	}

	regRec.CR67 = 0x00;			// defaults

	if (si.chipType == S3_VIRGE_VX) {
		if (mode.bpp == 8) {
			if (dclk <= 110000)
				regRec.CR67 = 0x00;		// 8bpp, 135MHz
			else
				regRec.CR67 = 0x10;		// 8bpp, 220MHz
		} else if (mode.bpp == 16) {
			if (dclk <= 110000)
				regRec.CR67 = 0x40;		// 16bpp, 135MHz
			else
				regRec.CR67 = 0x50;		// 16bpp, 220MHz
		}
		Virge_CalcClock(dclk, 1, 1, 31, 0, 4, 220000, 440000,
						&regRec.SR13, &regRec.SR12);
	} // end VX if()

	else if (S3_VIRGE_GX2_SERIES(si.chipType) || S3_VIRGE_MX_SERIES(si.chipType)) {
		uint8 ndiv;

		if (mode.bpp == 8)
			regRec.CR67 = 0x00;
		else if (mode.bpp == 16)
			regRec.CR67 = 0x50;

		// X.org code had a somewhat convuluted way of computing the clock for
		// MX chips.  I hope this simpler method works for most MX cases.

		Virge_CalcClock(dclk, 1, 1, 31, 0, 4, 170000, 340000,
						&regRec.SR13, &ndiv);

		regRec.SR29 = ndiv >> 7;
		regRec.SR12 = (ndiv & 0x1f) | ((ndiv & 0x60) << 1);
	} // end GX2 or MX if()

	else if (si.chipType == S3_TRIO_3D) {
		regRec.SR0F = 0x00;
		if (mode.bpp == 8) {
			if (dclk > 115000) {			// We need pixmux
				regRec.CR67 = 0x10;
				regRec.SR15 |= 0x10;		// Set DCLK/2 bit
				regRec.SR18 = 0x80;			// Enable pixmux
			}
		} else if (mode.bpp == 16) {
			if (dclk > 115000) {
				regRec.CR67 = 0x40;
				regRec.SR15 |= 0x10;
				regRec.SR18 = 0x80;
				regRec.SR0F = 0x10;
			} else {
				regRec.CR67 = 0x50;
			}
		}
		Virge_CalcClock(dclk, 1, 1, 31, 0, 4, 230000, 460000,
						&regRec.SR13, &regRec.SR12);
	} // end TRIO_3D if()

	else {           // Everything else ... (only VIRGE & VIRGE DX/GX).
		if (mode.bpp == 8) {
			if (dclk > 80000) {				// We need pixmux
				regRec.CR67 = 0x10;
				regRec.SR15 |= 0x10;		// Set DCLK/2 bit
				regRec.SR18 = 0x80;			// Enable pixmux
			}
		} else if (mode.bpp == 16) {
			regRec.CR67 = 0x50;
		}
		Virge_CalcClock(dclk, 1, 1, 31, 0, 3, 135000, 270000,
						&regRec.SR13, &regRec.SR12);
	} // end great big if()...


	regRec.CR42 = 0x00;

	if (S3_VIRGE_GX2_SERIES(si.chipType) || S3_VIRGE_MX_SERIES(si.chipType) ) {
		regRec.CR34 = 0;
	} else {
		regRec.CR34 = 0x10;		// set display fifo
	}

	int width = mode.bytesPerRow >> 3;
	regRec.CRTC[0x13] = 0xFF & width;
	regRec.CR51 = (0x300 & width) >> 4; 	// Extension bits

	regRec.CR33 = 0x20;
	if (si.chipType == S3_TRIO_3D_2X || S3_VIRGE_GX2_SERIES(si.chipType)
			/* MXTESTME */ ||  S3_VIRGE_MX_SERIES(si.chipType) ) {
		regRec.CR85 = 0x12;  	// avoid sreen flickering
		// by increasing FIFO filling, larger # fills FIFO from memory earlier
		// on GX2 this affects all depths, not just those running STREAMS.
		// new, secondary stream settings.
		regRec.CR87 = 0x10;
		// gx2 - set up in XV init code
		regRec.CR92 = 0x00;
		regRec.CR93 = 0x00;
		// gx2 primary mclk timeout, def=0xb
		regRec.CR7B = 0xb;
		// gx2 secondary mclk timeout, def=0xb
		regRec.CR7D = 0xb;
	}

	if (si.chipType == S3_VIRGE_DXGX || si.chipType == S3_TRIO_3D) {
		regRec.CR86 = 0x80;  // disable DAC power saving to avoid bright left edge
	}

	if (si.chipType == S3_VIRGE_DXGX || S3_VIRGE_GX2_SERIES(si.chipType) ||
			S3_VIRGE_MX_SERIES(si.chipType) || si.chipType == S3_TRIO_3D) {
		int dbytes = mode.bytesPerRow;
		regRec.CR91 = (dbytes + 7) / 8;
		regRec.CR90 = (((dbytes + 7) / 8) >> 8) | 0x80;
	}

	// S3_BLANK_DELAY settings based on defaults only. From 3.3.3.

	int blank_delay;

	if (si.chipType == S3_VIRGE_VX) {
		// These values need to be changed once CR67_1 is set
		// for gamma correction (see S3V server)!
		if (mode.bpp == 8)
			blank_delay = 0x00;
		else if (mode.bpp == 16)
			blank_delay = 0x00;
		else
			blank_delay = 0x51;
	} else {
		if (mode.bpp == 8)
			blank_delay = 0x00;
		else if (mode.bpp == 16)
			blank_delay = 0x02;
		else
			blank_delay = 0x04;
	}

	if (si.chipType == S3_VIRGE_VX) {
		regRec.CR6D = blank_delay;
	} else {
		regRec.CR65 = (regRec.CR65 & ~0x38) | (blank_delay & 0x07) << 3;
		regRec.CR6D = ReadCrtcReg(0x6d);
	}

	regRec.CR68 = ReadCrtcReg(0x68);
	regRec.CR69 = 0;

	// Flat panel centering and expansion registers.
	regRec.SR54 = 0x1f ;
	regRec.SR55 = 0x9f ;
	regRec.SR56 = 0x1f ;
	regRec.SR57 = 0xff ;

	Virge_WriteMode(mode, regRec);		// write mode registers to hardware

	// Note that the Virge VX chip does not display the hardware cursor when the
	// mode is set to 640x480;  thus, in this case the hardware cursor functions
	// will be disabled so that a software cursor will be used.

	si.bDisableHdwCursor = (si.chipType == S3_VIRGE_VX
		&& mode.timing.h_display == 640 && mode.timing.v_display == 480);

	return true;
}


bool 
Virge_SetDisplayMode(const DisplayModeEx& mode)
{
	// The code to actually configure the display.
	// All the error checking must be done in PROPOSE_DISPLAY_MODE(),
	// and assume that the mode values we get here are acceptable.

	WriteSeqReg(0x01, 0x20, 0x20);		// blank the screen

	if ( ! Virge_ModeInit(mode)) {
		TRACE("Virge_ModeInit() failed\n");
		return false;
	}

	Virge_AdjustFrame(mode);
	Virge_EngineReset(mode);

	WriteSeqReg(0x01, 0x00, 0x20);		// unblank the screen

	return true;
}



void
Virge_AdjustFrame(const DisplayModeEx& mode)
{
	// Adjust start address in frame buffer. We use the new CR69 reg
	// for this purpose instead of the older CR31/CR51 combo.

	SharedInfo& si = *gInfo.sharedInfo;

	int base = ((mode.v_display_start * mode.virtual_width + mode.h_display_start)
			* (mode.bpp / 8)) >> 2;

	if (mode.bpp == 16)
		if (si.chipType == S3_TRIO_3D && mode.timing.pixel_clock > 115000)
			base &= ~1;

	base += si.frameBufferOffset;

	// Now program the start address registers.

	WriteCrtcReg(0x0c, (base >> 8) & 0xff);
	WriteCrtcReg(0x0d, base & 0xff);
	WriteCrtcReg(0x69, (base & 0x0F0000) >> 16);
}


void 
Virge_SetIndexedColors(uint count, uint8 first, uint8* colorData, uint32 flags)
{
	// Set the indexed color palette for 8-bit color depth mode.

	(void)flags;		// avoid compiler warning for unused arg

	if (gInfo.sharedInfo->displayMode.space != B_CMAP8)
		return ;

	while (count--) {
		WriteIndexedColor(first++,	// color index
			colorData[0] >> 2,		// red
			colorData[1] >> 2,		// green
			colorData[2] >> 2);		// blue
		colorData += 3;
	}
}
