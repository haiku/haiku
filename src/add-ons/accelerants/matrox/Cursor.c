/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson,
	Rudolf Cornelissen 4/2003-11/2004
*/

#define MODULE_BIT 0x20000000

/*DUALHEAD notes - 
	No hardware cursor possible on the secondary head :( 
		Reasons:
		CRTC1 has a cursor, can be displayed on DAC or MAVEN
		CRTC2 has no cursor
		Can not switch CRTC in one vblank (has to resync)
		CRTC2 does not support split screen
		app_server does not support some modes with and some without cursor
	virtual not supported, because of MAVEN blanking issues
*/

#include "acc_std.h"

status_t SET_CURSOR_SHAPE(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, uint8 *andMask, uint8 *xorMask) 
{
	LOG(4,("SET_CURSOR_SHAPE: width %d, height %d\n", width, height));
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
		gx00_crtc_cursor_define(andMask,xorMask);

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

	/*set up minimum amount to scroll*/
	if (si->dm.flags & DUALHEAD_BITS)
	{
		switch(si->dm.space)
		{
		case B_RGB16_LITTLE:
			h_adjust = 0x1f;
			break;
		case B_RGB32_LITTLE:
			h_adjust = 0x0f;
			break;
		default:
			h_adjust = 0x1f;
			break;
		}
	}
	else
	{
		switch(si->dm.space)
		{
		case B_CMAP8:
			h_adjust = 0x07;
			break;
		case B_RGB15_LITTLE:case B_RGB16_LITTLE:
			h_adjust = 0x03;
			break;
		case B_RGB32_LITTLE:
			h_adjust = 0x01;
			break;
		default:
			h_adjust = 0x07;
			break;
		}
	}

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
		gx00_bes_move_overlay();
	}

	/* put cursor in correct physical position */
	x -= hds + si->cursor.hot_x;
	y -= vds + si->cursor.hot_y;

	/* account for switched CRTC's */
	if (si->switched_crtcs)	x -= si->dm.timing.h_display;

	/* position the cursor on the display */
	gx00_crtc_cursor_position(x,y);
}

void SHOW_CURSOR(bool is_visible) 
{
	/* record for our info */
	si->cursor.is_visible = is_visible;

	if (is_visible)
		gx00_crtc_cursor_show();
	else
		gx00_crtc_cursor_hide();
}
