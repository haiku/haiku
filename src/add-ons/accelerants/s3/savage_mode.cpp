/*
	Haiku S3 Savage driver adapted from the X.org Savage driver.

	Copyright (C) 1994-2000 The XFree86 Project, Inc.	All Rights Reserved.
	Copyright (c) 2003-2006, X.Org Foundation

	Copyright 2007-2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2006-2008
*/


#include "accel.h"
#include "savage.h"


#define BASE_FREQ	14.31818


struct SavageRegRec {
	uint8  CRTC[25];			// Crtc Controller reg's

	uint8  SR12, SR13, SR1B, SR29;
	uint8  CR33, CR34, CR3A, CR3B, CR3C;
	uint8  CR42, CR43, CR45;
	uint8  CR50, CR51, CR53, CR58, CR5D, CR5E;
	uint8  CR65, CR66, CR67, CR69;
	uint8  CR86, CR88;
	uint8  CR90, CR91, CRB0;
};



static void 
Savage_SetGBD_Twister(const DisplayModeEx& mode)
{
	SharedInfo& si = *gInfo.sharedInfo;
	int bci_enable;

	if (si.chipType == S3_SAVAGE4)
		bci_enable = BCI_ENABLE;
	else
		bci_enable = BCI_ENABLE_TWISTER;

	// MM81C0 and 81C4 are used to control primary stream.
	WriteReg32(PRI_STREAM_FBUF_ADDR0, 0);
	WriteReg32(PRI_STREAM_FBUF_ADDR1, 0);

	// Program Primary Stream Stride Register.
	//
	// Tell engine if tiling on or off, set primary stream stride, and
	// if tiling, set tiling bits/pixel and primary stream tile offset.
	// Note that tile offset (bits 16 - 29) must be scanline width in
	// bytes/128bytespertile * 256 Qwords/tile.  This is equivalent to
	// lDelta * 2.  Remember that if tiling, lDelta is screenwidth in
	// bytes padded up to an even number of tilewidths.

	WriteReg32(PRI_STREAM_STRIDE,
		(((mode.bytesPerRow * 2) << 16) & 0x3FFFE000) |
		(mode.bytesPerRow & 0x00001fff));

	//  CR69, bit 7 = 1
	//  to use MM streams processor registers to control primary stream.

	WriteCrtcReg(0x69, 0x80, 0x80);

	WriteReg32(S3_GLOBAL_GBD_REG, bci_enable | S3_LITTLE_ENDIAN | S3_BD64);

	// If MS1NB style linear tiling mode.
	// bit MM850C[15] = 0 select NB linear tile mode.
	// bit MM850C[15] = 1 select MS-1 128-bit non-linear tile mode.

	uint32 ulTmp = ReadReg32(ADVANCED_FUNC_CTRL) | 0x8000;	// use MS-s style tile mode
	WriteReg32(ADVANCED_FUNC_CTRL, ulTmp);

	// CR88, bit 4 - Block write enabled/disabled.
	//
	// Note: Block write must be disabled when writing to tiled
	//		 memory. Even when writing to non-tiled memory, block
	//		 write should only be enabled for certain types of SGRAM.

	WriteCrtcReg(0x88, DISABLE_BLOCK_WRITE_2D, DISABLE_BLOCK_WRITE_2D);
}


