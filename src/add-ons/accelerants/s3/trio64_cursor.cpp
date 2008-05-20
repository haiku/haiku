/*
	Haiku S3 Trio64 driver adapted from the X.org Virge driver.

	Copyright (C) 1994-1999 The XFree86 Project, Inc.  All Rights Reserved.

	Copyright 2007-2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2007-2008
*/

#include "accel.h"
#include "trio64.h"



void 
Trio64_ShowCursor(bool bShow)
{
	// Turn cursor on/off.

	WriteCrtcReg(0x45, bShow ? 0x01 : 0x00, 0x01);
}


void 
Trio64_SetCursorPosition(int x, int y)
{
	uint8 xOffset = 0;
	uint8 yOffset = 0;

	// xOffset & yOffset are used for displaying partial cursors on screen edges.

	if (x < 0) {
		xOffset = (( -x) & 0xfe);
		x = 0;
	}

	if (y < 0) {
		yOffset = (( -y) & 0xfe);
		y = 0;
	}

	// Note: when setting the cursor position, register 48 must be set last
	// since setting register 48 activates the new cursor position.

	WriteCrtcReg( 0x4e, xOffset );
	WriteCrtcReg( 0x4f, yOffset );

	WriteCrtcReg( 0x47, (x & 0xff) );
	WriteCrtcReg( 0x46, (x & 0x0700) >> 8 );

	WriteCrtcReg( 0x49, (y & 0xff) );
	WriteCrtcReg( 0x48, (y & 0x0700) >> 8 );
}


bool 
Trio64_LoadCursorImage(int width, int height, uint8* andMask, uint8* xorMask)
{
	SharedInfo& si = *gInfo.sharedInfo;

	if (andMask == NULL || xorMask == NULL)
		return false;

	// Initialize hardware cursor to be completely transparent.

	uint16* fbCursor16 = (uint16*)((addr_t)si.videoMemAddr + si.cursorOffset);

	for (int i = 0; i < CURSOR_BYTES; i += 4) {
		*fbCursor16++ = ~0;		// and bits
		*fbCursor16++ = 0;		// xor bits
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
			fbCursor[row * 16 + colByte + 2] = *xorMask++;
		}
	}

	// Set cursor location in video memory.

	WriteCrtcReg(0x4c, (0x0f00 & si.cursorOffset / 1024) >> 8);
	WriteCrtcReg(0x4d, (0xff & si.cursorOffset / 1024));

	// Set the cursor colors which are black foreground and white background.

	ReadCrtcReg(0x45);		// reset cursor color stack pointer
	WriteCrtcReg(0x4a, 0);	// set foreground color stack low, mid, high bytes
	WriteCrtcReg(0x4a, 0);
	WriteCrtcReg(0x4a, 0);

	ReadCrtcReg(0x45);		// reset cursor color stack pointer
	WriteCrtcReg(0x4b, ~0);	// set background color stack low, mid, high bytes
	WriteCrtcReg(0x4b, ~0);
	WriteCrtcReg(0x4b, ~0);

	return true;
}
