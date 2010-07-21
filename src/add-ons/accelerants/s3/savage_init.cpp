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



static bool
Savage_GetColorSpaceParams(int colorSpace, uint32& bitsPerPixel, uint32& maxPixelClock)
{
	// Get parameters for a color space which is supported by the Savage chips.
	// Argument maxPixelClock is in KHz.
	// Return true if the color space is supported;  else return false.

	switch (colorSpace) {
		case B_RGB32:
			bitsPerPixel = 32;
			maxPixelClock = 220000;
			break;
		case B_RGB16:
			bitsPerPixel = 16;
			maxPixelClock = 250000;
			break;
		case B_CMAP8:
			bitsPerPixel = 8;
			maxPixelClock = 250000;
			break;
		default:
			TRACE("Unsupported color space: 0x%X\n", colorSpace);
			return false;
	}

	return true;
}


// Wait until "v" queue entries are free.

static void 
WaitQueue3D(uint32 v)
{
	uint32 slots = MAXFIFO - v;
	while ((STATUS_WORD0 & 0x0000ffff) > slots);
}


static void 
WaitQueue4(uint32 v)
{
	uint32 slots = MAXFIFO - v;
	while ((ALT_STATUS_WORD0 & 0x001fffff) > slots);
}


static void 
WaitQueue2K(uint32 v)
{
	uint32 slots = MAXFIFO - v;
	while ((ALT_STATUS_WORD0 & 0x000fffff) > slots);
}


// Wait until GP is idle and queue is empty.

static void 
WaitIdleEmpty3D()
{
	while ((STATUS_WORD0 & 0x0008ffff) != 0x80000);
}


static void 
WaitIdleEmpty4()
{
	while ((ALT_STATUS_WORD0 & 0x00e1ffff) != 0x00e00000) ;
}


static void 
WaitIdleEmpty2K()
{
	while ((ALT_STATUS_WORD0 & 0x009fffff) != 0);
}


static void
Savage_GetPanelInfo()
{
	SharedInfo& si = *gInfo.sharedInfo;

	enum ACTIVE_DISPLAYS {		// these are the bits in CR6B
		ActiveCRT = 0x01,
		ActiveLCD = 0x02,
		ActiveTV	= 0x04,
		ActiveCRT2 = 0x20,
		ActiveDUO = 0x80
	};

	// Check LCD panel information.

	uint8 cr6b = ReadCrtcReg(0x6b);

	int panelX = (ReadSeqReg(0x61) + ((ReadSeqReg(0x66) & 0x02) << 7) + 1) * 8;
	int panelY =  ReadSeqReg(0x69) + ((ReadSeqReg(0x6e) & 0x70) << 4) + 1;

	// A Savage IX/MV in a Thinkpad T22 or SuperSavage in a Thinkpad T23 with
	// a 1400x1050 display will return a width of 1408;  thus, in this case,
	// set the width to the correct value of 1400.

	if (panelX == 1408)
		panelX = 1400;

	const char* sTechnology;

	if ((ReadSeqReg(0x39) & 0x03) == 0)
		sTechnology = "TFT";
	else if ((ReadSeqReg(0x30) & 0x01) == 0)
		sTechnology = "DSTN";
	else
		sTechnology = "STN";

	TRACE("%dx%d %s LCD panel detected %s\n", panelX, panelY, sTechnology,
			 cr6b & ActiveLCD ? "and active" : "but not active");

	if (cr6b & ActiveLCD) {
		TRACE("Limiting max video mode to %dx%d\n", panelX, panelY);
		si.panelX = panelX;
		si.panelY = panelY;
	} else {
		si.displayType = MT_CRT;
	}
}