static void 
Savage_SetGBD_3D(const DisplayModeEx& mode)
{
	int bci_enable = BCI_ENABLE;

	// MM81C0 and 81C4 are used to control primary stream.
	WriteReg32(PRI_STREAM_FBUF_ADDR0, 0);
	WriteReg32(PRI_STREAM_FBUF_ADDR1, 0);

	// Program Primary Stream Stride Register.
	//
	// Tell engine if tiling on or off, set primary stream stride, and
	// if tiling, set tiling bits/pixel and primary stream tile offset.
	// Note that tile offset (bits 16 - 29) must be scanline width in
	// bytes/128bytespertile * 256 Qwords/tile.  This is equivalent to
	// lDelta * 2.  Remember that if tiling, lDelta is screenwidth in
	// bytes padded up to an even number of tilewidths.

	WriteReg32(PRI_STREAM_STRIDE,
		(((mode.bytesPerRow * 2) << 16) & 0x3FFFE000) |
		(mode.bytesPerRow & 0x00001fff));

	// CR69, bit 7 = 1 to use MM streams processor registers to control primary
	// stream.

	WriteCrtcReg(0x69, 0x80, 0x80);

	WriteReg32(S3_GLOBAL_GBD_REG, bci_enable | S3_LITTLE_ENDIAN | S3_BD64);

	// If MS1NB style linear tiling mode.
	// bit MM850C[15] = 0 select NB linear tile mode.
	// bit MM850C[15] = 1 select MS-1 128-bit non-linear tile mode.

	uint32 ulTmp = ReadReg32(ADVANCED_FUNC_CTRL) | 0x8000;	// use MS-s style tile mode
	WriteReg32(ADVANCED_FUNC_CTRL, ulTmp);

	// CR88, bit 4 - Block write enabled/disabled.
	//
	// Note: Block write must be disabled when writing to tiled
	//		 memory.  Even when writing to non-tiled memory, block
	//		 write should only be enabled for certain types of SGRAM.

	WriteCrtcReg(0x88, DISABLE_BLOCK_WRITE_2D, DISABLE_BLOCK_WRITE_2D);
}


static void 
Savage_SetGBD_MX(const DisplayModeEx& mode)
{
	SharedInfo& si = *gInfo.sharedInfo;
	int bci_enable = BCI_ENABLE;

	// CR67_3:
	//	= 1  stream processor MMIO address and stride register
	//		 are used to control the primary stream.
	//	= 0  standard VGA address and stride registers
	//		 are used to control the primary streams.

	WriteCrtcReg(0x67, 0x08, 0x08);
	// IGA 2.
	WriteSeqReg(0x26, 0x4f);		// select IGA 2 read/writes
	WriteCrtcReg(0x67, 0x08, 0x08);
	WriteSeqReg(0x26, 0x40);		// select IGA 1

	// Set primary stream to bank 0 via reg CRCA.
	WriteCrtcReg(MEMORY_CTRL0_REG, 0x00, MEM_PS1 + MEM_PS2);

	// MM81C0 and 81C4 are used to control primary stream.
	WriteReg32(PRI_STREAM_FBUF_ADDR0,  si.frameBufferOffset & 0x7fffff);
	WriteReg32(PRI_STREAM_FBUF_ADDR1,  si.frameBufferOffset & 0x7fffff);
	WriteReg32(PRI_STREAM2_FBUF_ADDR0, si.frameBufferOffset & 0x7fffff);
	WriteReg32(PRI_STREAM2_FBUF_ADDR1, si.frameBufferOffset & 0x7fffff);

	// Program Primary Stream Stride Register.
	//
	// Tell engine if tiling on or off, set primary stream stride, and
	// if tiling, set tiling bits/pixel and primary stream tile offset.
	// Note that tile offset (bits 16 - 29) must be scanline width in
	// bytes/128bytespertile * 256 Qwords/tile.  This is equivalent to
	// lDelta * 2.  Remember that if tiling, lDelta is screenwidth in
	// bytes padded up to an even number of tilewidths.

	WriteReg32(PRI_STREAM_STRIDE,
		(((mode.bytesPerRow * 2) << 16) & 0x3FFF0000) |
		(mode.bytesPerRow & 0x00003fff));
	WriteReg32(PRI_STREAM2_STRIDE,
		(((mode.bytesPerRow * 2) << 16) & 0x3FFF0000) |
		(mode.bytesPerRow & 0x00003fff));

	WriteReg32(S3_GLOBAL_GBD_REG, bci_enable | S3_LITTLE_ENDIAN | S3_BD64);

	// CR78, bit 3  - Block write enabled(1)/disabled(0).
	//		 bit 2  - Block write cycle time(0:2 cycles,1: 1 cycle)
	//	Note:	Block write must be disabled when writing to tiled
	//			memory.	Even when writing to non-tiled memory, block
	//			write should only be enabled for certain types of SGRAM.

	WriteCrtcReg(0x78, 0xfb, 0xfb);
}


