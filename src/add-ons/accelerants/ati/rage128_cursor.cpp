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



void
Rage128_ShowCursor(bool bShow)
{
	// Turn cursor on/off.

   	OUTREGM(R128_CRTC_GEN_CNTL, bShow ? R128_CRTC_CUR_EN : 0, R128_CRTC_CUR_EN);
}


void
Rage128_SetCursorPosition(int x, int y)
{
	SharedInfo& si = *gInfo.sharedInfo;

	// xOffset & yOffset are used for displaying partial cursors on screen edges.

	uint8 xOffset = 0;
	uint8 yOffset = 0;

	if (x < 0) {
		xOffset = (( -x) & 0xFE);
		x = 0;
	}

	if (y < 0) {
		yOffset = (( -y) & 0xFE);
		y = 0;
	}

	OUTREG(R128_CUR_HORZ_VERT_OFF,  R128_CUR_LOCK | (xOffset << 16) | yOffset);
	OUTREG(R128_CUR_HORZ_VERT_POSN, R128_CUR_LOCK | (x << 16) | y);
	OUTREG(R128_CUR_OFFSET, si.cursorOffset + yOffset * 16);
}


bool
Rage128_LoadCursorImage(int width, int height, uint8* andMask, uint8* xorMask)
{
	SharedInfo& si = *gInfo.sharedInfo;

	if (andMask == NULL || xorMask == NULL)
		return false;

	// Initialize the hardware cursor as completely transparent.

	uint32* fbCursor32 = (uint32*)((addr_t)si.videoMemAddr + si.cursorOffset);

	for (int i = 0; i < CURSOR_BYTES; i += 16) {
		*fbCursor32++ = ~0;		// and bits
		*fbCursor32++ = ~0;
		*fbCursor32++ = 0;		// xor bits
		*fbCursor32++ = 0;
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

	OUTREG(R128_CUR_CLR0, ~0);
	OUTREG(R128_CUR_CLR1, 0);

	return true;
}
