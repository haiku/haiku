/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson,
	Rudolf Cornelissen 4/2003-5/2004
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
		head1_cursor_define(andMask,xorMask);
		if ((si->dm.flags & DUALHEAD_BITS) != DUALHEAD_OFF)
			head2_cursor_define(andMask,xorMask);

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

	/* clamp cursor to display */
	if (x >= si->dm.virtual_width) x = si->dm.virtual_width - 1;
	if (y >= si->dm.virtual_height) y = si->dm.virtual_height - 1;

	/* store, for our info */
	si->cursor.x = x;
	si->cursor.y = y;

	/* setting up minimum amount to scroll not needed:
	 * Nvidia cards can always do pixelprecise panning on both heads */
	h_adjust = 0x00;

	/* adjust h/v_display_start to move cursor onto screen */
	switch (si->dm.flags & DUALHEAD_BITS)
	{
	case DUALHEAD_ON:
	case DUALHEAD_SWITCH:
		if (x >= ((si->dm.timing.h_display * 2) + hds))
		{
			hds = ((x - (si->dm.timing.h_display * 2)) + 1 + h_adjust) & ~h_adjust;
			/* make sure we stay within the display! */
			if ((hds + (si->dm.timing.h_display * 2)) > si->dm.virtual_width)
				hds -= (h_adjust + 1);
		}
		else if (x < hds)
			hds = x & ~h_adjust;
		break;
	default:
		if (x >= (si->dm.timing.h_display + hds))
		{
			hds = ((x - si->dm.timing.h_display) + 1 + h_adjust) & ~h_adjust;
			/* make sure we stay within the display! */
			if ((hds + si->dm.timing.h_display) > si->dm.virtual_width)
				hds -= (h_adjust + 1);
		}
		else if (x < hds)
			hds = x & ~h_adjust;
		break;
	}

	if (y >= (si->dm.timing.v_display + vds))
		vds = y - si->dm.timing.v_display + 1;
	else if (y < vds)
		vds = y;

	/* reposition the desktop _and_ the overlay on the display if required */
	if ((hds!=si->dm.h_display_start) || (vds!=si->dm.v_display_start)) 
	{
		MOVE_DISPLAY(hds,vds);
		eng_bes_move_overlay();
	}

	/* put cursor in correct physical position, so stay onscreen (rel. to CRTC) */
	if (x > (hds + si->cursor.hot_x)) x -= (hds + si->cursor.hot_x);
	else x = 0;
	if (y > (vds + si->cursor.hot_y)) y -= (vds + si->cursor.hot_y);
	else y = 0;

	/* position the cursor on the display */
	switch (si->dm.flags & DUALHEAD_BITS)
	{
	case DUALHEAD_CLONE:
		head1_cursor_position(x,y);
		head2_cursor_position(x,y);
		break;
	case DUALHEAD_ON:
	case DUALHEAD_SWITCH:
		if (x < si->dm.timing.h_display)
		{
			if (si->cursor.dh_right)
			{
				LOG(4,("MOVE_CURSOR: now on left side\n"));
				head2_cursor_hide();
				head1_cursor_show();
				si->cursor.dh_right = false;
			}
			head1_cursor_position(x, y);
		}
		else
		{
			if (!si->cursor.dh_right)
			{
				LOG(4,("MOVE_CURSOR: now on right side\n"));
				head1_cursor_hide();
				head2_cursor_show();
				si->cursor.dh_right = true;
			}
			head2_cursor_position((x - si->dm.timing.h_display), y);
		}
		break;
	default: /* singlehead mode */
		head1_cursor_position(x,y);
		break;
	}
}

void SHOW_CURSOR(bool is_visible) 
{
	/* record for our info */
	si->cursor.is_visible = is_visible;

	switch (si->dm.flags & DUALHEAD_BITS)
	{
	case DUALHEAD_CLONE:
		if (is_visible)
		{
			head1_cursor_show();
			head2_cursor_show();
		}
		else
		{
			head1_cursor_hide();
			head2_cursor_hide();
		}
		break;
	case DUALHEAD_ON:
	case DUALHEAD_SWITCH:
		if (is_visible)
		{
			if (!si->cursor.dh_right)
			{
				head1_cursor_show();
			}
			else
			{
				head2_cursor_show();
			}
		}
		else
		{
			head1_cursor_hide();
			head2_cursor_hide();
		}
		break;
	default: /* singlehead mode */
		if (is_visible)
		{
			head1_cursor_show();
		}
		else
		{
			head1_cursor_hide();
		}
		break;
	}
}