static void 
Savage_SetGBD_Super(const DisplayModeEx& mode)
{
	SharedInfo& si = *gInfo.sharedInfo;
	int bci_enable = BCI_ENABLE_TWISTER;

	// CR67_3:
	//	= 1 stream processor MMIO address and stride register
	//		are used to control the primary stream.
	//	= 0 standard VGA address and stride registers
	//		are used to control the primary streams.

	WriteCrtcReg(0x67, 0x08, 0x08);
	// IGA 2.
	WriteSeqReg(0x26, 0x4f);		// select IGA 2 read/writes
	WriteCrtcReg(0x67, 0x08, 0x08);
	WriteSeqReg(0x26, 0x40);		// select IGA 1

	// Load ps1 active registers as determined by MM81C0/81C4.
	// Load ps2 active registers as determined by MM81B0/81B4.

	WriteCrtcReg(0x65, 0x03, 0x03);

	// Program Primary Stream Stride Register.
	//
	// Tell engine if tiling on or off, set primary stream stride, and
	// if tiling, set tiling bits/pixel and primary stream tile offset.
	// Note that tile offset (bits 16 - 29) must be scanline width in
	// bytes/128bytespertile * 256 Qwords/tile.  This is equivalent to
	// lDelta * 2.  Remember that if tiling, lDelta is screenwidth in
	// bytes padded up to an even number of tilewidths.

	WriteReg32(PRI_STREAM_STRIDE,
		(((mode.bytesPerRow * 2) << 16) & 0x3FFF0000) |
		(mode.bytesPerRow & 0x00001fff));
	WriteReg32(PRI_STREAM2_STRIDE,
		(((mode.bytesPerRow * 2) << 16) & 0x3FFF0000) |
		(mode.bytesPerRow & 0x00001fff));

	// MM81C0 and 81C4 are used to control primary stream.
	WriteReg32(PRI_STREAM_FBUF_ADDR0, si.frameBufferOffset);
	WriteReg32(PRI_STREAM_FBUF_ADDR1, 0x80000000);
	WriteReg32(PRI_STREAM2_FBUF_ADDR0, (si.frameBufferOffset & 0xfffffffc) | 0x80000000);
	WriteReg32(PRI_STREAM2_FBUF_ADDR1, si.frameBufferOffset & 0xfffffffc);

	// Bit 28:block write disable.
	WriteReg32(S3_GLOBAL_GBD_REG, bci_enable | S3_BD64 | 0x10000000);
}


static void 
Savage_SetGBD_2000(const DisplayModeEx& mode)
{
	SharedInfo& si = *gInfo.sharedInfo;
	int bci_enable = BCI_ENABLE_TWISTER;

	// MM81C0 and 81B0 are used to control primary stream.
	WriteReg32(PRI_STREAM_FBUF_ADDR0, si.frameBufferOffset);
	WriteReg32(PRI_STREAM2_FBUF_ADDR0, si.frameBufferOffset);

	// Program Primary Stream Stride Register.
	//
	// Tell engine if tiling on or off, set primary stream stride, and
	// if tiling, set tiling bits/pixel and primary stream tile offset.
	// Note that tile offset (bits 16 - 29) must be scanline width in
	// bytes/128bytespertile * 256 Qwords/tile.  This is equivalent to
	// lDelta * 2.  Remember that if tiling, lDelta is screenwidth in
	// bytes padded up to an even number of tilewidths.

	WriteReg32(PRI_STREAM_STRIDE,  ((mode.bytesPerRow << 4) & 0x7ff0));
	WriteReg32(PRI_STREAM2_STRIDE, ((mode.bytesPerRow << 4) & 0x7ff0));

	// CR67_3:
	//	= 1 stream processor MMIO address and stride register
	//		are used to control the primary stream.
	//	= 0 standard VGA address and stride registers
	//		are used to control the primary streams

	WriteCrtcReg(0x67, 0x08, 0x08);

	// Bit 28:block write disable.
	WriteReg32(S3_GLOBAL_GBD_REG, bci_enable | S3_BD64 | 0x10000000);

	WriteCrtcReg(0x73, 0x00, 0x20);		// CR73 bit 5 = 0 block write disable
}



