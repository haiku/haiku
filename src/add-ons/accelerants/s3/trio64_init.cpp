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



static void
WaitQueue(uint32 slots )
{
	// Wait until the requested number of queue slots are available.

	while (ReadReg16(GP_STAT) & (0x0100 >> slots)) {}
}


static void
WaitIdle()
{
	// Wait until Graphics Processor is idle.

	while (ReadReg16(GP_STAT) & GP_BUSY);
}



static bool
Trio64_GetColorSpaceParams(int colorSpace, uint32& bitsPerPixel, uint32& maxPixelClock)
{
	// Get parameters for a color space which is supported by the Trio64 chips.
	// Argument maxPixelClock is in KHz.
	// Return true if the color space is supported;  else return false.

	switch (colorSpace) {
		case B_RGB16:
			bitsPerPixel = 16;
			maxPixelClock = 110000;
			break;
		case B_CMAP8:
			bitsPerPixel = 8;
			maxPixelClock = 180000;
			break;
		default:
			TRACE("Unsupported color space: 0x%X\n", colorSpace);
			return false;
	}

	return true;
}


static bool
Trio64_IsModeUsable(const display_mode* mode)
{
	// Test if the display mode is usable by the current video chip.
	// Return true if the mode is usable.

	SharedInfo& si = *gInfo.sharedInfo;

	if (si.chipType == S3_TRIO64 && mode->timing.h_display >= 1600)
		return false;

	return IsModeUsable(mode);
}


static status_t 
Trio64_Init(void)
{
	TRACE("Trio64_Init()\n");

	SharedInfo& si = *gInfo.sharedInfo;

	// Use PIO to enable VGA, enable color instead of monochrome, and
	// enable MMIO.

	WritePIO_8(VGA_ENABLE, ReadPIO_8(VGA_ENABLE) | 0x01);
	WritePIO_8(MISC_OUT_W, ReadPIO_8(MISC_OUT_R) | 0x01);	// enable color

	WritePIO_8(CRTC_INDEX, 0x53);
	WritePIO_8(CRTC_DATA, ReadPIO_8(CRTC_DATA) | 0x8);		// enable MMIO

	WriteCrtcReg(0x38, 0x48);		// unlock sys regs
	WriteCrtcReg(0x39, 0xa5);		// unlock sys regs

	WriteCrtcReg(0x40, 0x01, 0x01);
	WriteCrtcReg(0x35, 0x00, 0x30);
	WriteCrtcReg(0x33, 0x20, 0x72);

	if (si.chipType == S3_TRIO64_V2) {
		WriteCrtcReg(0x86, 0x80);
		WriteCrtcReg(0x90, 0x00);
	}

	// Detect number of megabytes of installed ram.

	static const uint8 ramSizes[] = { 4, 0, 3, 8, 2, 6, 1, 0 };
	int ramSizeMB = ramSizes[(ReadCrtcReg(0x36) >> 5) & 0x7];

	TRACE("memory size: %d MB\n", ramSizeMB);

	if (ramSizeMB <= 0)
		return B_ERROR;

	si.videoMemSize = ramSizeMB * 1024 * 1024;
	si.cursorOffset = si.videoMemSize - CURSOR_BYTES;	// put cursor at end of video memory
	si.frameBufferOffset = 0;
	si.maxFrameBufferSize = si.videoMemSize - CURSOR_BYTES;

	// Detect current mclk.

	WriteSeqReg(0x08, 0x06);		// unlock extended sequencer regs

	uint8 m = ReadSeqReg(0x11) & 0x7f;
	uint8 n = ReadSeqReg(0x10);
	uint8 n1 = n & 0x1f;
	uint8 n2 = (n >> 5) & 0x03;
	si.mclk = ((1431818 * (m + 2)) / (n1 + 2) / (1 << n2) + 50) / 100;

	TRACE("MCLK value: %1.3f MHz\n", si.mclk / 1000.0);

	// Set up the array of color spaces supported by the Trio64 chips.

	si.colorSpaces[0] = B_CMAP8;
	si.colorSpaces[1] = B_RGB16;
	si.colorSpaceCount = 2;

	si.bDisableHdwCursor = false;	// allow use of hardware cursor
	si.bDisableAccelDraw = false;	// allow use of accelerated drawing functions

	// Setup the mode list.

	return CreateModeList(Trio64_IsModeUsable, NULL);
}


void
Trio64_SetFunctionPointers(void)
{
	// Setting the function pointers must be done prior to first ModeInit call
	// or any accel activity.

	gInfo.WaitQueue = WaitQueue;
	gInfo.WaitIdleEmpty = WaitIdle;

	gInfo.DPMSCapabilities = Trio64_DPMSCapabilities;
	gInfo.GetDPMSMode = Trio64_GetDPMSMode;
	gInfo.SetDPMSMode = Trio64_SetDPMSMode;

	gInfo.LoadCursorImage = Trio64_LoadCursorImage;
	gInfo.SetCursorPosition = Trio64_SetCursorPosition;
	gInfo.ShowCursor = Trio64_ShowCursor;

	// Note that the 2D accel functions set below do not work with all display
	// modes;  thus, when a mode is set, the function setting the mode will
	// adjust the pointers according to the mode that is set.

	gInfo.FillRectangle = Trio64_FillRectangle;
	gInfo.FillSpan = Trio64_FillSpan;
	gInfo.InvertRectangle = Trio64_InvertRectangle;
	gInfo.ScreenToScreenBlit = Trio64_ScreenToScreenBlit;

	gInfo.AdjustFrame = Trio64_AdjustFrame;
	gInfo.ChipInit = Trio64_Init;
	gInfo.GetColorSpaceParams = Trio64_GetColorSpaceParams;
	gInfo.SetDisplayMode = Trio64_SetDisplayMode;
	gInfo.SetIndexedColors = Trio64_SetIndexedColors;
}
