/*
	Haiku S3 Trio64 driver adapted from the X.org S3 driver.

	Copyright 2001	Ani Joshi <ajoshi@unixbox.com>

	Copyright 2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2008
*/


#include "accel.h"
#include "trio64.h"


#define BASE_FREQ		14.31818	// MHz


static void
Trio64_CalcClock(long freq, int min_m, int min_n1, int max_n1,
				int min_n2, int max_n2, long freq_min, long freq_max,
				uint8* mdiv, uint8* ndiv)
{
	uint8 best_n1 = 18, best_n2 = 2, best_m = 127;

	double ffreq = freq / 1000.0 / BASE_FREQ;
	double ffreq_min = freq_min / 1000.0 / BASE_FREQ;
	double ffreq_max = freq_max / 1000.0 / BASE_FREQ;

	if (ffreq < ffreq_min / (1 << max_n2)) {
		TRACE("Trio64_CalcClock() invalid frequency %1.3f Mhz [freq >= %1.3f Mhz]\n",
			   ffreq*BASE_FREQ, ffreq_min*BASE_FREQ / (1 << max_n2));
		ffreq = ffreq_min / (1 << max_n2);
	}
	if (ffreq > ffreq_max / (1 << min_n2)) {
		TRACE("Trio64_CalcClock() invalid frequency %1.3f Mhz [freq <= %1.3f Mhz]\n",
			   ffreq*BASE_FREQ, ffreq_max*BASE_FREQ / (1 << min_n2));
		ffreq = ffreq_max / (1 << min_n2);
	}

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

	if (max_n1 == 63)
		*ndiv = (best_n1 - 2) | (best_n2 << 6);
	else
		*ndiv = (best_n1 - 2) | (best_n2 << 5);
	*mdiv = best_m - 2;
}



