/*
 * Copyright 1999, Be Incorporated.
 * Copyright 2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Be Incorporated
 *		Eric Petit <eric.petit@lapsus.org>
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include <string.h>
#include "GlobalData.h"


static void
WriteScanline(void * scanline, uint16 sizeInBytes)
{
	uint32* words = (uint32*)scanline;
	/* sizeInBytes must be a multiple of 4 */
	uint16 sizeInWords = sizeInBytes / 4;
	uint16 i;
	for (i = 0; i < sizeInWords; i ++)
		FifoWrite(words[i]);
}


static void 
WriteAndMask(uint8 * andMask, uint16 width, uint16 height, uint8 * scanline, 
		uint16 scanlineSize)
{
	uint16 y;
	uint16 bpr = (width + 7) / 8;
	for (y = 0; y < height; y ++) {
		// copy andMask into scanline to avoid
		// out of bounds access at last row
		memcpy(scanline, &andMask[y * bpr], bpr);		
		WriteScanline(scanline, scanlineSize);
	}
}


static void
WriteXorMask(uint8 * andMask, uint8 * xorMask, uint16 width, uint16 height, 
		uint8 * scanline, uint16 scanlineSize)
{
	uint16 x;
	uint16 y;
	uint16 bpr = (width + 7) / 8;
	for (y = 0; y < height; y ++) {
		uint8 * andMaskRow = &andMask[y * bpr];
		// copy xorMask into scanline to avoid
		// out of bounds access at last row
		memcpy(scanline,  &xorMask[y * bpr], bpr);		
		// in case of a 1 bit in andMask
		// the meaning of the corresponding bit
		// in xorMask is the opposite in the
		// emulated graphics HW (1 = white, 0 =
		// black). Be API: 1 = black, 0 = white.
		for (x = 0; x < width; x ++) {
			uint8 bit = 7 - x % 8;
			uint8 bitMask = 1 << bit;
			uint16 byte = x / 8;
			if ((andMaskRow[byte] & bitMask) == 0)
				scanline[byte] = scanline[byte] ^ bitMask; 
		}		
		WriteScanline(scanline, scanlineSize);
	}
}


status_t
SET_CURSOR_SHAPE(uint16 width, uint16 height, uint16 hot_x,
	uint16 hot_y, uint8 * andMask, uint8 * xorMask)
{

	uint16 scanlineSize;
	uint8 * scanline;

	TRACE("SET_CURSOR_SHAPE (%d, %d, %d, %d)\n", width, height, hot_x, hot_y);

	/* Sanity check */
	if (hot_x >= width || hot_y >= height)
		return B_ERROR;

	scanlineSize = 4 * ((width + 31) / 32);
	scanline = calloc(1, scanlineSize);
	if (scanline == NULL)
		return B_ERROR;
		
	FifoBeginWrite();
	FifoWrite(SVGA_CMD_DEFINE_CURSOR);
	FifoWrite(CURSOR_ID);
	FifoWrite(hot_x);
	FifoWrite(hot_y);
	FifoWrite(width);
	FifoWrite(height);
	FifoWrite(1);
	FifoWrite(1);
	WriteAndMask(andMask, width, height, scanline, scanlineSize);
	WriteXorMask(andMask, xorMask, width, height, scanline, scanlineSize);
	FifoEndWrite();
	FifoSync();
	
	free(scanline);

	return B_OK;
}


void
MOVE_CURSOR(uint16 x, uint16 y)
{
	uint16 pos[2];

	//TRACE("MOVE_CURSOR (%d, %d)\n", x, y);

	pos[0] = x;
	pos[1] = y;
	ioctl(gFd, VMWARE_MOVE_CURSOR, pos, sizeof(pos));
}


void
SHOW_CURSOR(bool isVisible)
{
	TRACE("SHOW_CURSOR (%d)\n", isVisible);

	ioctl(gFd, VMWARE_SHOW_CURSOR, &isVisible, sizeof(bool));
}

