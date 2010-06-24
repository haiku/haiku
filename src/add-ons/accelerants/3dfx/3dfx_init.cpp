/*
 * Copyright 2010 Haiku, Inc.  All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
 */

#include "accelerant.h"
#include "3dfx.h"



uint32
TDFX_GetVideoMemorySize(void)
{
	// Return the number of bytes of video memory.

	uint32	chipSize;		// size is in megabytes
	uint32	dramInit0 = INREG32(DRAM_INIT0);
	uint32	dramInit1 = INREG32(DRAM_INIT1);
	uint32	numChips = (dramInit0 & SGRAM_NUM_CHIPSETS) ? 8 : 4;
	int		memType = (dramInit1 & MCTL_TYPE_SDRAM) ? MEM_TYPE_SDRAM : MEM_TYPE_SGRAM;

	if (gInfo.sharedInfo->chipType == VOODOO_5) {
		chipSize = 1 << ((dramInit0 >> 27) & 0x7);
	} else {
		// Banshee or Voodoo3
		if (memType == MEM_TYPE_SDRAM)
			chipSize = 2;
		else
			chipSize = (dramInit0 & SGRAM_TYPE) ? 2 : 1;
	}

	// Disable block writes for SDRAM.

	uint32 miscInit1 = INREG32(MISC_INIT1);
	if (memType == MEM_TYPE_SDRAM) {
		miscInit1 |= DISABLE_2D_BLOCK_WRITE;
	}
	miscInit1 |= 1;
	OUTREG32(MISC_INIT1, miscInit1);

	return chipSize * numChips * 1024 * 1024;
}


status_t
TDFX_Init(void)
{
	TRACE("TDFX_Init()\n");

	SharedInfo& si = *gInfo.sharedInfo;

	si.videoMemSize = TDFX_GetVideoMemorySize();

	si.cursorOffset = 0;
	si.frameBufferOffset = si.cursorOffset + CURSOR_BYTES;
	si.maxFrameBufferSize = si.videoMemSize - si.frameBufferOffset;

	TRACE("Video Memory size: %d MB\n", si.videoMemSize / 1024 / 1024);
	TRACE("frameBufferOffset: 0x%x  cursorOffset: 0x%x\n", 
		si.frameBufferOffset, si.cursorOffset);

	switch (si.chipType) {
		case BANSHEE:
			si.maxPixelClock = 270000;
			break;
		case VOODOO_3:
			si.maxPixelClock = 300000;
			break;
		case VOODOO_5:
			si.maxPixelClock = 350000;
			break;
		default:
			TRACE("Undefined chip type: %d\n", si.chipType);
			return B_ERROR;
	}

	// Set up the array of color spaces supported by the 3dfx chips.

	si.colorSpaces[0] = B_CMAP8;
	si.colorSpaces[1] = B_RGB16;
	si.colorSpaces[2] = B_RGB32;
	si.colorSpaceCount = 3;

	// Setup the mode list.

	return CreateModeList(IsModeUsable);
}
