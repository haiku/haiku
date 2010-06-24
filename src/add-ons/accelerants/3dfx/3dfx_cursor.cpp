/*
	Copyright 2010 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2010
*/


#include "accelerant.h"
#include "3dfx.h"

#include <string.h>



void
TDFX_ShowCursor(bool bShow)
{
	// Turn cursor on/off.

	uint32 config = INREG32(VIDEO_PROC_CONFIG);
	if (bShow)
		config |= CURSOR_ENABLE;
	else
		config &= ~CURSOR_ENABLE;

	TDFX_WaitForFifo(1);
	OUTREG32(VIDEO_PROC_CONFIG, config);
}


void
TDFX_SetCursorPosition(int x, int y)
{
	TDFX_WaitForFifo(1);
	OUTREG32(HW_CURSOR_LOC, ((y + 63) << 16) | (x + 63));
}


bool
TDFX_LoadCursorImage(int width, int height, uint8* andMask, uint8* xorMask)
{
	SharedInfo& si = *gInfo.sharedInfo;

	if (andMask == NULL || xorMask == NULL)
		return false;

	uint64* fbCursor64 = (uint64*)((addr_t)si.videoMemAddr + si.cursorOffset);

	// Initialize the hardware cursor as completely transparent.

	for (int i = 0; i < CURSOR_BYTES; i += 16) {
		*fbCursor64++ = ~0;		// and bits
		*fbCursor64++ = 0;		// xor bits
	}

	// Now load the AND & XOR masks for the cursor image into the cursor
	// buffer.  Note that a particular bit in these masks will have the
	// following effect upon the corresponding cursor pixel:
	//	AND  XOR	Result
	//	 0    0		 White pixel
	//	 0    1		 Black pixel
	//	 1    0		 Screen color (for transparency)
	//	 1    1		 Reverse screen color to black or white

	uint8* fbCursor = (uint8*)((addr_t)si.videoMemAddr + si.cursorOffset);

	for (int row = 0; row < height; row++) {
		for (int colByte = 0; colByte < width / 8; colByte++) {
			fbCursor[row * 16 + colByte] = *andMask++;
			fbCursor[row * 16 + colByte + 8] = *xorMask++;
		}
	}

	// Set the cursor colors which are white background and black foreground.

	TDFX_WaitForFifo(2);
	OUTREG32(HW_CURSOR_COLOR0, 0xffffff);
	OUTREG32(HW_CURSOR_COLOR1, 0);

	return true;
}
