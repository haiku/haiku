/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon accelerant
		
	Public functions to provide 2D hardware acceleration
*/


#include "radeon_accelerant.h"
#include "GlobalData.h"
#include "generic.h"
#include "cp_regs.h"
#include "3d_regs.h"
#include "2d_regs.h"
#include "mmio.h"


// currently, an CP instruction stream is written to
// a buffer on stack and then copied into the official
// CP buffer
		
#define PACKET_BUFFER_LEN 0x100

// copy screen to screen
//	et - ignored
//	list - list of rectangles
//	count - number of rectangles
void SCREEN_TO_SCREEN_BLIT(engine_token *et, blit_params *list, uint32 count) 
{
	virtual_card *vc = ai->vc;
	int offset = 0;
	uint32 buffer[PACKET_BUFFER_LEN];
	
	SHOW_FLOW0( 4, "" );
	
	for( ; count > 0; --count, ++list ) {
		if( offset == 0 ) {
			buffer[offset++] = RADEON_CP_PACKET3_CNTL_BITBLT_MULTI;
			buffer[offset++] = RADEON_GMC_BRUSH_NONE
				| (vc->datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
				| RADEON_GMC_SRC_DATATYPE_COLOR
				| RADEON_ROP3_S
				| RADEON_DP_SRC_SOURCE_MEMORY;
		}

		buffer[offset++] = (list->src_left << 16) | list->src_top;
		buffer[offset++] = (list->dest_left << 16) | list->dest_top;
		buffer[offset++] = ((list->width + 1) << 16) | (list->height + 1);
		
		if( offset + 3 > PACKET_BUFFER_LEN ) {
			buffer[0] |= (offset - 2) << 16;

			Radeon_SendCP( ai, buffer, offset );
			
			offset = 0;
		}
	}
	
	if( offset > 0 ) {
		buffer[0] |= (offset - 2) << 16;

		Radeon_SendCP( ai, buffer, offset );
	}
	
	++ai->si->engine.count;
}


// fill rectangles on screen
//	et - ignored
//	colorIndex - fill colour
//	list - list of rectangles
//	count - number of rectangles
void FILL_RECTANGLE(engine_token *et, uint32 colorIndex, 
	fill_rect_params *list, uint32 count) 
{
	virtual_card *vc = ai->vc;
	int offset = 0;
	uint32 buffer[PACKET_BUFFER_LEN];
	
	SHOW_FLOW0( 4, "" );
	
	for( ; count > 0; --count, ++list ) {
		if( offset == 0 ) {
			buffer[offset++] = RADEON_CP_PACKET3_CNTL_PAINT_MULTI;
			buffer[offset++] = RADEON_GMC_BRUSH_SOLID_COLOR
				| (vc->datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
				| RADEON_GMC_SRC_DATATYPE_COLOR
				| RADEON_ROP3_P;
			buffer[offset++] = colorIndex;
		}

		buffer[offset++] = (list->left << 16) | list->top;
		buffer[offset++] = 
			((list->right - list->left + 1) << 16) | 
			(list->bottom - list->top + 1);
		
		if( offset + 2 > PACKET_BUFFER_LEN ) {
			buffer[0] |= (offset - 2) << 16;

			Radeon_SendCP( ai, buffer, offset );
			
			offset = 0;
		}
	}
	
	if( offset > 0 ) {
		buffer[0] |= (offset - 2) << 16;

		Radeon_SendCP( ai, buffer, offset );
	}
		
	++ai->si->engine.count;
}


// invert rectangle on screen
//	et - ignored
//	list - list of rectangles
//	count - number of rectangles
void INVERT_RECTANGLE(engine_token *et, fill_rect_params *list, uint32 count) 
{
	virtual_card *vc = ai->vc;
	int offset = 0;
	uint32 buffer[PACKET_BUFFER_LEN];
	
	SHOW_FLOW0( 4, "" );
	
	for( ; count > 0; --count, ++list ) {
		if( offset == 0 ) {
			buffer[offset++] = RADEON_CP_PACKET3_CNTL_PAINT_MULTI;
			buffer[offset++] = RADEON_GMC_BRUSH_NONE
				| (vc->datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
				| RADEON_GMC_SRC_DATATYPE_COLOR
				| RADEON_ROP3_Dn;
		}

		buffer[offset++] = (list->left << 16) | list->top;
		buffer[offset++] = 
			((list->right - list->left + 1) << 16) | 
			(list->bottom - list->top + 1);

		// always leave 2 extra bytes for fix (see below)		
		if( offset + 2 > PACKET_BUFFER_LEN - 2 ) {
			buffer[0] |= (offset - 2) << 16;

			Radeon_SendCP( ai, buffer, offset );
			
			offset = 0;
		}
	}

	buffer[0] |= (offset - 2) << 16;

	// we have to reset ROP, else we get garbage during next
	// CPU access; it looks like some cache coherency/forwarding
	// problem as it goes away later on; things like flushing the
	// destination cache or waiting for 2D engine or HDP to become
	// idle and clean didn't change a thing
	// (I dont't really understand what exactly happens,
	//  but this code fixes it)
	buffer[offset++] = CP_PACKET0( RADEON_DP_GUI_MASTER_CNTL, 0 );
	buffer[offset++] = RADEON_GMC_BRUSH_NONE
		| (vc->datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
		| RADEON_GMC_SRC_DATATYPE_COLOR
		| RADEON_ROP3_S
		| RADEON_DP_SRC_SOURCE_MEMORY;

	if( offset > 0 )
		Radeon_SendCP( ai, buffer, offset );
		
	++ai->si->engine.count;
}

// fill horizontal spans on screen
//	et - ignored
//	colorIndex - fill colour
//	list - list of spans
//	count - number of spans
void FILL_SPAN(engine_token *et, uint32 colorIndex, uint16 *list, uint32 count) 
{
	virtual_card *vc = ai->vc;
	int offset = 0;
	uint32 buffer[PACKET_BUFFER_LEN];
	
	SHOW_FLOW0( 4, "" );
	
	for( ; count > 0; --count ) {
		uint16 y, x, width;
		
		if( offset == 0 ) {
			buffer[offset++] = RADEON_CP_PACKET3_CNTL_PAINT_MULTI;
			buffer[offset++] = RADEON_GMC_BRUSH_SOLID_COLOR
				| (vc->datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
				| RADEON_GMC_SRC_DATATYPE_COLOR
				| RADEON_ROP3_P;
			buffer[offset++] = colorIndex;
		}
		
		y = *list++;
		x = *list++;
		width = *list++ - x + 1;

		buffer[offset++] = (x << 16) | y;
		buffer[offset++] = (width << 16) | 1;
		
		if( offset + 2 > PACKET_BUFFER_LEN ) {
			buffer[0] |= (offset - 2) << 16;

			Radeon_SendCP( ai, buffer, offset );
			
			offset = 0;
		}
	}
	
	if( offset > 0 ) {
		buffer[0] |= (offset - 2) << 16;

		Radeon_SendCP( ai, buffer, offset );
	}
	
	++ai->si->engine.count;
}


// prepare 2D acceleration
void Radeon_Init2D( accelerator_info *ai, uint32 datatype )
{
	SHOW_FLOW0( 3, "" );

	// forget about 3D	
	OUTREG( ai->regs, RADEON_RB3D_CNTL, 0 );
	
	//Radeon_ResetEngine( ai );
	
	// no siccors	
	Radeon_WaitForFifo( ai, 1 );
	OUTREG( ai->regs, RADEON_DEFAULT_SC_BOTTOM_RIGHT, (RADEON_DEFAULT_SC_RIGHT_MAX
		    | RADEON_DEFAULT_SC_BOTTOM_MAX));

	// setup general flags - perhaps this is not needed as all
	// 2D commands contain this register
	Radeon_WaitForFifo( ai, 1 );
	OUTREG( ai->regs, RADEON_DP_GUI_MASTER_CNTL, 
		(datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
		| RADEON_GMC_CLR_CMP_CNTL_DIS
		
		| RADEON_GMC_BRUSH_SOLID_COLOR 
		| RADEON_GMC_SRC_DATATYPE_COLOR
		
		| RADEON_ROP3_P
		| RADEON_DP_SRC_SOURCE_MEMORY
		| RADEON_GMC_WR_MSK_DIS );
	    
	
	// most of this init is probably not nessacary
	// as we neither draw lines nor use brushes
	Radeon_WaitForFifo( ai, 7 );
	OUTREG( ai->regs, RADEON_DST_LINE_START,    0);
	OUTREG( ai->regs, RADEON_DST_LINE_END,      0);
	OUTREG( ai->regs, RADEON_DP_BRUSH_FRGD_CLR, 0xffffffff);
	OUTREG( ai->regs, RADEON_DP_BRUSH_BKGD_CLR, 0x00000000);
	OUTREG( ai->regs, RADEON_DP_SRC_FRGD_CLR,   0xffffffff);
	OUTREG( ai->regs, RADEON_DP_SRC_BKGD_CLR,   0x00000000);
	OUTREG( ai->regs, RADEON_DP_WRITE_MASK,     0xffffffff);
	
	Radeon_WaitForIdle( ai );
}

// switch to virtual card, i.e. setup all specific engine registers
void Radeon_ActivateVirtualCard( accelerator_info *ai )
{
	virtual_card *vc = ai->vc;
	uint32 buffer[3*2];
	uint32 pitch_offset;
	int idx = 0;
	
	SHOW_FLOW0( 4, "" );
	
	pitch_offset = (vc->fb_offset >> 10) | ((vc->pitch >> 6) << 22);
	buffer[idx++] = CP_PACKET0( RADEON_DEFAULT_OFFSET, 0 );
	buffer[idx++] = pitch_offset;
	buffer[idx++] = CP_PACKET0( RADEON_DST_PITCH_OFFSET, 0 );
	buffer[idx++] = pitch_offset;
	buffer[idx++] = CP_PACKET0( RADEON_SRC_PITCH_OFFSET, 0 );
	buffer[idx++] = pitch_offset;
	
	Radeon_SendCP( ai, buffer, idx );
	
	ai->si->active_vc = vc->id;
}
