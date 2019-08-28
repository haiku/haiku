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



static void
VirgeWaitFifoGX2(uint32 slots )
{
	while (((ReadReg32(SUBSYS_STAT_REG) >> 9) & 0x60) < slots) {}
}



static void
VirgeWaitFifoMain(uint32 slots )
{
	while (((ReadReg32(SUBSYS_STAT_REG) >> 8) & 0x1f) < slots) {}
}


static void
VirgeWaitIdleEmpty()
{
	// Wait until GP is idle and queue is empty.

	if(gInfo.sharedInfo->chipType == S3_TRIO_3D)
		while ((IN_SUBSYS_STAT() & 0x3f802000 & 0x20002000) != 0x20002000);
	else
		while ((IN_SUBSYS_STAT() & 0x3f00) != 0x3000);
}


static bool
Virge_GetColorSpaceParams(int colorSpace, uint32& bitsPerPixel, uint32& maxPixelClock)
{
	// Get parameters for a color space which is supported by the Virge chips.
	// Argument maxPixelClock is in KHz.
	// Return true if the color space is supported;  else return false.

	// Note that the X.org code set the max clock to 440000 for a Virge VX chip
	// and 270000 for all other chips.  440000 seems rather high for a chip that
	// old;  thus, 270000 is used for all chips.

	switch (colorSpace) {
		case B_RGB16:
			bitsPerPixel = 16;
			maxPixelClock = 270000;
			break;
		case B_CMAP8:
			bitsPerPixel = 8;
			maxPixelClock = 270000;
			break;
		default:
			TRACE("Unsupported color space: 0x%X\n", colorSpace);
			return false;
	}

	return true;
}



