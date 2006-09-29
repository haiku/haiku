/*
    Copyright 1999, Be Incorporated.   All Rights Reserved.
    This file may be used under the terms of the Be Sample Code License.
*/
/*
 * Copyright 1999  Erdi Chen
 */

#include "GlobalData.h"
#include "generic.h"
#include "s3drv.h"

void set_cursor_colors(void)
{
    /* it's only called by the INIT_ACCELERANT() */
    s3drv_set_cursor_color (0, ~0);
}

status_t
SET_CURSOR_SHAPE (
    uint16 width, uint16 height, uint16 hot_x, uint16 hot_y,
    uint8 *andMask, uint8 *xorMask)
{
/* NOTE: Currently, for BeOS, cursor width and height must be equal to 16. */

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
        /* Update cursor variables appropriately. */
        si->cursor.width = width;
        si->cursor.height = height;
        si->cursor.hot_x = hot_x;
        si->cursor.hot_y = hot_y;
        s3drv_load_cursor ((byte_t*)framebuffer, 0,
                           width, height, andMask, xorMask);
    }

    return B_OK;
}

/*
Move the cursor to the specified position on the desktop.  If we're
using some kind of virtual desktop, adjust the display start position
accordingly and position the cursor in the proper "virtual" location.
*/
void MOVE_CURSOR (uint16 x_p, uint16 y_p)
{
    int x = x_p, y = y_p;
    bool move_screen = false;
    /* the current horizontal starting pixel */
    uint16 hds = si->dm.h_display_start;
    /* the current vertical starting line */
    uint16 vds = si->dm.v_display_start;

    /* Need to set this value for 32 bit */
    uint16 h_adjust = 3;

    /* clamp cursor to virtual display */
    if (x >= si->dm.virtual_width) x = si->dm.virtual_width - 1;
    if (y >= si->dm.virtual_height) y = si->dm.virtual_height - 1;

    /* adjust h/v_display_start to move cursor onto screen */
    if (x >= (si->dm.timing.h_display + hds))
    {
    	hds = ((x - si->dm.timing.h_display) + 1 + h_adjust) & ~h_adjust;
    	move_screen = true;
    }
    else if (x < hds)
    {
    	hds = x & ~h_adjust;
    	move_screen = true;
    }
    if (y >= (si->dm.timing.v_display + vds))
    {
    	vds = y - si->dm.timing.v_display + 1;
    	move_screen = true;
    }
    else if (y < vds)
    {
    	vds = y;
    	move_screen = true;
    }

    /* reposition the desktop on the display if required */
    if (move_screen) MOVE_DISPLAY(hds,vds);

    /* put cursor in correct physical position */
    x -= hds;
    y -= vds;
    x -= si->cursor.hot_x;
    y -= si->cursor.hot_y;

    /* position the cursor on the display */
    s3drv_move_cursor (x, y);
}

void SHOW_CURSOR (bool is_visible)
{
    if (is_visible)
        s3drv_show_cursor ();
    else
        s3drv_hide_cursor ();

    /* record for our info */
    si->cursor.is_visible = is_visible;
}
