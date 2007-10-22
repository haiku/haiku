/*
	Haiku S3 Savage driver adapted from the X.org Savage driver.

	Copyright (C) 1994-2000 The XFree86 Project, Inc.  All Rights Reserved.
	Copyright (c) 2003-2006, X.Org Foundation

	Copyright 2007 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2006-2007
*/


#include "GlobalData.h"
#include "savage.h"



// Certain HW cursor operations seem to require a delay to prevent lockups.

static void 
WaitHSync(int n)
{
	while (n--) {
		while ((ReadST01()) & 0x01) {};
		while (!(ReadST01()) & 0x01) {};
	}
}


void 
SavageHideCursor(void)
{
	/* Turn cursor off. */

	if (S3_SAVAGE4_SERIES(si->chipset))
		WaitHSync(5);

	WriteCrtc(0x45, ReadCrtc(0x45) & 0xfe);
	si->cursor.bIsVisible = false;
}


void 
SavageShowCursor(void)
{
	/* Turn cursor on. */

	WriteCrtc(0x45, ReadCrtc(0x45) | 0x01);
	si->cursor.bIsVisible = true;
}


void 
SavageSetCursorPosition(int x, int y)
{
	uint8 xoff, yoff;

	if (S3_SAVAGE4_SERIES(si->chipset))
		WaitHSync(5);

	/* adjust for frame buffer base address granularity */
	if (si->bitsPerPixel == 8)
		x += ((si->frameX0) & 3);
	else if (si->bitsPerPixel == 16)
		x += ((si->frameX0) & 1);
	else if (si->bitsPerPixel == 32)
		x += ((si->frameX0 + 2) & 3) - 2;

	/*
	* Make these even when used.  There is a bug/feature on at least
	* some chipsets that causes a "shadow" of the cursor in interlaced
	* mode.  Making this even seems to have no visible effect, so just
	* do it for the generic case.
	*/

	if (x < 0) {
		xoff = (( -x) & 0xFE);
		x = 0;
	} else {
		xoff = 0;
	}

	if (y < 0) {
		yoff = (( -y) & 0xFE);
		y = 0;
	} else {
		yoff = 0;
	}

	/* This is the recomended order to move the cursor */
	WriteCrtc( 0x46, (x & 0xff00) >> 8 );
	WriteCrtc( 0x47, (x & 0xff) );
	WriteCrtc( 0x49, (y & 0xff) );
	WriteCrtc( 0x4e, xoff );
	WriteCrtc( 0x4f, yoff );
	WriteCrtc( 0x48, (y & 0xff00) >> 8 );
}


static void 
SavageSetCursorColors(int bg, int fg)
{
	/* With the streams engine on HW Cursor seems to be 24bpp ALWAYS */

//@	TRACE(("SavageSetCursorColors\n"));

	/* Do it straight, full 24 bit color. */
	/* Reset the cursor color stack pointer */
	ReadCrtc(0x45);
	/* Write low, mid, high bytes - foreground */
	WriteCrtc(0x4a, fg);
	WriteCrtc(0x4a, fg >> 8);
	WriteCrtc(0x4a, fg >> 16);
	/* Reset the cursor color stack pointer */
	ReadCrtc(0x45);
	/* Write low, mid, high bytes - background */
	WriteCrtc(0x4b, bg);
	WriteCrtc(0x4b, bg >> 8);
	WriteCrtc(0x4b, bg >> 16);
}


/* Assume width and height are byte-aligned. */

bool 
SavageLoadCursorImage(int width, int height, uint8* andMask, uint8* xorMask)
{
	int i, colByte, row;
	uint8*  fbCursor;
	uint16* fbCursor16;

//	TRACE(("SavageLoadCursorImage, width: %ld  height: %ld\n", width, height));

	if ( ! andMask || ! xorMask)
		return false;

	// Initialize the hardware cursor as completely transparent.
	
	fbCursor16 = (void *)((addr_t)si->videoMemAddr + si->cursorOffset);
	
	for (i = 0; i < 1024 / 4; i++) {
		*fbCursor16++ = ~0;		// and bits
		*fbCursor16++ = 0;		// xor bits
	}
	
	// Now load the AND & XOR masks for the cursor image into the cursor
	// buffer.
	
	fbCursor = (void *)((addr_t)si->videoMemAddr + si->cursorOffset);
	
	for (row = 0; row < height; row++) {
		for (colByte = 0; colByte < width / 8; colByte++) {
			fbCursor[row * 16 + colByte] = *andMask++;
			fbCursor[row * 16 + colByte + 2] = *xorMask++;
		}
	}
	
	SavageSetCursorColors(~0, 0);	// set cursor colors to black & white

	/* Set cursor location in video memory.  */
	WriteCrtc(0x4d, (0xff & si->cursorOffset / 1024));
	WriteCrtc(0x4c, (0xff00 & si->cursorOffset / 1024) >> 8);

	if (S3_SAVAGE4_SERIES(si->chipset)) {
		/*
		 * Bug in Savage4 Rev B requires us to do an MMIO read after
		 * loading the cursor.
		 */
		volatile unsigned int k = ALT_STATUS_WORD0;
		(void)k++;	/* Not to be optimised out */
	}

	return true;
}

