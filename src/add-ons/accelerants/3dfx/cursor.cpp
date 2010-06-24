/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Gerald Zajac 2007-2010
*/

#include "accelerant.h"


status_t
SetCursorShape(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y,
				uint8* andMask, uint8* xorMask)
{
	// NOTE: Currently, for BeOS, cursor width and height must be equal to 16.

	if ((width != 16) || (height != 16)) {
		return B_ERROR;
	} else if ((hot_x >= width) || (hot_y >= height)) {
		return B_ERROR;
	} else {
		// Update cursor variables appropriately.

		SharedInfo& si = *gInfo.sharedInfo;
		si.cursorHotX = hot_x;
		si.cursorHotY = hot_y;

		if ( ! TDFX_LoadCursorImage(width, height, andMask, xorMask))
			return B_ERROR;
	}

	return B_OK;
}


void
MoveCursor(uint16 xPos, uint16 yPos)
{
	// Move the cursor to the specified position on the desktop.  If we're
	// using some kind of virtual desktop, adjust the display start position
	// accordingly and position the cursor in the proper "virtual" location.

	int x = xPos;		// use signed int's since chip specific functions
	int y = yPos;		// need signed int to determine if cursor off screen

	SharedInfo& si = *gInfo.sharedInfo;
	DisplayModeEx& dm = si.displayMode;

	uint16 hds = dm.h_display_start;	// current horizontal starting pixel
	uint16 vds = dm.v_display_start;	// current vertical starting line

	// Clamp cursor to virtual display.
	if (x >= dm.virtual_width)
		x = dm.virtual_width - 1;
	if (y >= dm.virtual_height)
		y = dm.virtual_height - 1;

	// Adjust h/v display start to move cursor onto screen.
	if (x >= (dm.timing.h_display + hds))
		hds = x - dm.timing.h_display + 1;
	else if (x < hds)
		hds = x;

	if (y >= (dm.timing.v_display + vds))
		vds = y - dm.timing.v_display + 1;
	else if (y < vds)
		vds = y;

	// Reposition the desktop on the display if required.
	if (hds != dm.h_display_start || vds != dm.v_display_start)
		MoveDisplay(hds, vds);

	// Put cursor in correct physical position.
	x -= (hds + si.cursorHotX);
	y -= (vds + si.cursorHotY);

	// Position the cursor on the display.
	TDFX_SetCursorPosition(x, y);
}

