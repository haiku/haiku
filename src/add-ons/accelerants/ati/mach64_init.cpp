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



static bool
Mach64_InitDSPParams()
{
	// Initialize global variables used to set DSP registers on a VT-B or later.

	SharedInfo& si = *gInfo.sharedInfo;
	M64_Params& params = si.m64Params;

	// Retrieve XCLK settings.

	uint8 ioValue = Mach64_GetPLLReg(PLL_XCLK_CNTL);
	params.xClkPostDivider = ioValue & 0x7;

	switch (params.xClkPostDivider) {
	case 0:
	case 1:
	case 2:
	case 3:
		params.xClkRefDivider = 1;
		break;

	case 4:
		params.xClkRefDivider = 3;
		params.xClkPostDivider = 0;
		break;

	default:
		TRACE("Unsupported XCLK source:  %d.\n", params.xClkPostDivider);
		return false;
	}

	if (ioValue & PLL_MFB_TIMES_4_2B)
		params.xClkPostDivider--;

	// Compute maximum RAS delay and related params.

	uint32 memCntl = INREG(MEM_CNTL);
	int trp = GetBits(memCntl, CTL_MEM_TRP);
	params.xClkPageFaultDelay = GetBits(memCntl, CTL_MEM_TRCD) +
							   GetBits(memCntl, CTL_MEM_TCRD) + trp + 2;
	params.xClkMaxRASDelay = GetBits(memCntl, CTL_MEM_TRAS) + trp + 2;

	params.displayFIFODepth = 32;

	if (si.chipType < MACH64_264VT4) {
		params.xClkPageFaultDelay += 2;
		params.xClkMaxRASDelay += 3;
		params.displayFIFODepth = 24;
	}

	// Determine type of memory used with the chip.

	int memType = INREG(CONFIG_STAT0) & 0x7;
	TRACE("Memory type: %d\n", memType);

	switch (memType) {
	case MEM_DRAM:
		if (si.videoMemSize <= 1024 * 1024) {
			params.displayLoopLatency = 10;
		} else {
			params.displayLoopLatency = 8;
			params.xClkPageFaultDelay += 2;
		}
		break;

	case MEM_EDO:
	case MEM_PSEUDO_EDO:
		if (si.videoMemSize <= 1024 * 1024) {
			params.displayLoopLatency = 9;
		} else {
			params.displayLoopLatency = 8;
			params.xClkPageFaultDelay++;
		}
		break;

	case MEM_SDRAM:
		if (si.videoMemSize <= 1024 * 1024) {
			params.displayLoopLatency = 11;
		} else {
			params.displayLoopLatency = 10;
			params.xClkPageFaultDelay++;
		}
		break;

	case MEM_SGRAM:
		params.displayLoopLatency = 8;
		params.xClkPageFaultDelay += 3;
		break;

	default:                 /* Set maximums */
		params.displayLoopLatency = 11;
		params.xClkPageFaultDelay += 3;
		break;
	}

	if (params.xClkMaxRASDelay <= params.xClkPageFaultDelay)
		params.xClkMaxRASDelay = params.xClkPageFaultDelay + 1;

	uint32 dspConfig = INREG(DSP_CONFIG);
	if (dspConfig)
		params.displayLoopLatency = GetBits(dspConfig, DSP_LOOP_LATENCY);

	return true;
}


static bool
Mach64_GetColorSpaceParams(int colorSpace, uint8& bitsPerPixel, uint32& maxPixelClock)
{
	// Get parameters for a color space which is supported by the Mach64 chips.
	// Argument maxPixelClock is in KHz.
	// Return true if the color space is supported;  else return false.

	SharedInfo& si = *gInfo.sharedInfo;

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

	if (si.chipType >= MACH64_264VTB) {
		if ((si.chipType >= MACH64_264VT4) && (si.chipType != MACH64_264LTPRO))
			maxPixelClock = 230000;
		else if (si.chipType >= MACH64_264VT3)
			maxPixelClock = 200000;
		else
			maxPixelClock = 170000;
	} else {
		if (bitsPerPixel == 8)
			maxPixelClock = 135000;
		else
			maxPixelClock = 80000;
	}

	return true;
}