status_t 
Virge_Init(void)
{
	TRACE("Virge_Init()\n");

	SharedInfo& si = *gInfo.sharedInfo;

	// Use PIO for following operations since MMIO may not be currently enabled.

	WritePIO_8(VGA_ENABLE, ReadPIO_8(VGA_ENABLE) | 0x01);	// enable VGA
	WritePIO_8(MISC_OUT_W, ReadPIO_8(MISC_OUT_R) | 0x01);	// enable color

	// Set linear base register to the PCI register value;
	// some DX chipsets don't seem to do it automatically.

	WritePIO_8(CRTC_INDEX, 0x59);
	WritePIO_8(CRTC_DATA, (uint8)((uint32)(si.videoMemPCI) >> 24));
	WritePIO_8(CRTC_INDEX, 0x5A);
	WritePIO_8(CRTC_DATA, (uint8)((uint32)(si.videoMemPCI) >> 16));

	// Enable MMIO.

	WritePIO_8(CRTC_INDEX, 0x53);
	WritePIO_8(CRTC_DATA, ReadPIO_8(CRTC_DATA) | 0x8);

	if (si.chipType == S3_TRIO_3D)
		WriteCrtcReg(0x40, 0x01, 0x01);

	// Detect amount of installed ram.

	uint8 config1 = ReadCrtcReg(0x36);	// get amount of vram installed
	uint8 config2 = ReadCrtcReg(0x37);	// get amount of off-screen ram

	// Compute the amount of video memory and offscreen memory.

	int   ramOffScreenMB = 0;	// off screen memory size in megabytes
	int   ramSizeMB = 0;		// memory size in megabytes

	if (si.chipType == S3_VIRGE_VX) {
		switch ((config2 & 0x60) >> 5) {
			case 1:
				ramOffScreenMB = 4;
				break;
			case 2:
				ramOffScreenMB = 2;
				break;
		}

		switch ((config1 & 0x60) >> 5) {
			case 0:
				ramSizeMB = 2;
				break;
			case 1:
				ramSizeMB = 4;
				break;
			case 2:
				ramSizeMB = 6;
				break;
			case 3:
				ramSizeMB = 8;
				break;
		}
		ramSizeMB -= ramOffScreenMB;

	} else if (si.chipType == S3_TRIO_3D_2X) {
		switch ((config1 & 0xE0) >> 5) {
			case 0:   	// 8MB -- only 4MB usable for display/cursor
				ramSizeMB = 4;
				ramOffScreenMB = 4;
				break;
			case 1:     // 32 bit interface -- yuck
				TRACE("Undefined video memory size on S3 Trio 3D/2X\n");
			case 2:
				ramSizeMB = 4;
				break;
			case 6:
				ramSizeMB = 2;
				break;
		}
	} else if (si.chipType == S3_TRIO_3D) {
		switch ((config1 & 0xE0) >> 5) {
			case 0:
			case 2:
				ramSizeMB = 4;
				break;
			case 4:
				ramSizeMB = 2;
				break;
		}
	} else if (si.chipType == S3_VIRGE_GX2 || S3_VIRGE_MX_SERIES(si.chipType)) {
		switch ((config1 & 0xC0) >> 6) {
			case 1:
				ramSizeMB = 4;
				break;
			case 3:
				ramSizeMB = 2;
				break;
		}
	} else {
		switch ((config1 & 0xE0) >> 5) {
			case 0:
				ramSizeMB = 4;
				break;
			case 4:
				ramSizeMB = 2;
				break;
			case 6:
				ramSizeMB = 1;
				break;
		}
	}

	TRACE("usable memory: %d MB,  off-screen memory: %d MB\n", ramSizeMB, ramOffScreenMB);

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

	TRACE("Detected current MCLK value of %1.3f MHz\n", si.mclk / 1000.0);

	if (S3_VIRGE_MX_SERIES(si.chipType)) {
		si.displayType = ((ReadSeqReg(0x31) & 0x10) ? MT_LCD : MT_CRT);
		si.panelX = (ReadSeqReg(0x61) + ((ReadSeqReg(0x66) & 0x02) << 7) + 1) * 8;
		si.panelY =  ReadSeqReg(0x69) + ((ReadSeqReg(0x6e) & 0x70) << 4) + 1;

		TRACE("%dx%d LCD panel detected %s\n", si.panelX, si.panelY,
				 si.displayType == MT_LCD ? "and active" : "but not active");
	} else {
		si.displayType = MT_CRT;
		si.panelX = 0;
		si.panelY = 0;
	}

	// Set up the array of color spaces supported by the Virge/Trio3D chips.

	si.colorSpaces[0] = B_CMAP8;
	si.colorSpaces[1] = B_RGB16;
	si.colorSpaceCount = 2;

	si.bDisableHdwCursor = false;	// allow use of hardware cursor
	si.bDisableAccelDraw = false;	// allow use of accelerated drawing functions

	// Setup the mode list.

	return CreateModeList(IsModeUsable, Virge_GetEdidInfo);
}


void
Virge_SetFunctionPointers(void)
{
	// Setting the function pointers must be done prior to first ModeInit call
	// or any accel activity.

	if (S3_VIRGE_GX2_SERIES(gInfo.sharedInfo->chipType)) {
		gInfo.WaitQueue = VirgeWaitFifoGX2;
	} else {
		gInfo.WaitQueue = VirgeWaitFifoMain;
	}

	gInfo.WaitIdleEmpty = VirgeWaitIdleEmpty;

	gInfo.DPMSCapabilities = Virge_DPMSCapabilities;
	gInfo.GetDPMSMode = Virge_GetDPMSMode;
	gInfo.SetDPMSMode = Virge_SetDPMSMode;

	gInfo.LoadCursorImage = Virge_LoadCursorImage;
	gInfo.SetCursorPosition = Virge_SetCursorPosition;
	gInfo.ShowCursor = Virge_ShowCursor;

	gInfo.FillRectangle = Virge_FillRectangle;
	gInfo.FillSpan = Virge_FillSpan;
	gInfo.InvertRectangle = Virge_InvertRectangle;
	gInfo.ScreenToScreenBlit = Virge_ScreenToScreenBlit;

	gInfo.AdjustFrame = Virge_AdjustFrame;
	gInfo.ChipInit = Virge_Init;
	gInfo.GetColorSpaceParams = Virge_GetColorSpaceParams;
	gInfo.SetDisplayMode = Virge_SetDisplayMode;
	gInfo.SetIndexedColors = Virge_SetIndexedColors;
}