static void 
Savage_SetGBD(const DisplayModeEx& mode)
{
	SharedInfo& si = *gInfo.sharedInfo;

	VerticalRetraceWait();

	switch (gInfo.sharedInfo->chipType) {
		case S3_SAVAGE_3D:
			Savage_SetGBD_3D(mode);
			break;

		case S3_SAVAGE_MX:
			Savage_SetGBD_MX(mode);
			break;

		case S3_SAVAGE4:
		case S3_PROSAVAGE:
		case S3_TWISTER:
		case S3_PROSAVAGE_DDR:
			Savage_SetGBD_Twister(mode);
			break;

		case S3_SUPERSAVAGE:
			Savage_SetGBD_Super(mode);
			break;

		case S3_SAVAGE2000:
			Savage_SetGBD_2000(mode);
			break;
	}

	WriteCrtcReg(0x50, 0xc1, 0xc1);		// CR50, bit 7,6,0 = 111, Use GBD

	// Set up the Global Bitmap Descriptor used for BCI.
	// Do not enable block_write because we can't determine if the memory
	// type is a certain type of SGRAM for which block write can be used.
	//	bit 24~25: tile format,  00 = linear
	//	bit 28: block write disble/enable, 0 = disable, 1 = enable

	si.globalBitmapDesc = mode.timing.h_display | (mode.bpp << 16)
		| TILE_FORMAT_LINEAR | BCI_BD_BW_DISABLE | S3_BD64;	// disable block write
}


static void 
Savage_Initialize2DEngine(const DisplayModeEx& mode)
{
	SharedInfo& si = *gInfo.sharedInfo;

	WriteCrtcReg(0x40, 0x01);	// enable graphics engine
	WriteCrtcReg(0x31, 0x0c);	// turn on 16-bit register access

	// Setup plane masks.
	WriteReg32(0x8128, ~0);		// enable all write planes
	WriteReg32(0x812C, ~0);		// enable all read planes
	WriteReg16(0x8134, 0x27);
	WriteReg16(0x8136, 0x07);

	switch (si.chipType) {
		case S3_SAVAGE_3D:
		case S3_SAVAGE_MX:

			WriteReg32(0x48C18, ReadReg32(0x48C18) & 0x3FF0);	// Disable BCI

			// Setup BCI command overflow buffer.  0x48c14
			// Bits 0-11  = Bits 22-11 of the Command Buffer Offset.
			// Bits 12-28 = Total number of entries in the command buffer(Read only).
			// Bits 29-31 = COB size index, 111 = 32K entries or 128K bytes
			WriteReg32(0x48C14, (si.cobOffset >> 11) | (si.cobSizeIndex << 29));

			// Program shadow status update.
			WriteReg32(0x48C10, 0x78207220);

			WriteReg32(0x48C0C, 0);
			// Enable BCI and command overflow buffer.
			WriteReg32(0x48C18, ReadReg32(0x48C18) | 0x0C);
			break;

		case S3_SAVAGE4:
		case S3_PROSAVAGE:
		case S3_TWISTER:
		case S3_PROSAVAGE_DDR:
		case S3_SUPERSAVAGE:

			// Some Savage4 and ProSavage chips have coherency problems with
			// respect to the Command Overflow Buffer (COB); thus, do not
			// enable the COB.

			WriteReg32(0x48C18, ReadReg32(0x48C18) & 0x3FF0);	// Disable BCI
			WriteReg32(0x48C10, 0x00700040);
			WriteReg32(0x48C0C, 0);
			WriteReg32(0x48C18, ReadReg32(0x48C18) | 0x08);		// enable BCI without COB

			break;

		case S3_SAVAGE2000:

			// Disable BCI.
			WriteReg32(0x48C18, 0);
			// Setup BCI command overflow buffer.
			WriteReg32(0x48C18, (si.cobOffset >> 7) | (si.cobSizeIndex));
			// Disable shadow status update.
			WriteReg32(0x48A30, 0);
			// Enable BCI and command overflow buffer.
			WriteReg32(0x48C18, ReadReg32(0x48C18) | 0x00280000);

			break;
	}

	// Use and set global bitmap descriptor.

	Savage_SetGBD(mode);
}


