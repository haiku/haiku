/*
	Copyright (c) 2002-2004, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Hardware cursor support.
*/


#include "radeon_accelerant.h"
#include "GlobalData.h"
#include "generic.h"
#include "mmio.h"
#include "crtc_regs.h"

static void moveOneCursor( accelerator_info *ai, int crtc_idx, int x, int y );

// set standard foreground/background colours
void Radeon_SetCursorColors( accelerator_info *ai, int crtc_idx ) 
{
	SHOW_FLOW0( 3, "" );
	
	if( crtc_idx == 0 ) {
		OUTREG( ai->regs, RADEON_CUR_CLR0, 0xffffff );
		OUTREG( ai->regs, RADEON_CUR_CLR1, 0 );
	} else {
		OUTREG( ai->regs, RADEON_CUR2_CLR0, 0xffffff );
		OUTREG( ai->regs, RADEON_CUR2_CLR1, 0 );
	}
}

// public function to set shape of cursor
status_t SET_CURSOR_SHAPE( uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, 
						   uint8 *andMask, uint8 *xorMask) 
{
	virtual_card *vc = ai->vc;
	uint8 *fb_cursor = vc->cursor.data;
	int row, col_byte;

	/* NOTE: Currently, for BeOS, cursor width and height must be equal to 16. */
/*	if( width != 16 || height != 16 )
		return B_ERROR;*/

	if( hot_x >= width || hot_y >= height )
		return B_ERROR;
		
	// TBD: should we sync here? I'd say so, but if I fail, we deadlock
			
	vc->cursor.hot_x = hot_x;
	vc->cursor.hot_y = hot_y;
		
	for( row = 0; row < 64; ++row ) {
		for( col_byte = 0; col_byte < 64 / 8; ++col_byte ) {
			if( row < height && col_byte < (width + 7) / 8 ) {
				fb_cursor[row * 64/8 * 2 + col_byte] = *andMask++;
				fb_cursor[row * 64/8 * 2 + col_byte + 64/8] = *xorMask++;
			} else {
				fb_cursor[row * 64/8 * 2 + col_byte] = 0xff;
				fb_cursor[row * 64/8 * 2 + col_byte + 64/8] = 0;
			}
		}
	}
	
	return B_OK;
}


// public function to move cursor 
void MOVE_CURSOR(uint16 x, uint16 y) 
{
	virtual_card *vc = ai->vc;
	bool move_screen = false;
	uint16 hds, vds;
	
	// alignment mask for horizontal position
	uint16 h_adjust = 7;
	
	ACQUIRE_BEN( ai->si->engine.lock );
	
	hds = vc->mode.h_display_start;
	vds = vc->mode.v_display_start;

	// clamp cursor (negative positions are impossible due to uint16)
	if (x >= vc->mode.virtual_width) 
		x = vc->mode.virtual_width - 1;
	if (y >= vc->mode.virtual_height) 
		y = vc->mode.virtual_height - 1;

	// if scrolling enabled, i.e. we have a larger virtual screen,
	// pan display accordingly
	if( vc->scroll ) {
		if( x >= (vc->mode.timing.h_display + hds) ) {
			hds = ((x - vc->mode.timing.h_display) + 1 + h_adjust) & ~h_adjust;
			move_screen = true;
		} else if( x < hds ) {
			hds = x & ~h_adjust;
			move_screen = true;
		}
		if( y >= (vc->mode.timing.v_display + vds) ) {
			vds = y - vc->mode.timing.v_display + 1;
			move_screen = true;
		} else if( y < vds ) {
			vds = y;
			move_screen = true;
		}

		if( move_screen )
			Radeon_MoveDisplay( ai, hds, vds );
	}
	
	// adjust according to virtual screen position
	x -= hds;
	y -= vds;

	// go
	if( vc->used_crtc[0] )
		moveOneCursor( ai, 0, x, y );
	if( vc->used_crtc[1] )
		moveOneCursor( ai, 1, x, y );
			
	RELEASE_BEN( ai->si->engine.lock );
}


