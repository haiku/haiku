/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Syncing to graphics card engine
*/

#include "radeon_accelerant.h"
//#include "../include/radeon_regs.h"
#include "mmio.h"
#include "cp_regs.h"
#include "pll_regs.h"
#include "rbbm_regs.h"
#include "buscntrl_regs.h"

#include "log_coll.h"
#include "log_enum.h"

#include <string.h>

void Radeon_FlushPixelCache( accelerator_info *ai );

// send command to purge cache
// (may not work with pre-r200 as affected registers
//  aren't described there)
void Radeon_SendPurgeCache( accelerator_info *ai )
{
	uint32 buffer[2];
	
	buffer[0] = CP_PACKET0( RADEON_RB2D_DSTCACHE_CTLSTAT, 0 );
	buffer[1] = RADEON_RB2D_DC_FLUSH_ALL;

	Radeon_SendCP( ai, buffer, 2 );
}

// send command to wait until everything is idle
void Radeon_SendWaitUntilIdle( accelerator_info *ai )
{
	uint32 buffer[2];
	
	buffer[0] = CP_PACKET0( RADEON_WAIT_UNTIL, 0 );
	buffer[1] = RADEON_WAIT_2D_IDLECLEAN |
		   RADEON_WAIT_3D_IDLECLEAN |
		   RADEON_WAIT_HOST_IDLECLEAN;

	Radeon_SendCP( ai, buffer, 2 );
}

// make sure all drawing is finished
void Radeon_Finish( accelerator_info *ai )
{
	shared_info *si = ai->si;
	
	LOG( si->log, _Radeon_Finish );
	
	OUTREG( ai->regs, RADEON_CP_RB_WPTR, si->ring.tail );
	Radeon_WaitForIdle( ai );
	Radeon_FlushPixelCache( ai );
}

// wait until engine is idle
int Radeon_WaitForIdle( accelerator_info *ai )
{
	SHOW_FLOW0( 3, "" );
	
	Radeon_WaitForFifo( ai, 64 );
	
	while( 1 ) {
		bigtime_t start_time = system_time();
	
		do {
			if( (INREG( ai->regs, RADEON_RBBM_STATUS ) & RADEON_RBBM_ACTIVE) == 0 ) {
				Radeon_FlushPixelCache( ai );
				return 0;
			}
			
			snooze( 1 );
		} while( system_time() - start_time < 1000000 );
		
		SHOW_ERROR0( 3, "Engine didn't become idle" );
		
		LOG( ai->si->log, _Radeon_WaitForIdle );

		Radeon_ResetEngine( ai );
	}
}

// wait until "entries" FIFO entries are empty
void Radeon_WaitForFifo( accelerator_info *ai, int entries )
{
	SHOW_FLOW( 4, "entries=%ld", entries );
	
	while( 1 ) {
		bigtime_t start_time = system_time();
	
		do {
			int slots = INREG( ai->regs, RADEON_RBBM_STATUS ) & RADEON_RBBM_FIFOCNT_MASK;
			
			SHOW_FLOW( 4, "empty slots: %ld", slots );
		
			if ( slots >= entries ) 
				return;

			snooze( 1 );
		} while( system_time() - start_time < 1000000 );
		
		LOG( ai->si->log, _Radeon_WaitForFifo );
		
		Radeon_ResetEngine( ai );
	}
}

// flush pixel cache of graphics card
void Radeon_FlushPixelCache( accelerator_info *ai )
{
	bigtime_t start_time;
	
	SHOW_FLOW0( 3, "" );

	OUTREGP( ai->regs, RADEON_RB2D_DSTCACHE_CTLSTAT, RADEON_RB2D_DC_FLUSH_ALL,
		~RADEON_RB2D_DC_FLUSH_ALL );

	start_time = system_time();
	
	do {
		if( (INREG( ai->regs, RADEON_RB2D_DSTCACHE_CTLSTAT ) 
			 & RADEON_RB2D_DC_BUSY) == 0 ) 
			return;

		snooze( 1 );
	} while( system_time() - start_time < 1000000 );
	
	LOG( ai->si->log, _Radeon_FlushPixelCache );

	SHOW_ERROR0( 0, "pixel cache didn't become empty" );
}

