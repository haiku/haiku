/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon accelerant
		
	Public functions to provide 2D hardware acceleration
*/


#include "radeon_accelerant.h"
#include "GlobalData.h"
#include "generic.h"
#include "3d_regs.h"
#include "2d_regs.h"
#include "mmio.h"
#include "CP.h"


// copy screen to screen
//	et - ignored
//	list - list of rectangles
//	count - number of rectangles
void SCREEN_TO_SCREEN_BLIT(engine_token *et, blit_params *list, uint32 count) 
{
	virtual_card *vc = ai->vc;
		
	SHOW_FLOW0( 4, "" );
	
	(void)et;

	while( count > 0 ) {
		uint32 sub_count;

		START_IB();

		WRITE_IB_PACKET3_HEAD( RADEON_CP_PACKET3_CNTL_BITBLT_MULTI, count, 
			INDIRECT_BUFFER_SIZE, 3, 2 );
					
		*buffer++ = RADEON_GMC_BRUSH_NONE
			| (vc->datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
			| RADEON_GMC_SRC_DATATYPE_COLOR
			| RADEON_ROP3_S
			| RADEON_DP_SRC_SOURCE_MEMORY;
		
		for( ; sub_count > 0; --sub_count, ++list ) {
			*buffer++ = (list->src_left << 16) | list->src_top;
			*buffer++ = (list->dest_left << 16) | list->dest_top;
			*buffer++ = ((list->width + 1) << 16) | (list->height + 1);
		}

		SUBMIT_IB_VC();
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
	
	SHOW_FLOW0( 4, "" );
	
	(void)et;

	while( count > 0 ) {
		uint32 sub_count;
		
		START_IB();

		WRITE_IB_PACKET3_HEAD( RADEON_CP_PACKET3_CNTL_PAINT_MULTI, count, 
			INDIRECT_BUFFER_SIZE, 2, 3 );

		*buffer++ = RADEON_GMC_BRUSH_SOLID_COLOR
				| (vc->datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
				| RADEON_GMC_SRC_DATATYPE_COLOR
				| RADEON_ROP3_P;
		*buffer++ = colorIndex;

		for( ; sub_count > 0; --sub_count, ++list ) {
			*buffer++ = (list->left << 16) | list->top;
			*buffer++ = 
				((list->right - list->left + 1) << 16) | 
				(list->bottom - list->top + 1);
		}
		
		SUBMIT_IB_VC();
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
	
	SHOW_FLOW0( 4, "" );
	
	(void)et;

	while( count > 0 ) {
		uint32 sub_count;
		
		START_IB();
		
		// take core to leave space for ROP reset!
		WRITE_IB_PACKET3_HEAD( RADEON_CP_PACKET3_CNTL_PAINT_MULTI, count, 
			INDIRECT_BUFFER_SIZE - 2, 2, 2 );
		
		*buffer++ = RADEON_GMC_BRUSH_NONE
			| (vc->datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
			| RADEON_GMC_SRC_DATATYPE_COLOR
			| RADEON_ROP3_Dn;

		for( ; sub_count > 0; --sub_count, ++list ) {
			*buffer++ = (list->left << 16) | list->top;
			*buffer++ = 
				((list->right - list->left + 1) << 16) | 
				(list->bottom - list->top + 1);
		}
		
		// we have to reset ROP, else we get garbage during next
		// CPU access; it looks like some cache coherency/forwarding
		// problem as it goes away later on; things like flushing the
		// destination cache or waiting for 2D engine or HDP to become
		// idle and clean didn't change a thing
		// (I dont't really understand what exactly happens,
		//  but this code fixes it)
		*buffer++ = CP_PACKET0( RADEON_DP_GUI_MASTER_CNTL, 1 );
		*buffer++ = RADEON_GMC_BRUSH_NONE
			| (vc->datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
			| RADEON_GMC_SRC_DATATYPE_COLOR
			| RADEON_ROP3_S
			| RADEON_DP_SRC_SOURCE_MEMORY;

		SUBMIT_IB_VC();
	}

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
	
	SHOW_FLOW0( 4, "" );
	
	(void)et;

	while( count > 0 ) {
		uint32 sub_count;
		
		START_IB();

		WRITE_IB_PACKET3_HEAD( RADEON_CP_PACKET3_CNTL_PAINT_MULTI, count, 
			INDIRECT_BUFFER_SIZE , 2, 3 );
				
		*buffer++ = RADEON_GMC_BRUSH_SOLID_COLOR
			| (vc->datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
			| RADEON_GMC_SRC_DATATYPE_COLOR
			| RADEON_ROP3_P;
		*buffer++ = colorIndex;
		
		for( ; sub_count > 0; --sub_count ) {
			uint16 y, x, width;
	
			y = *list++;
			x = *list++;
			width = *list++ - x + 1;
	
			*buffer++ = (x << 16) | y;
			*buffer++ = (width << 16) | 1;
		}
				
		SUBMIT_IB_VC();
	}

	++ai->si->engine.count;
}


// prepare 2D acceleration
void Radeon_Init2D( accelerator_info *ai )
{
	SHOW_FLOW0( 3, "" );

	START_IB();
	
	// forget about 3D	
	WRITE_IB_REG( RADEON_RB3D_CNTL, 0 );
		
	SUBMIT_IB();
}


// fill state buffer that sets 2D registers up for accelerated operations
void Radeon_FillStateBuffer( accelerator_info *ai, uint32 datatype )
{
	virtual_card *vc = ai->vc;
	uint32 pitch_offset;
	uint32 *buffer, *buffer_start;
	
	SHOW_FLOW0( 4, "" );

	// make sure buffer is not used
	Radeon_InvalidateStateBuffer( ai, vc->state_buffer_idx );
	
	buffer = buffer_start = Radeon_GetIndirectBufferPtr( ai, vc->state_buffer_idx );

	// set offset of frame buffer and pitch
	pitch_offset = 
		((ai->si->memory[mt_local].virtual_addr_start + vc->fb_offset) >> 10) | 
		((vc->pitch >> 6) << 22);
	WRITE_IB_REG( RADEON_DEFAULT_OFFSET, pitch_offset );
	WRITE_IB_REG( RADEON_DST_PITCH_OFFSET, pitch_offset );
	WRITE_IB_REG( RADEON_SRC_PITCH_OFFSET, pitch_offset );
	
	// no siccors	
	WRITE_IB_REG( RADEON_DEFAULT_SC_BOTTOM_RIGHT, 
		(RADEON_DEFAULT_SC_RIGHT_MAX | RADEON_DEFAULT_SC_BOTTOM_MAX));

	// setup general flags - perhaps this is not needed as all
	// 2D commands contain this register
	WRITE_IB_REG( RADEON_DP_GUI_MASTER_CNTL, 
		(datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
		| RADEON_GMC_CLR_CMP_CNTL_DIS
		
		| RADEON_GMC_BRUSH_SOLID_COLOR 
		| RADEON_GMC_SRC_DATATYPE_COLOR
		
		| RADEON_ROP3_P
		| RADEON_DP_SRC_SOURCE_MEMORY
		| RADEON_GMC_WR_MSK_DIS );
	    
	
	// most of this init is probably not nessacary
	// as we neither draw lines nor use brushes
	WRITE_IB_REG( RADEON_DST_LINE_START,    0);
	WRITE_IB_REG( RADEON_DST_LINE_END,      0);
	WRITE_IB_REG( RADEON_DP_BRUSH_FRGD_CLR, 0xffffffff);
	WRITE_IB_REG( RADEON_DP_BRUSH_BKGD_CLR, 0x00000000);
	WRITE_IB_REG( RADEON_DP_SRC_FRGD_CLR,   0xffffffff);
	WRITE_IB_REG( RADEON_DP_SRC_BKGD_CLR,   0x00000000);
	WRITE_IB_REG( RADEON_DP_WRITE_MASK,     0xffffffff);


	vc->state_buffer_size = buffer - buffer_start;
	
	ai->si->active_vc = vc->id;
}


// allocate indirect buffer to contain state of virtual card
void Radeon_AllocateVirtualCardStateBuffer( accelerator_info *ai )
{
	virtual_card *vc = ai->vc;
	
	vc->state_buffer_idx = Radeon_AllocIndirectBuffer( ai, false );
	// mark as being unused
	vc->state_buffer_size = -1;
}


// free indirect buffer containing state of virtual card
void Radeon_FreeVirtualCardStateBuffer( accelerator_info *ai )
{
	virtual_card *vc = ai->vc;

	// make sure it's not used anymore
	Radeon_InvalidateStateBuffer( ai, vc->state_buffer_idx );

	// get rid of it
	Radeon_FreeIndirectBuffer( ai, vc->state_buffer_idx, false );
}
