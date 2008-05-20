/*
	Haiku S3 Savage driver adapted from the X.org Savage driver.

	Copyright (C) 1994-2000 The XFree86 Project, Inc.  All Rights Reserved.
	Copyright (c) 2003-2006, X.Org Foundation

	Copyright 2007-2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2006-2008
*/


#include "accel.h"
#include "savage.h"



// Certain HW cursor operations seem to require a delay to prevent lockups.

static void 
WaitHSync(int n)
{
	while (n--) {
		while (ReadReg8(SYSTEM_CONTROL_REG) & 0x01) {};
		while ( ! ReadReg8(SYSTEM_CONTROL_REG) & 0x01) {};
	}
}


void
Savage_ShowCursor(bool bShow)
{
	// Turn cursor on/off.

	if ( ! bShow && S3_SAVAGE4_SERIES(gInfo.sharedInfo->chipType))
		WaitHSync(5);

	WriteCrtcReg(0x45, bShow ? 0x01 : 0x00, 0x01);
}


void 
Savage_SetCursorPosition(int x, int y)
{
	SharedInfo& si = *gInfo.sharedInfo;

	if (S3_SAVAGE4_SERIES(si.chipType))
		WaitHSync(5);

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
Savage_LoadCursorImage(int width, int height, uint8* andMask, uint8* xorMask)
{
	SharedInfo& si = *gInfo.sharedInfo;

	if (andMask == NULL || xorMask == NULL)
		return false;

	// Initialize the hardware cursor as completely transparent.

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

	if (S3_SAVAGE4_SERIES(si.chipType)) {

		// Bug in Savage4 Rev B requires us to do an MMIO read after
		// loading the cursor.

		volatile unsigned int k = ALT_STATUS_WORD0;
		(void)k++;	// Not to be optimised out
	}

	// Set cursor location in video memory.

	WriteCrtcReg(0x4d, (0xff & si.cursorOffset / 1024));
	WriteCrtcReg(0x4c, (0xff00 & si.cursorOffset / 1024) >> 8);

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