// public function to show cursor
void SHOW_CURSOR( bool is_visible )
{
	virtual_card *vc = ai->vc;
	
	SHOW_FLOW0( 4, "" );
	
	ACQUIRE_BEN( ai->si->engine.lock );
	
	// this is the public statement
	vc->cursor.is_visible = is_visible;
	
	// the following functions take also care to not
	// show the cursor if it's on the other port
	if( vc->used_crtc[0] )
		Radeon_ShowCursor( ai, 0 );
	if( vc->used_crtc[1] )
		Radeon_ShowCursor( ai, 1 );
	
	RELEASE_BEN( ai->si->engine.lock );
}


// move cursor on one port
//	main_port - common data is stored here
void moveOneCursor( accelerator_info *ai, int crtc_idx, int x, int y )
{
	virtual_card *vc = ai->vc;
	crtc_info *crtc = &ai->si->crtc[crtc_idx];
	int xorigin, yorigin;
	bool prev_state;

	// adjust according to relative screen position	
	x -= crtc->rel_x;
	y -= crtc->rel_y;
	
	// and to hot spot
	x -= vc->cursor.hot_x;
	y -= vc->cursor.hot_y;

	// check whether the cursor is (partially) visible on this screen
   	prev_state = crtc->cursor_on_screen;
	crtc->cursor_on_screen = true;

	// in theory, cursor can be up to 64 pixels off screen, 
	// but there were display errors
	if( y > crtc->mode.timing.v_display ||
		x > crtc->mode.timing.h_display ||
		x <= -16 || y <= -16 )
	{
		crtc->cursor_on_screen = false;
	}
	
	if( prev_state != crtc->cursor_on_screen ) 
		Radeon_ShowCursor( ai, crtc_idx );

	if( !crtc->cursor_on_screen )
		return;

	// if upper-left corner of cursor is outside of
	// screen, we have to use special registers to clip it
	xorigin = 0;
	yorigin = 0;
		
    if( x < 0 )
    	xorigin = -x;
    	
    if( y < 0 ) 
    	yorigin = -y;

	if( crtc_idx == 0 )	{
		OUTREG( ai->regs, RADEON_CUR_HORZ_VERT_OFF, RADEON_CUR_LOCK
			| (xorigin << 16)
			| yorigin );
		OUTREG( ai->regs, RADEON_CUR_HORZ_VERT_POSN, RADEON_CUR_LOCK
			| ((xorigin ? 0 : x) << 16)
			| (yorigin ? 0 : y) );
		OUTREG( ai->regs, RADEON_CUR_OFFSET, 
			vc->cursor.fb_offset + xorigin + yorigin * 16 );
	} else {
		OUTREG( ai->regs, RADEON_CUR2_HORZ_VERT_OFF, RADEON_CUR2_LOCK
			| (xorigin << 16)
			| yorigin );
		OUTREG( ai->regs, RADEON_CUR2_HORZ_VERT_POSN, RADEON_CUR2_LOCK
			| ((xorigin ? 0 : x) << 16)
			| (yorigin ? 0 : y) );
		OUTREG( ai->regs, RADEON_CUR2_OFFSET, 
			vc->cursor.fb_offset + xorigin + yorigin * 16 );
	}
}


// show cursor on one port, depending on official whishes and whether
// cursor is located on this subscreen
void Radeon_ShowCursor( accelerator_info *ai, int crtc_idx )
{
	virtual_card *vc = ai->vc;
	crtc_info *crtc = &ai->si->crtc[crtc_idx];
	uint32 tmp;

	if( crtc_idx == 0 ) {
		tmp = INREG( ai->regs, RADEON_CRTC_GEN_CNTL );
		
		if( vc->cursor.is_visible && crtc->cursor_on_screen ) {
			tmp |= RADEON_CRTC_CUR_EN;
		} else {
			tmp &= ~RADEON_CRTC_CUR_EN;
		}

		OUTREG( ai->regs, RADEON_CRTC_GEN_CNTL, tmp );
		
	} else {
		tmp = INREG( ai->regs, RADEON_CRTC2_GEN_CNTL );
	
		if( vc->cursor.is_visible && crtc->cursor_on_screen )
			tmp |= RADEON_CRTC2_CUR_EN;
		else
			tmp &= ~RADEON_CRTC2_CUR_EN;

		OUTREG( ai->regs, RADEON_CRTC2_GEN_CNTL, tmp );
	}
}