status_t 
Savage_Init(void)
{
	TRACE("Savage_Init()\n");

	SharedInfo& si = *gInfo.sharedInfo;

	// MMIO should be automatically enabled for Savage chips;  thus, use MMIO
	// to enable VGA and turn color on.

	WriteReg8(VGA_ENABLE + 0x8000, ReadReg8(VGA_ENABLE + 0x8000) | 0x01);
	WriteMiscOutReg(ReadMiscOutReg() | 0x01);		// turn color on

	if (si.chipType >= S3_SAVAGE4)
		WriteCrtcReg(0x40, 0x01, 0x01);

	WriteCrtcReg(0x11, 0x00, 0x80);	// unlock CRTC reg's 0-7 by clearing bit 7 of cr11
	WriteCrtcReg(0x38, 0x48);		// unlock sys regs CR20~CR3F
	WriteCrtcReg(0x39, 0xa0);		// unlock sys regs CR40~CRFF
	WriteSeqReg(0x08, 0x06);		// unlock sequencer regs SR09~SRFF

	WriteCrtcReg(0x40, 0x00, 0x01);
	WriteCrtcReg(0x38, 0x48);		// unlock sys regs CR20~CR3F

	// Compute the amount of video memory and offscreen memory.

	static const uint8 RamSavage3D[] = { 8, 4, 4, 2 };
	static		 uint8 RamSavage4[] =  { 2, 4, 8, 12, 16, 32, 64, 32 };
	static const uint8 RamSavageMX[] = { 2, 8, 4, 16, 8, 16, 4, 16 };
	static const uint8 RamSavageNB[] = { 0, 2, 4, 8, 16, 32, 16, 2 };
	uint32 ramSizeMB = 0;		// memory size in megabytes

	uint8 cr36 = ReadCrtcReg(0x36);		// get amount of video ram

	switch (si.chipType) {
		case S3_SAVAGE_3D:
			ramSizeMB = RamSavage3D[ (cr36 & 0xC0) >> 6 ];
			break;

		case S3_SAVAGE4:
			// The Savage4 has one ugly special case to consider.  On
			// systems with 4 banks of 2Mx32 SDRAM, the BIOS says 4MB
			// when it really means 8MB.	Why do it the same when you
			// can do it different...
			if ((ReadCrtcReg(0x68) & 0xC0) == (0x01 << 6))
				RamSavage4[1] = 8;

			// FALL THROUGH

		case S3_SAVAGE2000:
			ramSizeMB = RamSavage4[ (cr36 & 0xE0) >> 5 ];
			break;

		case S3_SAVAGE_MX:
		case S3_SUPERSAVAGE:
			ramSizeMB = RamSavageMX[ (cr36 & 0x0E) >> 1 ];
			break;

		case S3_PROSAVAGE:
		case S3_PROSAVAGE_DDR:
		case S3_TWISTER:
			ramSizeMB = RamSavageNB[ (cr36 & 0xE0) >> 5 ];
			break;

		default:
			// How did we get here?
			ramSizeMB = 0;
			break;
	}

	uint32 usableMB = ramSizeMB;


	// If a Savage MX chip has > 8 MB, clamp it at 8 MB since memory for the
	// hardware cursor above 8 MB is unusable.

	if (si.chipType == S3_SAVAGE_MX && ramSizeMB > 8)
		usableMB = 8;

	TRACE("Savage_Init() memory size: %d MB, usable memory: %d MB\n", ramSizeMB, usableMB);

	if (usableMB <= 0)
		return B_ERROR;

	si.videoMemSize = usableMB * 1024 * 1024;

	// Compute the Command Overflow Buffer (COB) location.

	uint32 cobSize = 0x20000;	// use 128kB for the COB
	si.cobSizeIndex = 7;

	// Note that the X.org developers stated that the command overflow buffer
	// (COB) must END at a 4MB boundary which for all practical purposes means
	// the very end of the video memory.

	si.cobOffset = (si.videoMemSize - cobSize) & ~0x1ffff;	// align cob to 128k
	si.cursorOffset = (si.cobOffset - CURSOR_BYTES) & ~0xfff;	// align to 4k boundary
	si.frameBufferOffset = 0;
	si.maxFrameBufferSize = si.cursorOffset - si.frameBufferOffset;

	TRACE("cobSizeIndex: %d	cobSize: %d  cobOffset: 0x%x\n", si.cobSizeIndex, cobSize, si.cobOffset);
	TRACE("cursorOffset: 0x%x  frameBufferOffset: 0x%x\n", si.cursorOffset, si.frameBufferOffset);

	// Reset graphics engine to avoid memory corruption.

	WriteCrtcReg(0x66, 0x02, 0x02);		// set reset flag
	snooze(10000);
	WriteCrtcReg(0x66, 0x00, 0x02);		// clear reset flag
	snooze(10000);

	// Check for DVI/flat panel.

	bool bDvi = false;
	if (si.chipType == S3_SAVAGE4) {
		WriteSeqReg(0x30, 0x00, 0x02);		// clear bit 1
		if (ReadSeqReg(0x30) & 0x02 /* 0x04 */) {
			 bDvi = true;
			 TRACE("Digital Flat Panel Detected\n");
		}
	}

	if (S3_SAVAGE_MOBILE_SERIES(si.chipType) || S3_MOBILE_TWISTER_SERIES(si.chipType)) {
		si.displayType = MT_LCD;
		Savage_GetPanelInfo();
	}
	else if (bDvi)
		si.displayType = MT_DFP;
	else
		si.displayType = MT_CRT;

	TRACE("Display Type: %d\n", si.displayType);

	// Detect current mclk.

	WriteSeqReg(0x08, 0x06);		// unlock extended sequencer regs

	uint8 m = ReadSeqReg(0x11) & 0x7f;
	uint8 n = ReadSeqReg(0x10);
	uint8 n1 = n & 0x1f;
	uint8 n2 = (n >> 5) & 0x03;
	si.mclk = ((1431818 * (m + 2)) / (n1 + 2) / (1 << n2) + 50) / 100;

	TRACE("Detected current MCLK value of %1.3f MHz\n", si.mclk / 1000.0);

	// Set up the array of color spaces supported by the Savage chips.

	si.colorSpaces[0] = B_CMAP8;
	si.colorSpaces[1] = B_RGB16;
	si.colorSpaces[2] = B_RGB32;
	si.colorSpaceCount = 3;

	si.bDisableHdwCursor = false;	// allow use of hardware cursor
	si.bDisableAccelDraw = false;	// allow use of accelerated drawing functions

	// Setup the mode list.

	return CreateModeList(IsModeUsable, Savage_GetEdidInfo);
}


