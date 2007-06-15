/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Gerald Zajac 2006-2007
*/

#include "GlobalData.h"
#include "AccelerantPrototypes.h"
#include "savage.h"


status_t 
SET_CURSOR_SHAPE(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y,
						  uint8* andMask, uint8* xorMask)
{
	/* NOTE: Currently, for BeOS, cursor width and height must be equal to 16. */

	if ((width != 16) || (height != 16)) {
		return B_ERROR;
	} else if ((hot_x >= width) || (hot_y >= height)) {
		return B_ERROR;
	} else {
		/* Update cursor variables appropriately. */
		si->cursor.width = width;
		si->cursor.height = height;
		si->cursor.hot_x = hot_x;
		si->cursor.hot_y = hot_y;

		if (!SavageLoadCursorImage(width, height, andMask, xorMask))
			return B_ERROR;
	}

	return B_OK;
}

/*
Move the cursor to the specified position on the desktop.  If we're
using some kind of virtual desktop, adjust the display start position
accordingly and position the cursor in the proper "virtual" location.
*/
void 
MOVE_CURSOR(uint16 xPos, uint16 yPos)
{
	int x = xPos;		// use signed int's since SavageSetCursorPosition()
	int y = yPos;		// needs signed int to determine if cursor off screen

	uint16 hds = si->dm.h_display_start;	/* current horizontal starting pixel */
	uint16 vds = si->dm.v_display_start;	/* current vertical starting line */

	/* clamp cursor to virtual display */
	if (x >= si->dm.virtual_width)
		x = si->dm.virtual_width - 1;
	if (y >= si->dm.virtual_height)
		y = si->dm.virtual_height - 1;

	/* adjust h/v display start to move cursor onto screen */
	if (x >= (si->dm.timing.h_display + hds))
		hds = x - si->dm.timing.h_display + 1;
	else if (x < hds)
		hds = x;

	if (y >= (si->dm.timing.v_display + vds))
		vds = y - si->dm.timing.v_display + 1;
	else if (y < vds)
		vds = y;

	/* reposition the desktop on the display if required */
	if (hds != si->dm.h_display_start || vds != si->dm.v_display_start)
		MOVE_DISPLAY(hds, vds);

	/* put cursor in correct physical position */
	x -= (hds + si->cursor.hot_x);
	y -= (vds + si->cursor.hot_y);

	/* position the cursor on the display */
	SavageSetCursorPosition(x, y);
}


void 
SHOW_CURSOR(bool bShow)
{
	if (bShow)
		SavageShowCursor();
	else
		SavageHideCursor();
}