status_t
Mach64_Init(void)
{
	TRACE("Mach64_Init()\n");

	SharedInfo& si = *gInfo.sharedInfo;

	static const int videoRamSizes[] =
		{ 512, 1024, 2*1024, 4*1024, 6*1024, 8*1024, 12*1024, 16*1024 };

	uint32 memCntl = INREG(MEM_CNTL);
	if (si.chipType < MACH64_264VTB) {
		si.videoMemSize = videoRamSizes[memCntl & 0x7] * 1024;
	} else {
		uint32 ioValue = (memCntl & 0xf);
		if (ioValue < 8)
			si.videoMemSize = (ioValue + 1) * 512 * 1024;
		else if (ioValue < 12)
			si.videoMemSize = (ioValue - 3) * 1024 * 1024;
		else
			si.videoMemSize = (ioValue - 7) * 2048 * 1024;
	}

	si.cursorOffset = (si.videoMemSize - CURSOR_BYTES) & ~0xfff;	// align to 4k boundary
	si.frameBufferOffset = 0;
	si.maxFrameBufferSize = si.cursorOffset - si.frameBufferOffset;

	TRACE("Video Memory size: %d MB  frameBufferOffset: 0x%x  cursorOffset: 0x%x\n",
		si.videoMemSize / 1024 / 1024, si.frameBufferOffset, si.cursorOffset);

	// 264VT-B's and later have DSP registers.

	if ((si.chipType >= MACH64_264VTB) && !Mach64_InitDSPParams())
		return B_ERROR;

	// Determine if the LCD display of a laptop computer is active.

	si.displayType = MT_VGA;

	if (si.chipType == MACH64_MOBILITY && si.panelX > 0 && si.panelY > 0) {
		if (Mach64_GetLCDReg(LCD_GEN_CNTL) & LCD_ON)
			si.displayType = MT_LAPTOP;
	}

	// Set up the array of color spaces supported by the Mach64 chips.

	si.colorSpaces[0] = B_CMAP8;
	si.colorSpaces[1] = B_RGB15;
	si.colorSpaces[2] = B_RGB16;
	si.colorSpaces[3] = B_RGB32;
	si.colorSpaceCount = 4;

	// Setup the mode list.

	return CreateModeList(IsModeUsable);
}


static void
Mach64_WaitForFifo(uint32 entries)
{
	// The FIFO has 16 slots.  This routines waits until at least `entries'
	// of these slots are empty.

	while ((INREG(FIFO_STAT) & 0xffff) > (0x8000ul >> entries)) ;
}


static void
Mach64_WaitForIdle()
{
	// Wait for the graphics engine to be completely idle.  That is, the FIFO
	// has drained, the Pixel Cache is flushed, and the engine is idle.  This
	// is a standard "sync" function that will make the hardware "quiescent".

	Mach64_WaitForFifo(16);

	while (INREG(GUI_STAT) & ENGINE_BUSY) ;
}


void
Mach64_SetFunctionPointers(void)
{
	// Setting the function pointers must be done prior to first ModeInit call
	// or any accel activity.

	gInfo.WaitForFifo = Mach64_WaitForFifo;
	gInfo.WaitForIdle = Mach64_WaitForIdle;

	gInfo.DPMSCapabilities = Mach64_DPMSCapabilities;
	gInfo.GetDPMSMode = Mach64_GetDPMSMode;
	gInfo.SetDPMSMode = Mach64_SetDPMSMode;

	gInfo.LoadCursorImage = Mach64_LoadCursorImage;
	gInfo.SetCursorPosition = Mach64_SetCursorPosition;
	gInfo.ShowCursor = Mach64_ShowCursor;

	gInfo.FillRectangle = Mach64_FillRectangle;
	gInfo.FillSpan = Mach64_FillSpan;
	gInfo.InvertRectangle = Mach64_InvertRectangle;
	gInfo.ScreenToScreenBlit = Mach64_ScreenToScreenBlit;

	gInfo.AdjustFrame = Mach64_AdjustFrame;
	gInfo.ChipInit = Mach64_Init;
	gInfo.GetColorSpaceParams = Mach64_GetColorSpaceParams;
	gInfo.SetDisplayMode = Mach64_SetDisplayMode;
	gInfo.SetIndexedColors = Mach64_SetIndexedColors;
}