void
Savage_SetFunctionPointers(void)
{
	// Setting the function pointers must be done prior to first ModeInit call
	// or any accel activity.

	switch (gInfo.sharedInfo->chipType) {
		case S3_SAVAGE_3D:
		case S3_SAVAGE_MX:
			 gInfo.WaitQueue		= WaitQueue3D;
			 gInfo.WaitIdleEmpty	= WaitIdleEmpty3D;
			 break;

		case S3_SAVAGE4:
		case S3_PROSAVAGE:
		case S3_SUPERSAVAGE:
		case S3_PROSAVAGE_DDR:
		case S3_TWISTER:
			 gInfo.WaitQueue		= WaitQueue4;
			 gInfo.WaitIdleEmpty	= WaitIdleEmpty4;
			 break;

		case S3_SAVAGE2000:
			 gInfo.WaitQueue		= WaitQueue2K;
			 gInfo.WaitIdleEmpty	= WaitIdleEmpty2K;
			 break;
	}

	gInfo.DPMSCapabilities = Savage_DPMSCapabilities;
	gInfo.GetDPMSMode = Savage_GetDPMSMode;
	gInfo.SetDPMSMode = Savage_SetDPMSMode;

	gInfo.LoadCursorImage = Savage_LoadCursorImage;
	gInfo.SetCursorPosition = Savage_SetCursorPosition;
	gInfo.ShowCursor = Savage_ShowCursor;

	gInfo.FillRectangle = Savage_FillRectangle;
	gInfo.FillSpan = Savage_FillSpan;
	gInfo.InvertRectangle = Savage_InvertRectangle;
	gInfo.ScreenToScreenBlit = Savage_ScreenToScreenBlit;

	gInfo.AdjustFrame = Savage_AdjustFrame;
	gInfo.ChipInit = Savage_Init;
	gInfo.GetColorSpaceParams = Savage_GetColorSpaceParams;
	gInfo.SetDisplayMode = Savage_SetDisplayMode;
	gInfo.SetIndexedColors = Savage_SetIndexedColors;
}

