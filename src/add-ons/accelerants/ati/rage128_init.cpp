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



// Memory Specifications from RAGE 128 Software Development Manual
// (Technical Reference Manual P/N SDK-G04000 Rev 0.01), page 3-21.
static R128_RAMSpec sRAMSpecs[] = {
	{ 4, 4, 3, 3, 1, 3, 1, 16, 12, "128-bit SDR SGRAM 1:1" },
	{ 4, 8, 3, 3, 1, 3, 1, 17, 13, "64-bit SDR SGRAM 1:1" },
	{ 4, 4, 1, 2, 1, 2, 1, 16, 12, "64-bit SDR SGRAM 2:1" },
	{ 4, 4, 3, 3, 2, 3, 1, 16, 12, "64-bit DDR SGRAM" },
};



static bool
Rage128_GetColorSpaceParams(int colorSpace, uint8& bitsPerPixel, uint32& maxPixelClock)
{
	// Get parameters for a color space which is supported by the Rage128 chips.
	// Argument maxPixelClock is in KHz.
	// Return true if the color space is supported;  else return false.

	switch (colorSpace) {
		case B_RGB32:
			bitsPerPixel = 32;
			break;
		case B_RGB16:
			bitsPerPixel = 16;
			break;
		case B_RGB15:
			bitsPerPixel = 15;
			break;
		case B_CMAP8:
			bitsPerPixel = 8;
			break;
		default:
			TRACE("Unsupported color space: 0x%X\n", colorSpace);
			return false;
	}

	maxPixelClock = gInfo.sharedInfo->r128PLLParams.max_pll_freq * 10;
	return true;
}


static void
WaitForFifo(uint32 entries)
{
	// The FIFO has 64 slots.  This routines waits until at least `entries'
	// of these slots are empty.

	while (true) {
		for (int i = 0; i < R128_TIMEOUT; i++) {
			uint32 slots = INREG(R128_GUI_STAT) & R128_GUI_FIFOCNT_MASK;
			if (slots >= entries)
				return;
		}

		TRACE("FIFO timed out: %d entries, stat=0x%08x, probe=0x%08x\n",
			INREG(R128_GUI_STAT) & R128_GUI_FIFOCNT_MASK,
			INREG(R128_GUI_STAT),
			INREG(R128_GUI_PROBE));
		TRACE("FIFO timed out, resetting engine...\n");

		Rage128_EngineReset();
	}
}


static void
WaitForIdle()
{
	// Wait for the graphics engine to be completely idle.  That is, the FIFO
	// has drained, the Pixel Cache is flushed, and the engine is idle.  This
	// is a standard "sync" function that will make the hardware "quiescent".

	WaitForFifo(64);

	while (true) {
		for (uint32 i = 0; i < R128_TIMEOUT; i++) {
			if ( ! (INREG(R128_GUI_STAT) & R128_GUI_ACTIVE)) {
				Rage128_EngineFlush();
				return ;
			}
		}

		TRACE("Idle timed out: %d entries, stat=0x%08x, probe=0x%08x\n",
				   INREG(R128_GUI_STAT) & R128_GUI_FIFOCNT_MASK,
				   INREG(R128_GUI_STAT),
				   INREG(R128_GUI_PROBE));
		TRACE("Idle timed out, resetting engine...\n");

		Rage128_EngineReset();
	}
}


status_t
Rage128_Init(void)
{
	TRACE("Rage128_Init()\n");

	SharedInfo& si = *gInfo.sharedInfo;

	si.videoMemSize = INREG(R128_CONFIG_MEMSIZE);

	si.cursorOffset = (si.videoMemSize - CURSOR_BYTES) & ~0xfff;	// align to 4k boundary
	si.frameBufferOffset = 0;
	si.maxFrameBufferSize = si.cursorOffset - si.frameBufferOffset;

	TRACE("Video Memory size: %d MB  frameBufferOffset: 0x%x  cursorOffset: 0x%x\n",
		si.videoMemSize / 1024 / 1024, si.frameBufferOffset, si.cursorOffset);

	// Get the specifications of the memory used by the chip.

	uint32 offset;

	switch (INREG(R128_MEM_CNTL) & 0x3) {
	case 0:					// SDR SGRAM 1:1
		switch (si.deviceID) {
		case 0x4C45:		// RAGE128 LE:
		case 0x4C46:		// RAGE128 LF:
		case 0x4D46:		// RAGE128 MF:
		case 0x4D4C:		// RAGE128 ML:
		case 0x5245:		// RAGE128 RE:
		case 0x5246:		// RAGE128 RF:
		case 0x5247:		// RAGE128 RG:
		case 0x5446:		// RAGE128 TF:
		case 0x544C:		// RAGE128 TL:
		case 0x5452:		// RAGE128 TR:
			offset = 0;		// 128-bit SDR SGRAM 1:1
			break;
		default:
			offset = 1;		// 64-bit SDR SGRAM 1:1
			break;
		}
		break;
	case 1:
		offset = 2;		// 64-bit SDR SGRAM 2:1
		break;
	case 2:
		offset = 3;		// 64-bit DDR SGRAM
		break;
	default:
		offset = 1;		// 64-bit SDR SGRAM 1:1
		break;
	}
	si.r128MemSpec = sRAMSpecs[offset];
	TRACE("RAM type: %s\n", si.r128MemSpec.name);

	// Determine the type of display.

	si.displayType = MT_VGA;

	if (INREG(R128_FP_PANEL_CNTL) & R128_FP_DIGON)	// don't know if this is correct
		si.displayType = MT_DVI;

	if (si.chipType == RAGE128_MOBILITY && si.panelX > 0 && si.panelY > 0) {
		if (INREG(R128_LVDS_GEN_CNTL) & R128_LVDS_ON)
			si.displayType = MT_LAPTOP;	// laptop LCD display is on
	}

	// Set up the array of color spaces supported by the Rage128 chips.

	si.colorSpaces[0] = B_CMAP8;
	si.colorSpaces[1] = B_RGB15;
	si.colorSpaces[2] = B_RGB16;
	si.colorSpaces[3] = B_RGB32;
	si.colorSpaceCount = 4;

	// Setup the mode list.

	return CreateModeList(IsModeUsable);
}


void
Rage128_SetFunctionPointers(void)
{
	// Setting the function pointers must be done prior to first ModeInit call
	// or any accel activity.

	gInfo.WaitForFifo = WaitForFifo;
	gInfo.WaitForIdle = WaitForIdle;

	gInfo.DPMSCapabilities = Rage128_DPMSCapabilities;
	gInfo.GetDPMSMode = Rage128_GetDPMSMode;
	gInfo.SetDPMSMode = Rage128_SetDPMSMode;

	gInfo.LoadCursorImage = Rage128_LoadCursorImage;
	gInfo.SetCursorPosition = Rage128_SetCursorPosition;
	gInfo.ShowCursor = Rage128_ShowCursor;

	gInfo.FillRectangle = Rage128_FillRectangle;
	gInfo.FillSpan = Rage128_FillSpan;
	gInfo.InvertRectangle = Rage128_InvertRectangle;
	gInfo.ScreenToScreenBlit = Rage128_ScreenToScreenBlit;

	gInfo.AdjustFrame = Rage128_AdjustFrame;
	gInfo.ChipInit = Rage128_Init;
	gInfo.GetColorSpaceParams = Rage128_GetColorSpaceParams;
	gInfo.SetDisplayMode = Rage128_SetDisplayMode;
	gInfo.SetIndexedColors = Rage128_SetIndexedColors;
}

