/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Rudolf Cornelissen 4/2003-11/2004
*/

#define MODULE_BIT 0x20000000

#include "acc_std.h"

status_t SET_CURSOR_SHAPE(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, uint8 *andMask, uint8 *xorMask) 
{
	LOG(4,("SET_CURSOR_SHAPE: width %d, height %d, hot_x %d, hot_y %d\n",
		width, height, hot_x, hot_y));

	if ((width != 16) || (height != 16))
	{
		return B_ERROR;
	}
	else if ((hot_x >= width) || (hot_y >= height))
	{
		return B_ERROR;
	}
	else
	{
		nm_crtc_cursor_define(andMask,xorMask);

		/* Update cursor variables appropriately. */
		si->cursor.width = width;
		si->cursor.height = height;
		si->cursor.hot_x = hot_x;
		si->cursor.hot_y = hot_y;
	}

	return B_OK;
}

/* Move the cursor to the specified position on the desktop, taking account of virtual/dual issues */
void MOVE_CURSOR(uint16 x, uint16 y) 
{
	uint16 hds = si->dm.h_display_start;	/* the current horizontal starting pixel */
	uint16 vds = si->dm.v_display_start;	/* the current vertical starting line */
	uint16 h_adjust;   
	uint16 h_display = si->dm.timing.h_display; /* local copy needed for flatpanel */
	uint16 v_display = si->dm.timing.v_display; /* local copy needed for flatpanel */

	/* clamp cursor to display */
	if (x >= si->dm.virtual_width) x = si->dm.virtual_width - 1;
	if (y >= si->dm.virtual_height) y = si->dm.virtual_height - 1;

	/* store, for our info */
	si->cursor.x = x;
	si->cursor.y = y;

	/* setting up minimum amount to scroll not needed:
	 * Neomagic cards can always do pixelprecise panning */
	h_adjust = 0x00;

	/* if internal panel is active correct visible screensize! */
	if (nm_general_output_read() & 0x02)
	{
		if (h_display > si->ps.panel_width) h_display = si->ps.panel_width;
		if (v_display > si->ps.panel_height) v_display = si->ps.panel_height;
	}

	/* adjust h/v_display_start to move cursor onto screen */
	if (x >= (h_display + hds))
	{
		hds = ((x - h_display) + 1 + h_adjust) & ~h_adjust;
		/* make sure we stay within the display! */
		if ((hds + h_display) > si->dm.virtual_width)
			hds -= (h_adjust + 1);
	}
	else if (x < hds)
		hds = x & ~h_adjust;

	if (y >= (v_display + vds))
		vds = y - v_display + 1;
	else if (y < vds)
		vds = y;

	/* reposition the desktop _and_ the overlay on the display if required */
	if ((hds!=si->dm.h_display_start) || (vds!=si->dm.v_display_start)) 
	{
		MOVE_DISPLAY(hds,vds);
		nm_bes_move_overlay();
	}

	/* put cursor in correct physical position */
	if (x > (hds + si->cursor.hot_x)) x -= hds + si->cursor.hot_x;
	else x = 0;
	if (y > (vds + si->cursor.hot_y)) y -= vds + si->cursor.hot_y;
	else y = 0;

	/* position the cursor on the display */
	nm_crtc_cursor_position(x,y);
}

void SHOW_CURSOR(bool is_visible) 
{
	/* record for our info */
	si->cursor.is_visible = is_visible;

	if (is_visible)
		nm_crtc_cursor_show();
	else
		nm_crtc_cursor_hide();
}
