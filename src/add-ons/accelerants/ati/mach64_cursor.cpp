/*
	Copyright 2009 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2009
*/


#include "accelerant.h"
#include "mach64.h"

#include <string.h>



void
Mach64_ShowCursor(bool bShow)
{
	// Turn cursor on/off.

   	OUTREGM(GEN_TEST_CNTL, bShow ? HWCURSOR_ENABLE : 0, HWCURSOR_ENABLE);
}


void
Mach64_SetCursorPosition(int x, int y)
{
	SharedInfo& si = *gInfo.sharedInfo;

	// xOffset & yOffset are used for displaying partial cursors on screen edges.

	uint8 xOffset = 0;
	uint8 yOffset = 0;

	if (x < 0) {
		xOffset = -x;
		x = 0;
	}

	if (y < 0) {
		yOffset = -y;
		y = 0;
	}

	OUTREG(CUR_OFFSET, (si.cursorOffset >> 3) + (yOffset << 1));
	OUTREG(CUR_HORZ_VERT_OFF, (yOffset << 16) | xOffset);
	OUTREG(CUR_HORZ_VERT_POSN, (y << 16) | x);
}


bool
Mach64_LoadCursorImage(int width, int height, uint8* andMask, uint8* xorMask)
{
	SharedInfo& si = *gInfo.sharedInfo;

	if (andMask == NULL || xorMask == NULL)
		return false;

	uint16* fbCursor = (uint16*)((addr_t)si.videoMemAddr + si.cursorOffset);

	// Initialize the hardware cursor as completely transparent.

	memset(fbCursor, 0xaa, CURSOR_BYTES);

	// Now load the AND & XOR masks for the cursor image into the cursor
	// buffer.  Note that a particular bit in these masks will have the
	// following effect upon the corresponding cursor pixel:
	//	AND  XOR	Result
	//	 0    0		 White pixel
	//	 0    1		 Black pixel
	//	 1    0		 Screen color (for transparency)
	//	 1    1		 Reverse screen color to black or white

	for (int row = 0; row < height; row++) {
		for (int colByte = 0; colByte < width / 8; colByte++) {
			// Convert the 8 bit AND and XOR masks into a 16 bit mask containing
			// pairs of the bits from the AND and XOR maks.

			uint8 andBits = *andMask++;
			uint8 xorBits = *xorMask++;
			uint16 cursorBits = 0;

			for (int j = 0; j < 8; j++) {
				cursorBits <<= 2;
				cursorBits |= ((andBits & 0x01) << 1);
				cursorBits |= (xorBits & 0x01);
				andBits >>= 1;
				xorBits >>= 1;
			}

			fbCursor[row * 8 + colByte] = cursorBits;
		}
	}

	// Set the cursor colors which are white background and black foreground.

	OUTREG(CUR_CLR0, ~0);
	OUTREG(CUR_CLR1, 0);

	return true;
}