static void 
Savage_GEReset(const DisplayModeEx& mode)
{
	gInfo.WaitIdleEmpty();
	snooze(10000);

	for (int r = 1; r < 10; r++) {
		bool bSuccess = false;

		WriteCrtcReg(0x66, 0x02, 0x02);
		snooze(10000);
		WriteCrtcReg(0x66, 0x00, 0x02);
		snooze(10000);

		gInfo.WaitIdleEmpty();

		WriteReg32(DEST_SRC_STR, mode.bytesPerRow << 16 | mode.bytesPerRow);

		snooze(10000);
		switch (gInfo.sharedInfo->chipType) {
			case S3_SAVAGE_3D:
			case S3_SAVAGE_MX:
				bSuccess = (STATUS_WORD0 & 0x0008ffff) == 0x00080000;
				break;

			case S3_SAVAGE4:
			case S3_PROSAVAGE:
			case S3_PROSAVAGE_DDR:
			case S3_TWISTER:
			case S3_SUPERSAVAGE:
				bSuccess = (ALT_STATUS_WORD0 & 0x0081ffff) == 0x00800000;
				break;

			case S3_SAVAGE2000:
				bSuccess = (ALT_STATUS_WORD0 & 0x008fffff) == 0;
				break;
		}

		if (bSuccess)
			break;

		snooze(10000);
		TRACE("Savage_GEReset(), restarting S3 graphics engine reset %2d ...\n", r);
	}

	// At this point, the FIFO is empty and the engine is idle.

	WriteReg32(SRC_BASE, 0);
	WriteReg32(DEST_BASE, 0);
	WriteReg32(CLIP_L_R, ((0) << 16) | mode.timing.h_display);
	WriteReg32(CLIP_T_B, ((0) << 16) | mode.timing.v_display);
	WriteReg32(MONO_PAT_0, ~0);
	WriteReg32(MONO_PAT_1, ~0);

	Savage_SetGBD(mode);
}


