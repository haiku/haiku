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
#include "radeon_regs.h"
#include "mmio.h"
#include "CP.h"


// copy screen to screen
//	et - ignored
//	list - list of rectangles
//	count - number of rectangles
void SCREEN_TO_SCREEN_BLIT_DMA(engine_token *et, blit_params *list, uint32 count)
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

#define SRC_DSTCOLOR			0x00030000
#define DP_SRC_RECT				0x00000200

void SCREEN_TO_SCREEN_BLIT_PIO(engine_token *et, blit_params *list, uint32 count)
{
	int xdir;
	int ydir;
	virtual_card *vc = ai->vc;

	SHOW_FLOW0( 4, "" );

	Radeon_WaitForFifo ( ai , 1 );

	// Setup for Screen to screen blit
	OUTREG(ai->regs, RADEON_DP_GUI_MASTER_CNTL, (vc->datatype << RADEON_GMC_DST_DATATYPE_SHIFT
								 | RADEON_GMC_BRUSH_NONE
								 | RADEON_GMC_SRC_DATATYPE_COLOR
								 | RADEON_ROP3_S
								 | RADEON_DP_SRC_SOURCE_MEMORY
								 | RADEON_GMC_SRC_PITCH_OFFSET_CNTL));


	for( ; count > 0; --count, ++list ) {

		// make sure there is space in the FIFO for 4 register writes
		Radeon_WaitForFifo ( ai , 4 );

		xdir = ((list->src_left < list->dest_left) && (list->src_top == list->dest_top)) ? -1 : 1;
		ydir = (list->src_top < list->dest_top) ? -1 : 1;

		if (xdir < 0) list->src_left += list->width , list->dest_left += list->width ;
    	if (ydir < 0) list->src_top += list->height , list->dest_top += list->height ;

		OUTREG(ai->regs, RADEON_DP_CNTL, ((xdir >= 0 ? RADEON_DST_X_LEFT_TO_RIGHT : 0)
										| (ydir >= 0 ? RADEON_DST_Y_TOP_TO_BOTTOM : 0)));

		// Tell the engine where the source data resides.
		OUTREG( ai->regs, RADEON_SRC_Y_X, (list->src_top << 16 ) | list->src_left);
		OUTREG( ai->regs, RADEON_DST_Y_X, (list->dest_top << 16 ) | list->dest_left);

		// this is the blt initiator.
		OUTREG( ai->regs, RADEON_DST_HEIGHT_WIDTH, ((list->height + 1) << 16 ) | (list->width + 1));

	}

	++ai->si->engine.count;
	(void)et;

}