// reset graphics card's engine
void Radeon_ResetEngine( accelerator_info *ai )
{
	vuint8 *regs = ai->regs;
	shared_info *si = ai->si;
	uint32 clock_cntl_index, mclk_cntl, rbbm_soft_reset, host_path_cntl;
	uint32 cur_read_ptr;
	
	SHOW_FLOW0( 3, "" );

	Radeon_FlushPixelCache( ai );

	clock_cntl_index = INREG( regs, RADEON_CLOCK_CNTL_INDEX );
	R300_PLLFix( ai );
	
	// OUCH!
	// XFree disables any kind of automatic power power management 
	// because of bugs of some ASIC revision (seems like the revisions
	// cannot be read out)
	// -> this is a very bad idea, especially when it comes to laptops
	// I comment it out for now, let's hope noone takes notice
    if( ai->si->has_crtc2 ) {
		Radeon_OUTPLLP( ai, RADEON_SCLK_CNTL, 
			RADEON_CP_MAX_DYN_STOP_LAT |
			RADEON_SCLK_FORCEON_MASK,
			~RADEON_DYN_STOP_LAT_MASK );
	
/*		if( ai->si->asic == rt_rv200 ) {
		    Radeon_OUTPLLP( ai, RADEON_SCLK_MORE_CNTL, 
		    	RADEON_SCLK_MORE_FORCEON, ~0 );
		}*/
    }

	mclk_cntl = Radeon_INPLL( ai, RADEON_MCLK_CNTL );

	// enable clock of units to be reset
	Radeon_OUTPLL( ai, RADEON_MCLK_CNTL, mclk_cntl |
      RADEON_FORCEON_MCLKA |
      RADEON_FORCEON_MCLKB |
      RADEON_FORCEON_YCLKA |
      RADEON_FORCEON_YCLKB |
      RADEON_FORCEON_MC |
      RADEON_FORCEON_AIC );

	// do the reset
    host_path_cntl = INREG( regs, RADEON_HOST_PATH_CNTL );
	rbbm_soft_reset = INREG( regs, RADEON_RBBM_SOFT_RESET );

	switch( ai->si->asic ) {
	case rt_r300:
		OUTREG( regs, RADEON_RBBM_SOFT_RESET, (rbbm_soft_reset |
			RADEON_SOFT_RESET_CP |
			RADEON_SOFT_RESET_HI |
			RADEON_SOFT_RESET_E2 |
			RADEON_SOFT_RESET_AIC ));
		INREG( regs, RADEON_RBBM_SOFT_RESET);
		OUTREG( regs, RADEON_RBBM_SOFT_RESET, 0);
		// this bit has no description
		OUTREGP( regs, RADEON_RB2D_DSTCACHE_MODE, (1 << 17), ~0 );

		break;
	default:
		OUTREG( regs, RADEON_RBBM_SOFT_RESET, rbbm_soft_reset |
			RADEON_SOFT_RESET_CP |
			RADEON_SOFT_RESET_HI |
			RADEON_SOFT_RESET_SE |
			RADEON_SOFT_RESET_RE |
			RADEON_SOFT_RESET_PP |
			RADEON_SOFT_RESET_E2 |
			RADEON_SOFT_RESET_RB |
			RADEON_SOFT_RESET_AIC );
		INREG( regs, RADEON_RBBM_SOFT_RESET );
		OUTREG( regs, RADEON_RBBM_SOFT_RESET, rbbm_soft_reset &
			~( RADEON_SOFT_RESET_CP |
			RADEON_SOFT_RESET_HI |
			RADEON_SOFT_RESET_SE |
			RADEON_SOFT_RESET_RE |
			RADEON_SOFT_RESET_PP |
			RADEON_SOFT_RESET_E2 |
			RADEON_SOFT_RESET_RB |
			RADEON_SOFT_RESET_AIC ) );
		INREG( regs, RADEON_RBBM_SOFT_RESET );
	}

    OUTREG( regs, RADEON_HOST_PATH_CNTL, host_path_cntl | RADEON_HDP_SOFT_RESET );
    INREG( regs, RADEON_HOST_PATH_CNTL );
    OUTREG( regs, RADEON_HOST_PATH_CNTL, host_path_cntl );

	// restore regs
	OUTREG( regs, RADEON_RBBM_SOFT_RESET, rbbm_soft_reset);

	OUTREG( regs, RADEON_CLOCK_CNTL_INDEX, clock_cntl_index );
	R300_PLLFix( ai );
	Radeon_OUTPLL( ai, RADEON_MCLK_CNTL, mclk_cntl );
	
	// reset ring buffer
	cur_read_ptr = INREG( regs, RADEON_CP_RB_RPTR );
	OUTREG( regs, RADEON_CP_RB_WPTR, cur_read_ptr );
	
	if( si->ring.head ) {
		*si->ring.head = cur_read_ptr;
		si->ring.tail = cur_read_ptr;
	}

	++si->engine.count;

	return;
}
