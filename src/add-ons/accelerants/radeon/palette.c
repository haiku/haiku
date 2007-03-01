/*
	Copyright (c) 2002-2004, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Palette handling. Though it's very often referred to as 
	being part of the DAC, this is not really true as palette
	lookup is part of the CRTC unit (else it wouldn't work for
	digital output like DVI)
*/


#include "GlobalData.h"
#include "dac_regs.h"
#include "mmio.h"
#include "CP.h"
#include "generic.h"

// set standard colour palette (needed for non-palette modes)
void Radeon_InitPalette( 
	accelerator_info *ai, int crtc_idx )
{
	int i;
	
	if ( ai->si->acc_dma ) {
		START_IB();
		
		WRITE_IB_REG( RADEON_DAC_CNTL2,
			(crtc_idx == 0 ? 0 : RADEON_DAC2_PALETTE_ACC_CTL) |
			(ai->si->dac_cntl2 & ~RADEON_DAC2_PALETTE_ACC_CTL) );
		
		WRITE_IB_REG( RADEON_PALETTE_INDEX, 0 );
		
		for( i = 0; i < 256; ++i )
			WRITE_IB_REG( RADEON_PALETTE_DATA, (i << 16) | (i << 8) | i );
			
		SUBMIT_IB();
	} else {
		Radeon_WaitForFifo ( ai , 1 );
		OUTREG( ai->regs, RADEON_DAC_CNTL2,
			(crtc_idx == 0 ? 0 : RADEON_DAC2_PALETTE_ACC_CTL) |
			(ai->si->dac_cntl2 & ~RADEON_DAC2_PALETTE_ACC_CTL) );

		OUTREG( ai->regs, RADEON_PALETTE_INDEX, 0 );
		for( i = 0; i < 256; ++i ) {
			Radeon_WaitForFifo ( ai , 1 );  // TODO FIXME good god this is gonna be slow...
			OUTREG( ai->regs, RADEON_PALETTE_DATA, (i << 16) | (i << 8) | i );
		}
	}
}

static void setPalette( 
	accelerator_info *ai, int crtc_idx, 
	uint count, uint8 first, uint8 *color_data );

// public function: set colour palette
void SET_INDEXED_COLORS(
	uint count, uint8 first, uint8 *color_data, uint32 flags )
{
	virtual_card *vc = ai->vc;
	
	(void)flags;

	SHOW_FLOW( 3, "first=%d, count=%d", first, count );
	
	if( vc->mode.space != B_CMAP8 ) {
		SHOW_ERROR0( 2, "Tried to set palette in non-palette mode" );
		return;
	} 

	if( vc->used_crtc[0] )
		setPalette( ai, 0, count, first, color_data );
	if( vc->used_crtc[1] )
		setPalette( ai, 1, count, first, color_data );
}


// set palette of one DAC
static void setPalette( 
	accelerator_info *ai, int crtc_idx, 
	uint count, uint8 first, uint8 *color_data )
{
	uint i;
	
	if ( ai->si->acc_dma ) {
		START_IB();
		
		WRITE_IB_REG( RADEON_DAC_CNTL2,
			(crtc_idx == 0 ? 0 : RADEON_DAC2_PALETTE_ACC_CTL) |
			(ai->si->dac_cntl2 & ~RADEON_DAC2_PALETTE_ACC_CTL) );
		
		WRITE_IB_REG( RADEON_PALETTE_INDEX, first );
		
		for( i = 0; i < count; ++i, color_data += 3 )
			WRITE_IB_REG( RADEON_PALETTE_DATA, 
				((uint32)color_data[0] << 16) | 
				((uint32)color_data[1] << 8) | 
				 color_data[2] );
				 
		SUBMIT_IB();
	} else {
		Radeon_WaitForFifo ( ai , 1 );
		OUTREG( ai->regs, RADEON_DAC_CNTL2,
			(crtc_idx == 0 ? 0 : RADEON_DAC2_PALETTE_ACC_CTL) |
			(ai->si->dac_cntl2 & ~RADEON_DAC2_PALETTE_ACC_CTL) );

		OUTREG( ai->regs, RADEON_PALETTE_INDEX, first );
		for( i = 0; i < count; ++i, color_data += 3 ) {
			Radeon_WaitForFifo ( ai , 1 );  // TODO FIXME good god this is gonna be slow...
			OUTREG( ai->regs, RADEON_PALETTE_DATA, 
				((uint32)color_data[0] << 16) | 
				((uint32)color_data[1] << 8) | 
				 color_data[2] );
		}
	}
}