static bool
Trio64_ModeInit(const DisplayModeEx& mode)
{
	SharedInfo& si = *gInfo.sharedInfo;

	TRACE("Trio64_ModeInit(%d x %d, %d KHz)\n",
		mode.timing.h_display, mode.timing.v_display, mode.timing.pixel_clock);

	uint32 videoRamMB = si.videoMemSize / (1024 * 1024);	// MB's of video RAM

	WriteCrtcReg(0x38, 0x48);			// unlock sys regs
	WriteCrtcReg(0x39, 0xa5);			// unlock sys regs
	WriteSeqReg(0x08, 0x06);			// unlock extended sequencer regs

	WriteCrtcReg(0x45, 0x00, 0x01);		// turn off hardware cursor

	uint8 sr12, sr13;
	Trio64_CalcClock(mode.timing.pixel_clock, 1, 1, 31, 0, 3, 135000, 270000,
					 &sr13, &sr12);

	// Set clock registers.

	WriteSeqReg(0x12, sr12);
	WriteSeqReg(0x13, sr13);

	// Activate clock

	uint8 tmp = ReadSeqReg(0x15) & ~0x21;
	WriteSeqReg(0x15, tmp | 0x02);
	WriteSeqReg(0x15, tmp | 0x22);
	WriteSeqReg(0x15, tmp | 0x02);

	uint8 cr33 = ReadCrtcReg(0x33) & ~0x28;
	uint8 cr50 = 0;
	uint8 pixmux = 0;

	if (si.chipType == S3_TRIO64_V2)
		cr33 |= 0x20;

	switch (mode.bpp) {
		case 8:
			break;
		case 15:
			cr33 |= 0x08;
			cr50 = 0x10;
			pixmux = 0x30;
			break;
		case 16:
			cr33 |= 0x08;
			cr50 = 0x10;
			pixmux = 0x50;
			break;
		case 32:
			cr50 = 0x30;
			pixmux = 0xd0;
			break;
	}

	bool bDisableAccelFuncs = false;

	switch (mode.timing.h_display) {
		case 640:
			cr50 |= 0x40;
			break;
		case 800:
			cr50 |= 0x80;
			break;
		case 1024:
			cr50 |= 0x00;
			break;
		case 1152:
			cr50 |= 0x01;
			break;
		case 1280:
			cr50 |= 0xc0;
			break;
		case 1600:
			cr50 |= 0x81;
			break;
		default:
			bDisableAccelFuncs = true;	// use app_server default accel functions
			break;
	}

	WriteCrtcReg(0x33, cr33);
	WriteCrtcReg(0x50, cr50);		// set number of bits per pixel & display width
	WriteCrtcReg(0x67, pixmux);		// set pixel format

	WriteSeqReg(0x15, 0x00, 0x10);		// turn off pixel multiplex
	WriteSeqReg(0x18, 0x00, 0x80);

	// Note that the 2D acceleration (drawing) functions in this accelerant work
	// only with the display widths defined in the above switch statement.  For
	// the other widths, the default functions in the app_server will be used.

	si.bDisableAccelDraw = bDisableAccelFuncs;

	// Set the standard CRTC vga regs.

	uint8 crtc[25], cr3b, cr3c, cr5d, cr5e;

	InitCrtcTimingValues(mode, (mode.bpp > 8) ? 2 : 1, crtc, cr3b, cr3c, cr5d, cr5e);
	crtc[0x17] = 0xe3;

	WriteCrtcReg(0x11, 0x00, 0x80);	// unlock CRTC reg's 0-7 by clearing bit 7 of cr11

	for (int k = 0; k < NUM_ELEMENTS(crtc); k++) {
		WriteCrtcReg(k, crtc[k]);
	}

	WriteCrtcReg(0x3b, cr3b);
	WriteCrtcReg(0x3c, cr3c);
	WriteCrtcReg(0x5d, cr5d);
	WriteCrtcReg(0x5e, cr5e);

	uint8 miscOutReg = 0x2f;

	if ( ! (mode.timing.flags & B_POSITIVE_HSYNC))
		miscOutReg |= 0x40;
	if ( ! (mode.timing.flags & B_POSITIVE_VSYNC))
		miscOutReg |= 0x80;

	WriteMiscOutReg(miscOutReg);

	uint8 cr58;
	if (videoRamMB <= 1)
		cr58 = 0x01;
	else if (videoRamMB <= 2)
		cr58 = 0x02;
	else
		cr58 = 0x03;

	WriteCrtcReg(0x58, cr58 | 0x10, 0x13);	// enable linear addressing & set memory size

	WriteCrtcReg(0x31, 0x08);
	WriteCrtcReg(0x32, 0x00);
	WriteCrtcReg(0x34, 0x10);
	WriteCrtcReg(0x3a, 0x15);

	WriteCrtcReg(0x51, mode.bytesPerRow >> 7, 0x30);
	WriteCrtcReg(0x53, 0x18, 0x18);

	int n = 255;
	int clock2 = mode.timing.pixel_clock * (mode.bpp / 8);
	if (videoRamMB < 2)
		clock2 *= 2;
	int m = (int)((gInfo.sharedInfo->mclk / 1000.0 * .72 + 16.867) * 89.736
			/ (clock2 / 1000.0 + 39) - 21.1543);
	if (videoRamMB < 2)
		m /= 2;
	if (m > 31)
		m = 31;
	else if (m < 0) {
		m = 0;
		n = 16;
	}

	if (n < 0)
		n = 0;
	else if (n > 255)
		n = 255;

	WriteCrtcReg(0x54, m << 3);
	WriteCrtcReg(0x60, n);

	WriteCrtcReg(0x42, 0x00, 0x20);		// disable interlace mode
	WriteCrtcReg(0x66, 0x89, 0x8f);

	WriteReg16(ADVFUNC_CNTL, 0x0001);		// enable enhanced functions

	WriteReg16(SUBSYS_CNTL, 0x8000);		// reset graphics engine
	WriteReg16(SUBSYS_CNTL, 0x4000);		// enable graphics engine
	ReadReg16(SUBSYS_STAT);

	WriteReg16(MULTIFUNC_CNTL, 0x5000 | 0x0004 | 0x000c);

	gInfo.WaitQueue(5);
	WriteReg16(MULTIFUNC_CNTL, SCISSORS_L | 0);
	WriteReg16(MULTIFUNC_CNTL, SCISSORS_T | 0);
	WriteReg16(MULTIFUNC_CNTL, SCISSORS_R | (mode.timing.h_display - 1));
	WriteReg16(MULTIFUNC_CNTL, SCISSORS_B | ((si.maxFrameBufferSize / mode.bytesPerRow) - 1));

	WriteReg32(WRT_MASK, ~0);		// enable all planes

	TRACE("Trio64_ModeInit() exit\n");
	return true;
}


bool 
Trio64_SetDisplayMode(const DisplayModeEx& mode)
{
	// The code to actually configure the display.
	// All the error checking must be done in ProposeDisplayMode(),
	// and assume that the mode values we get here are acceptable.

	WriteSeqReg(0x01, 0x20, 0x20);		// blank the screen

	if ( ! Trio64_ModeInit(mode)) {
		TRACE("Trio64_ModeInit() failed\n");
		return false;
	}

	Trio64_AdjustFrame(mode);

	WriteSeqReg(0x01, 0x00, 0x20);		// unblank the screen

	return true;
}



void
Trio64_AdjustFrame(const DisplayModeEx& mode)
{
	// Adjust start address in frame buffer.

	int base = (((mode.v_display_start * mode.virtual_width + mode.h_display_start)
			* (mode.bpp / 8)) >> 2) & ~1;
	base += gInfo.sharedInfo->frameBufferOffset;

	WriteCrtcReg(0x0c, (base >> 8) & 0xff);
	WriteCrtcReg(0x0d, base & 0xff);
	WriteCrtcReg(0x31, base >> 12, 0x30);	// put bits 16-17 in bits 4-5 of CR31
	WriteCrtcReg(0x51, base >> 18, 0x03);	// put bits 18-19 in bits 0-1 of CR51
}


void 
Trio64_SetIndexedColors(uint count, uint8 first, uint8* colorData, uint32 flags)
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