// fill rectangles on screen
//	et - ignored
//	colorIndex - fill colour
//	list - list of rectangles
//	count - number of rectangles
void FILL_RECTANGLE_DMA(engine_token *et, uint32 colorIndex,
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


// fill rectangles on screen
//	et - ignored
//	colorIndex - fill colour
//	list - list of rectangles
//	count - number of rectangles
#define BRUSH_SOLIDCOLOR		0x00000d00

void FILL_RECTANGLE_PIO(engine_token *et, uint32 colorIndex, fill_rect_params *list, uint32 count)
{
	virtual_card *vc = ai->vc;

	SHOW_FLOW( 4, "colorIndex", colorIndex);

	Radeon_WaitForFifo(ai, 3);
    OUTREG(ai->regs, RADEON_DP_GUI_MASTER_CNTL, ((vc->datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
									 | RADEON_GMC_BRUSH_SOLID_COLOR
									 | RADEON_GMC_SRC_DATATYPE_COLOR
									 | RADEON_ROP3_P));
	// Set brush colour
    OUTREG(ai->regs, RADEON_DP_BRUSH_FRGD_CLR,  colorIndex);
    OUTREG(ai->regs, RADEON_DP_CNTL, (RADEON_DST_X_LEFT_TO_RIGHT | RADEON_DST_Y_TOP_TO_BOTTOM));

	for( ; count > 0; --count, ++list )
	{

		Radeon_WaitForFifo(ai, 2);
		OUTREG(ai->regs, RADEON_DST_Y_X,          (list->top << 16) | list->left);
		OUTREG(ai->regs, RADEON_DST_WIDTH_HEIGHT, ((list->right - list->left + 1) << 16) | (list->bottom - list->top + 1));

	}
	++ai->si->engine.count;
	(void)et;
}



// invert rectangle on screen
//	et - ignored
//	list - list of rectangles
//	count - number of rectangles
void INVERT_RECTANGLE_DMA(engine_token *et, fill_rect_params *list, uint32 count)
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


void INVERT_RECTANGLE_PIO(engine_token *et, fill_rect_params *list, uint32 count)
{
	virtual_card *vc = ai->vc;

	SHOW_FLOW0( 4, "" );

	Radeon_WaitForFifo(ai, 3);
    OUTREG(ai->regs, RADEON_DP_GUI_MASTER_CNTL, ((vc->datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
									 | RADEON_GMC_BRUSH_NONE
									 | RADEON_GMC_SRC_DATATYPE_COLOR
									 | RADEON_ROP3_Dn
									 | RADEON_DP_SRC_SOURCE_MEMORY));

    OUTREG(ai->regs, RADEON_DP_CNTL, (RADEON_DST_X_LEFT_TO_RIGHT | RADEON_DST_Y_TOP_TO_BOTTOM));

	for( ; count > 0; --count, ++list )
	{

		Radeon_WaitForFifo(ai, 2);
		OUTREG(ai->regs, RADEON_DST_Y_X,          (list->top << 16) | list->left);
		OUTREG(ai->regs, RADEON_DST_WIDTH_HEIGHT, ((list->right - list->left + 1) << 16) | (list->bottom - list->top + 1));
	}

	++ai->si->engine.count;
	(void)et;

}

// fill horizontal spans on screen
//	et - ignored
//	colorIndex - fill colour
//	list - list of spans
//	count - number of spans
void FILL_SPAN_DMA(engine_token *et, uint32 colorIndex, uint16 *list, uint32 count)
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


// fill horizontal spans on screen
//	et - ignored
//	colorIndex - fill colour
//	list - list of spans
//	count - number of spans
void FILL_SPAN_PIO(engine_token *et, uint32 colorIndex, uint16 *list, uint32 count)
{

	virtual_card *vc = ai->vc;
	//int offset = 0;
	uint16 y, x, width;

	SHOW_FLOW0( 4, "" );

	Radeon_WaitForFifo( ai , 1);
    OUTREG( ai->regs, RADEON_DP_GUI_MASTER_CNTL, 0
									 | RADEON_GMC_BRUSH_SOLID_COLOR
									 | (vc->datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
									 | RADEON_GMC_SRC_DATATYPE_COLOR
									 | RADEON_ROP3_P);

	if ( ai->si->asic >= rt_rv200 ) {
		Radeon_WaitForFifo( ai , 1);
		OUTREG( ai->regs, RADEON_DST_LINE_PATCOUNT, 0x55 << RADEON_BRES_CNTL_SHIFT);
	}

	Radeon_WaitForFifo( ai , 1);
	OUTREG( ai->regs, RADEON_DP_BRUSH_FRGD_CLR,  colorIndex);

	for( ; count > 0; --count ) {

		Radeon_WaitForFifo( ai , 2);

		y = *list++;
		x = *list++;
		width = *list++ - x + 1;

	    OUTREG( ai->regs, RADEON_DST_LINE_START,           (y << 16) | x);
    	OUTREG( ai->regs, RADEON_DST_LINE_END,             ((y) << 16) | (x + width));

	}

	++ai->si->engine.count;
	(void)et;
}


void SCREEN_TO_SCREEN_BLIT(engine_token *et, blit_params *list, uint32 count)
{
	if ( ai->si->acc_dma )
		SCREEN_TO_SCREEN_BLIT_DMA(et,list,count);
	else
		SCREEN_TO_SCREEN_BLIT_PIO(et,list,count);
}

void FILL_RECTANGLE(engine_token *et, uint32 color, fill_rect_params *list, uint32 count)
{
	if ( ai->si->acc_dma )
		FILL_RECTANGLE_DMA(et, color, list, count);
	else
		FILL_RECTANGLE_PIO(et, color, list, count);
}

void INVERT_RECTANGLE(engine_token *et, fill_rect_params *list, uint32 count)
{
	if ( ai->si->acc_dma )
		INVERT_RECTANGLE_DMA(et, list, count);
	else
		INVERT_RECTANGLE_PIO(et, list, count);
}

void FILL_SPAN(engine_token *et, uint32 color, uint16 *list, uint32 count)
{
	if ( ai->si->acc_dma )
		FILL_SPAN_DMA(et, color, list, count);
	else
		FILL_SPAN_PIO(et, color, list, count);

}


// prepare 2D acceleration
void Radeon_Init2D( accelerator_info *ai )
{
	SHOW_FLOW0( 3, "" );

	// forget about 3D
	if ( ai->si->acc_dma ) {
		START_IB();
		WRITE_IB_REG( RADEON_RB3D_CNTL, 0 );
		SUBMIT_IB();
	}
	else {
		OUTREG( ai->regs, RADEON_RB3D_CNTL, 0 );
	}
}

// fill state buffer that sets 2D registers up for accelerated operations
void Radeon_FillStateBuffer( accelerator_info *ai, uint32 datatype )
{
	virtual_card *vc = ai->vc;
	uint32 pitch_offset;
	uint32 *buffer = NULL, *buffer_start = NULL;

	SHOW_FLOW0( 4, "" );

	// set offset of frame buffer and pitch
	pitch_offset =
		((ai->si->memory[mt_local].virtual_addr_start + vc->fb_offset) >> 10) |
		((vc->pitch >> 6) << 22);

	if ( ai->si->acc_dma ) {
		// make sure buffer is not used
		Radeon_InvalidateStateBuffer( ai, vc->state_buffer_idx );
		buffer = buffer_start = Radeon_GetIndirectBufferPtr( ai, vc->state_buffer_idx );

		WRITE_IB_REG( RADEON_DEFAULT_OFFSET, pitch_offset );
		WRITE_IB_REG( RADEON_DST_PITCH_OFFSET, pitch_offset );
		WRITE_IB_REG( RADEON_SRC_PITCH_OFFSET, pitch_offset );
		// no sissors
		WRITE_IB_REG( RADEON_DEFAULT_SC_BOTTOM_RIGHT,
			(RADEON_DEFAULT_SC_RIGHT_MAX | RADEON_DEFAULT_SC_BOTTOM_MAX));

		// general fluff
		WRITE_IB_REG( RADEON_DP_GUI_MASTER_CNTL,
			(datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
			| RADEON_GMC_CLR_CMP_CNTL_DIS

			| RADEON_GMC_BRUSH_SOLID_COLOR
			| RADEON_GMC_SRC_DATATYPE_COLOR

			| RADEON_ROP3_P
			| RADEON_DP_SRC_SOURCE_MEMORY
			| RADEON_GMC_WR_MSK_DIS );

		// most of this init is probably not necessary
		// as we neither draw lines nor use brushes
		WRITE_IB_REG( RADEON_DP_BRUSH_FRGD_CLR, 0xffffffff);
		WRITE_IB_REG( RADEON_DP_BRUSH_BKGD_CLR, 0x00000000);
		WRITE_IB_REG( RADEON_DP_SRC_FRGD_CLR,   0xffffffff);
		WRITE_IB_REG( RADEON_DP_SRC_BKGD_CLR,   0x00000000);
		WRITE_IB_REG( RADEON_DP_WRITE_MASK,     0xffffffff);

		// this is required
		vc->state_buffer_size = buffer - buffer_start;

	} else {
		Radeon_WaitForFifo( ai, 10 );
		OUTREG( ai->regs, RADEON_DEFAULT_OFFSET, pitch_offset );
		OUTREG( ai->regs, RADEON_DST_PITCH_OFFSET, pitch_offset );
		OUTREG( ai->regs, RADEON_SRC_PITCH_OFFSET, pitch_offset );
		// no sissors
		OUTREG( ai->regs, RADEON_DEFAULT_SC_BOTTOM_RIGHT, (RADEON_DEFAULT_SC_RIGHT_MAX
		    | RADEON_DEFAULT_SC_BOTTOM_MAX));
		// general fluff
		OUTREG( ai->regs, RADEON_DP_GUI_MASTER_CNTL,
			(datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
			| RADEON_GMC_CLR_CMP_CNTL_DIS

			| RADEON_GMC_BRUSH_SOLID_COLOR
			| RADEON_GMC_SRC_DATATYPE_COLOR

			| RADEON_ROP3_P
			| RADEON_DP_SRC_SOURCE_MEMORY
			| RADEON_GMC_WR_MSK_DIS );

		// most of this init is probably not necessary
		// as we neither draw lines nor use brushes
		OUTREG( ai->regs, RADEON_DP_BRUSH_FRGD_CLR, 0xffffffff);
		OUTREG( ai->regs, RADEON_DP_BRUSH_BKGD_CLR, 0x00000000);
		OUTREG( ai->regs, RADEON_DP_SRC_FRGD_CLR,   0xffffffff);
		OUTREG( ai->regs, RADEON_DP_SRC_BKGD_CLR,   0x00000000);
		OUTREG( ai->regs, RADEON_DP_WRITE_MASK,     0xffffffff);

	}

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