static void 
Savage_CalcClock(long freq, int min_m,
				int min_n1, int max_n1,
				int min_n2, int max_n2,
				long freq_min, long freq_max,
				unsigned int *mdiv, unsigned int *ndiv, unsigned int *r)
{
	uint8 best_n1 = 16 + 2, best_n2 = 2, best_m = 125 + 2;

	double ffreq = freq / 1000.0 / BASE_FREQ;
	double ffreq_max = freq_max / 1000.0 / BASE_FREQ;
	double ffreq_min = freq_min / 1000.0 / BASE_FREQ;

	if (ffreq < ffreq_min / (1 << max_n2)) {
		TRACE("Savage_CalcClock() invalid frequency %1.3f Mhz\n", ffreq * BASE_FREQ);
		ffreq = ffreq_min / (1 << max_n2);
	}
	if (ffreq > ffreq_max / (1 << min_n2)) {
		TRACE("Savage_CalcClock() invalid frequency %1.3f Mhz\n", ffreq * BASE_FREQ);
		ffreq = ffreq_max / (1 << min_n2);
	}

	// Work out suitable timings.

	double best_diff = ffreq;

	for (uint8 n2 = min_n2; n2 <= max_n2; n2++) {
		for (uint8 n1 = min_n1 + 2; n1 <= max_n1 + 2; n1++) {
			int m = (int)(ffreq * n1 * (1 << n2) + 0.5);
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

	*ndiv = best_n1 - 2;
	*r = best_n2;
	*mdiv = best_m - 2;
}


static void 
Savage_WriteMode(const DisplayModeEx& mode, const SavageRegRec& regRec)
{
	TRACE("Savage_WriteMode() begin\n");

	SharedInfo& si = *gInfo.sharedInfo;

	gInfo.WaitIdleEmpty();

	WriteMiscOutReg(0x23);
	WriteCrtcReg(0x38, 0x48);		// unlock sys regs CR20~CR3F
	WriteCrtcReg(0x39, 0xa0);		// unlock sys regs CR40~CRFF
	WriteSeqReg(0x08, 0x06);		// unlock sequencer regs SR09~SRFF

	if ( ! S3_SAVAGE_MOBILE_SERIES(si.chipType))
		Savage_Initialize2DEngine(mode);

	if (ReadCrtcReg(0x66) & 0x01) {
		Savage_GEReset(mode);	// reset GE to make sure nothing is going on
	}

	WriteCrtcReg(0x67, regRec.CR67 & ~0x0e); // no STREAMS yet old and new

	// Set register SR19 to zero so that the ProSavage chips will start up
	// when booting under BeOS using the default boot screen, and set register
	// CR5F to zero so that the ProSavage chips will start up when Haiku boot
	// screen had a depth of 32 bits/pixel

	if (si.chipType == S3_PROSAVAGE || si.chipType == S3_TWISTER) {
		WriteSeqReg(0x19, 0);
		WriteCrtcReg(0x5f, 0);
	}

	// Clear bit 3 in SR30 so that Savage MX chip will startup.  If bit 3 is
	// not cleared, it will startup only if booting under BeOS using the
	// default boot screen or the boot screen resolution matches the resolution
	// of the mode currently being set.

	if (si.chipType == S3_SAVAGE_MX)
		WriteSeqReg(0x30, 0x00, 0x08);

	// Set extended regs.

	WriteCrtcReg(0x66, regRec.CR66);
	WriteCrtcReg(0x3a, regRec.CR3A);
	WriteCrtcReg(0x58, regRec.CR58);
	WriteCrtcReg(0x53, regRec.CR53 & 0x7f);

	// If Savage IX/MX or SuperSavage, set SR54 & SR56 to 0x10 so that when
	// resolutions are set where the width and/or height is less than the
	// native resolution of the attached LCD display, the chip will not expand
	// the display to fill the screen.  That is, if a resolution is set to
	// 640x480, it will use only 640x480 pixels for the display.  When the chip
	// expands the display, text is much less readable.

	if (S3_SAVAGE_MOBILE_SERIES(si.chipType)) {
		WriteSeqReg(0x54, 0x10);
		WriteSeqReg(0x56, 0x10);
	}

	// Set the standard CRTC vga regs.

	WriteCrtcReg(0x11, 0x00, 0x80);		// unlock CRTC reg's 0-7 by clearing bit 7 of cr11

	for (int j = 0; j < NUM_ELEMENTS(regRec.CRTC); j++)
		WriteCrtcReg(j, regRec.CRTC[j]);

	// Setup HSYNC & VSYNC polarity.

	uint8 temp = ((ReadMiscOutReg() & 0x3f) | 0x0c);

	if (!(mode.timing.flags & B_POSITIVE_HSYNC))
		temp |= 0x40;
	if (!(mode.timing.flags & B_POSITIVE_VSYNC))
		temp |= 0x80;

	WriteMiscOutReg(temp);

	// Extended mode timing registers.
	WriteCrtcReg(0x53, regRec.CR53);
	WriteCrtcReg(0x5d, regRec.CR5D);
	WriteCrtcReg(0x5e, regRec.CR5E);
	WriteCrtcReg(0x3b, regRec.CR3B);
	WriteCrtcReg(0x3c, regRec.CR3C);
	WriteCrtcReg(0x43, regRec.CR43);
	WriteCrtcReg(0x65, regRec.CR65);

	// Restore the desired video mode with cr67.
	WriteCrtcReg(0x67, regRec.CR67 & ~0x0e);	// no streams for new and old streams engines

	// Other mode timing and extended regs.
	WriteCrtcReg(0x34, regRec.CR34);
	WriteCrtcReg(0x42, regRec.CR42);
	WriteCrtcReg(0x45, regRec.CR45);
	WriteCrtcReg(0x50, regRec.CR50);
	WriteCrtcReg(0x51, regRec.CR51);

	// Memory timings.
	VerticalRetraceWait();
	WriteCrtcReg(0x69, regRec.CR69);

	WriteCrtcReg(0x33, regRec.CR33);
	WriteCrtcReg(0x86, regRec.CR86);
	WriteCrtcReg(0x88, regRec.CR88);
	WriteCrtcReg(0x90, regRec.CR90);
	WriteCrtcReg(0x91, regRec.CR91);

	if (si.chipType == S3_SAVAGE4)
		WriteCrtcReg(0xb0, regRec.CRB0);

	WriteSeqReg(0x1b, regRec.SR1B);

	if ( ! (S3_SAVAGE_MOBILE_SERIES(si.chipType) &&  si.displayType == MT_LCD)) {
		// Set extended seq regs for dclk.
		WriteSeqReg(0x12, regRec.SR12);
		WriteSeqReg(0x13, regRec.SR13);
		WriteSeqReg(0x29, regRec.SR29);

		// Load new m, n pll values for dclk & mclk.
		temp = ReadSeqReg(0x15) & ~0x20;
		WriteSeqReg(0x15, temp);
		WriteSeqReg(0x15, temp | 0x20);
		WriteSeqReg(0x15, temp);
		snooze(100);
	}

	// Now write out cr67 in full, possibly starting STREAMS.
	VerticalRetraceWait();
	WriteCrtcReg(0x67, regRec.CR67);

	uint8 cr66 = ReadCrtcReg(0x66);
	WriteCrtcReg(0x66, cr66 | 0x80);
	uint8 cr3a = ReadCrtcReg(0x3a);
	WriteCrtcReg(0x3a, cr3a | 0x80);

	Savage_GEReset(mode);

	WriteCrtcReg(0x66, cr66);
	WriteCrtcReg(0x3a, cr3a);

	Savage_Initialize2DEngine(mode);

	WriteCrtcReg(0x40, 0x01);		// enable graphics engine

	Savage_SetGBD(mode);

	TRACE("Savage_WriteMode() done\n");

	return;
}


static bool 
Savage_ModeInit(const DisplayModeEx& mode)
{
	TRACE("Savage_ModeInit(%dx%d, %d kHz)\n",
		mode.timing.h_display, mode.timing.v_display, mode.timing.pixel_clock);

	SharedInfo& si = *gInfo.sharedInfo;
	SavageRegRec regRec;

	int horizScaleFactor = 1;

	if (mode.bpp == 16 && si.chipType == S3_SAVAGE_3D)
		horizScaleFactor = 2;

	InitCrtcTimingValues(mode, horizScaleFactor, regRec.CRTC,
			regRec.CR3B, regRec.CR3C, regRec.CR5D, regRec.CR5E);
	regRec.CRTC[0x17] = 0xEB;

	int dclk = mode.timing.pixel_clock;
	regRec.CR67 = 0x00;

	switch (mode.bpp) {
		case 8:
			if ((si.chipType == S3_SAVAGE2000) && (dclk >= 230000))
				regRec.CR67 = 0x10;		// 8bpp, 2 pixels/clock
			else
				regRec.CR67 = 0x00;		// 8bpp, 1 pixel/clock
			break;

		case 15:
			if (S3_SAVAGE_MOBILE_SERIES(si.chipType)
				|| ((si.chipType == S3_SAVAGE2000) && (dclk >= 230000)))
				regRec.CR67 = 0x30;		// 15bpp, 2 pixel/clock
			else
				regRec.CR67 = 0x20;		// 15bpp, 1 pixels/clock
			break;

		case 16:
			if (S3_SAVAGE_MOBILE_SERIES(si.chipType)
				|| ((si.chipType == S3_SAVAGE2000) && (dclk >= 230000)))
				regRec.CR67 = 0x50;		// 16bpp, 2 pixel/clock
			else
				regRec.CR67 = 0x40;		// 16bpp, 1 pixels/clock
			break;

		case 32:
			regRec.CR67 = 0xd0;
			break;
	}

	regRec.CR3A = (ReadCrtcReg(0x3a) & 0x7f) | 0x15;
	regRec.CR53 = 0x00;
	regRec.CR66 = 0x89;
	regRec.CR58 = (ReadCrtcReg(0x58) & 0x80) | 0x13;

	regRec.SR1B = ReadSeqReg(0x1b) | 0x10;	// enable 8-bit Color Lookup Table

	regRec.CR43 = regRec.CR45 = regRec.CR65 = 0x00;

	unsigned int m, n, r;
	Savage_CalcClock(dclk, 1, 1, 127, 0, 4, 180000, 360000, &m, &n, &r);
	regRec.SR12 = (r << 6) | (n & 0x3f);
	regRec.SR13 = m & 0xff;
	regRec.SR29 = (r & 4) | (m & 0x100) >> 5 | (n & 0x40) >> 2;

	TRACE("CalcClock, m: %d  n: %d  r: %d\n", m, n, r);

	regRec.CR42 = 0x00;
	regRec.CR34 = 0x10;

	int width = mode.bytesPerRow / 8;
	regRec.CR91 = 0xff & width;
	regRec.CR51 = (0x300 & width) >> 4;
	regRec.CR90 = 0x80 | (width >> 8);

	// Set frame buffer description.

	if (mode.bpp <= 8)
		regRec.CR50 = 0;
	else if (mode.bpp <= 16)
		regRec.CR50 = 0x10;
	else
		regRec.CR50 = 0x30;

	width = mode.timing.h_display;		// width of display in pixels

	if (width == 640)
		regRec.CR50 |= 0x40;
	else if (width == 800)
		regRec.CR50 |= 0x80;
	else if (width == 1024)
		regRec.CR50 |= 0x00;
	else if (width == 1152)
		regRec.CR50 |= 0x01;
	else if (width == 1280)
		regRec.CR50 |= 0xc0;
	else if (width == 1600)
		regRec.CR50 |= 0x81;
	else
		regRec.CR50 |= 0xc1;	// use GBD

	if (S3_SAVAGE_MOBILE_SERIES(si.chipType))
		regRec.CR33 = 0x00;
	else
		regRec.CR33 = 0x08;

	regRec.CR69 = 0;
	regRec.CR86 = ReadCrtcReg(0x86) | 0x08;
	regRec.CR88 = ReadCrtcReg(0x88) | DISABLE_BLOCK_WRITE_2D;
	regRec.CRB0 = ReadCrtcReg(0xb0) | 0x80;

	Savage_WriteMode(mode, regRec);		// write registers to set mode

	return true;
}



bool 
Savage_SetDisplayMode(const DisplayModeEx& mode)
{
	// The code to actually configure the display.
	// All the error checking must be done in ProposeDisplayMode(),
	// and assume that the mode values we get here are acceptable.

	WriteSeqReg(0x01, 0x20, 0x20);		// blank the screen

	if ( ! Savage_ModeInit(mode)) {
		TRACE("Savage_ModeInit() failed\n");
		return false;
	}

	Savage_AdjustFrame(mode);

	WriteSeqReg(0x01, 0x00, 0x20);		// unblank the screen

	return true;
}



void
Savage_AdjustFrame(const DisplayModeEx& mode)
{
	// Adjust start address in frame buffer.

	SharedInfo& si = *gInfo.sharedInfo;

	int address = (mode.v_display_start * mode.virtual_width)
			+ ((mode.h_display_start & ~0x3F) * (mode.bpp / 8));
	address &= ~0x1F;
	address += si.frameBufferOffset;

	switch (si.chipType) {
		case S3_SAVAGE_MX:
			WriteReg32(PRI_STREAM_FBUF_ADDR0, address & 0xFFFFFFFC);
			WriteReg32(PRI_STREAM_FBUF_ADDR1, address & 0xFFFFFFFC);
			break;
		case S3_SUPERSAVAGE:
			WriteReg32(PRI_STREAM_FBUF_ADDR0, 0x80000000);
			WriteReg32(PRI_STREAM_FBUF_ADDR1, address & 0xFFFFFFF8);
			break;
		case S3_SAVAGE2000:
			// Certain Y values seems to cause havoc, not sure why
			WriteReg32(PRI_STREAM_FBUF_ADDR0, (address & 0xFFFFFFF8));
			WriteReg32(PRI_STREAM2_FBUF_ADDR0, (address & 0xFFFFFFF8));
			break;
		default:
			WriteReg32(PRI_STREAM_FBUF_ADDR0, address | 0xFFFFFFFC);
			WriteReg32(PRI_STREAM_FBUF_ADDR1, address | 0x80000000);
			break;
	}

	return;
}


void 
Savage_SetIndexedColors(uint count, uint8 first, uint8* colorData, uint32 flags)
{
	// Set the indexed color palette for 8-bit color depth mode.

	(void)flags;		// avoid compiler warning for unused arg

	if (gInfo.sharedInfo->displayMode.space != B_CMAP8)
		return ;

	while (count--) {
		WriteIndexedColor(first++,	// color index
			colorData[0],			// red
			colorData[1],			// green
			colorData[2]);			// blue
		colorData += 3;
	}
}
