/*
 * Copyright 1999, Be Incorporated.
 * Copyright 2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Be Incorporated
 *		Eric Petit <eric.petit@lapsus.org>
 */

#include "GlobalData.h"

status_t
SET_CURSOR_SHAPE(uint16 width, uint16 height, uint16 hot_x,
	uint16 hot_y, uint8 * andMask, uint8 * xorMask)
{
	int i, shift;
	uint32 * alphaCursor;

	TRACE("SET_CURSOR_SHAPE (%d, %d, %d, %d)\n", width, height, hot_x, hot_y);

	/* Sanity check */
	if (hot_x >= width || hot_y >= height)
		return B_ERROR;

	/* Build ARGB image */
	alphaCursor = calloc(1, height * width * sizeof(uint32));
	shift = 7;
	for (i = 0; i < height * width; i++) {
		if (!((*andMask >> shift) & 1)) {
			/* Opaque */
			alphaCursor[i] |= 0xFF000000;
			if (!((*xorMask >> shift) & 1))
				/* White */
				alphaCursor[i] |= 0x00FFFFFF;
		}
		if (--shift < 0) {
			shift = 7;
			andMask++;
			xorMask++;
		}
	}

	/* Give it to VMware */
	FifoBeginWrite();
	FifoWrite(SVGA_CMD_DEFINE_ALPHA_CURSOR);
	FifoWrite(CURSOR_ID);
	FifoWrite(hot_x);
	FifoWrite(hot_y);
	FifoWrite(width);
	FifoWrite(height);
	for (i = 0; i < height * width; i++)
		FifoWrite(alphaCursor[i]);
	FifoEndWrite();

	free(alphaCursor);

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

