/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon accelerant
		
	CRTC programming
*/

#include "radeon_accelerant.h"
#include "mmio.h"
#include "crtc_regs.h"
#include "dac_regs.h"
#include "GlobalData.h"


// read old CRTC register content
void Radeon_ReadCRTCRegisters( accelerator_info *ai, physical_head *head, 
	port_regs *values )
{
	vuint8 *regs = ai->regs;

	// only CRTC_EXT_CNTL is programmed by someone else (namely the monitor
	// router); if more registers are affected, you must read them here too!
	if( !head->is_crtc2 ) {
		values->crtc_ext_cntl = INREG( regs, RADEON_CRTC_EXT_CNTL );
	}
}


// hammer CRTC registers
void Radeon_ProgramCRTCRegisters( accelerator_info *ai, physical_head *head, 
	port_regs *values )
{
	vuint8 *regs = ai->regs;
	
	SHOW_FLOW0( 2, "" );

	if( head->is_crtc2 ) {
		OUTREGP( regs, RADEON_CRTC2_GEN_CNTL, values->crtc_gen_cntl,
			RADEON_CRTC2_VSYNC_DIS |
			RADEON_CRTC2_HSYNC_DIS |
			RADEON_CRTC2_DISP_DIS );

		OUTREG( regs, RADEON_CRTC2_H_TOTAL_DISP, values->crtc_h_total_disp );
		OUTREG( regs, RADEON_CRTC2_H_SYNC_STRT_WID, values->crtc_h_sync_strt_wid );
		OUTREG( regs, RADEON_CRTC2_V_TOTAL_DISP, values->crtc_v_total_disp );
		OUTREG( regs, RADEON_CRTC2_V_SYNC_STRT_WID, values->crtc_v_sync_strt_wid );
		OUTREG( regs, RADEON_CRTC2_OFFSET_CNTL, values->crtc_offset_cntl );
		OUTREG( regs, RADEON_CRTC2_PITCH, values->crtc_pitch );
		
	} else {	
		OUTREG( regs, RADEON_CRTC_GEN_CNTL, values->crtc_gen_cntl );
		
		OUTREGP( regs, RADEON_CRTC_EXT_CNTL, values->crtc_ext_cntl,
			RADEON_CRTC_VSYNC_DIS |
			RADEON_CRTC_HSYNC_DIS |
			RADEON_CRTC_DISPLAY_DIS | 
			RADEON_CRTC_CRT_ON );
			
		OUTREGP( regs, RADEON_DAC_CNTL, values->dac_cntl,
			RADEON_DAC_RANGE_CNTL_MASK | RADEON_DAC_BLANKING );
			
		OUTREG( regs, RADEON_CRTC_H_TOTAL_DISP, values->crtc_h_total_disp );
		OUTREG( regs, RADEON_CRTC_H_SYNC_STRT_WID, values->crtc_h_sync_strt_wid );
		OUTREG( regs, RADEON_CRTC_V_TOTAL_DISP, values->crtc_v_total_disp );
		OUTREG( regs, RADEON_CRTC_V_SYNC_STRT_WID, values->crtc_v_sync_strt_wid );
		OUTREG( regs, RADEON_CRTC_OFFSET_CNTL, values->crtc_offset_cntl );
		OUTREG( regs, RADEON_CRTC_PITCH, values->crtc_pitch );
	}
}


// get required hsync delay depending on bit depth and output device
uint16 Radeon_GetHSyncFudge( physical_head *head, int datatype )
{
	static int hsync_fudge_default[] = { 0x00, 0x12, 0x09, 0x09, 0x06, 0x05 };
	static int hsync_fudge_fp[]      = { 0x02, 0x02, 0x00, 0x00, 0x05, 0x05 };
	
	// there is an sync delay which depends on colour-depth and output device
	if( (head->chosen_displays & (dd_dvi | dd_dvi_ext | dd_lvds )) != 0 )
		return hsync_fudge_fp[datatype - 1];
    else               
    	return hsync_fudge_default[datatype - 1];	
}


// calculate CRTC register content
void Radeon_CalcCRTCRegisters( accelerator_info *ai, physical_head *head, 
	display_mode *mode, port_regs *values )
{
	virtual_card *vc = ai->vc;
    int    hsync_start;
    int    hsync_wid;
    int    hsync_fudge;
    int    vsync_wid;

	hsync_fudge = Radeon_GetHSyncFudge( head, vc->datatype );

	if( head->is_crtc2 ) {
		values->crtc_gen_cntl = (RADEON_CRTC2_EN
			| RADEON_CRTC2_CRT2_ON
			| (vc->datatype << 8)
			| (0/*doublescan*/ ? RADEON_CRTC2_DBL_SCAN_EN	: 0)
			| ((mode->timing.flags & B_TIMING_INTERLACED)
				? RADEON_CRTC2_INTERLACE_EN	: 0));
	} else {
		// here, we should set interlace/double scan mode
		// but we don't support them (anyone missing them?)
	    values->crtc_gen_cntl = (RADEON_CRTC_EXT_DISP_EN
			| RADEON_CRTC_EN
			| (vc->datatype << 8));

		values->crtc_ext_cntl = 
	    	RADEON_VGA_ATI_LINEAR | 
			RADEON_XCRT_CNT_EN;

		values->dac_cntl = RADEON_DAC_MASK_ALL
			| RADEON_DAC_VGA_ADR_EN
			| RADEON_DAC_8BIT_EN;
	}
	
    values->crtc_h_total_disp = 
    	((mode->timing.h_total / 8 - 1) & RADEON_CRTC_H_TOTAL)
		| (((mode->timing.h_display / 8 - 1) << RADEON_CRTC_H_DISP_SHIFT) & RADEON_CRTC_H_DISP);

    hsync_wid = (mode->timing.h_sync_end - mode->timing.h_sync_start) / 8;

    hsync_start = mode->timing.h_sync_start - 8 + hsync_fudge;

    values->crtc_h_sync_strt_wid = 
    	(hsync_start & (RADEON_CRTC_H_SYNC_STRT_CHAR | RADEON_CRTC_H_SYNC_STRT_PIX))
		| (hsync_wid << RADEON_CRTC_H_SYNC_WID_SHIFT)
		| ((mode->flags & B_POSITIVE_HSYNC) == 0 ? RADEON_CRTC_H_SYNC_POL : 0);
  
   	values->crtc_v_total_disp = 
    	((mode->timing.v_total - 1) & RADEON_CRTC_V_TOTAL)
		| (((mode->timing.v_display - 1) << RADEON_CRTC_V_DISP_SHIFT) & RADEON_CRTC_V_DISP);

    vsync_wid = mode->timing.v_sync_end - mode->timing.v_sync_start;

	values->crtc_v_sync_strt_wid = 
    	((mode->timing.v_sync_start - 1) & RADEON_CRTC_V_SYNC_STRT)
		| (vsync_wid << RADEON_CRTC_V_SYNC_WID_SHIFT)
		| ((mode->flags & B_POSITIVE_VSYNC) == 0
		    ? RADEON_CRTC_V_SYNC_POL : 0);

    values->crtc_offset_cntl = 0;

	values->crtc_pitch = Radeon_RoundVWidth( mode->virtual_width, vc->bpp ) / 8;

	SHOW_FLOW( 2, "crtc_pitch=%ld", values->crtc_pitch );
	
    values->crtc_pitch |= values->crtc_pitch << 16;
}


// update shown are of one port
static void moveOneDisplay( accelerator_info *ai, virtual_head *virtual_head )
{
	virtual_card *vc = ai->vc;
	uint32 offset;
	
	offset = (vc->mode.v_display_start + virtual_head->rel_y) * vc->pitch + 
		(vc->mode.h_display_start + virtual_head->rel_x) * vc->bpp + 
		vc->fb_offset;
	
	SHOW_FLOW( 3, "Setting address %x on port %d", 
		offset,	virtual_head->physical_head );
	
	OUTREG( ai->regs, virtual_head->physical_head ? RADEON_CRTC2_OFFSET : RADEON_CRTC_OFFSET, offset );
}

// internal function: pan display
// engine lock should be hold
status_t Radeon_MoveDisplay( accelerator_info *ai, uint16 h_display_start, uint16 v_display_start )
{
	virtual_card *vc = ai->vc;
	
	SHOW_FLOW( 4, "h_display_start=%ld, v_display_start=%ld",
		h_display_start, v_display_start );

	if( h_display_start + vc->eff_width > vc->mode.virtual_width ||
		v_display_start + vc->eff_height > vc->mode.virtual_height )
		return B_ERROR;

	// this is needed both for get_mode_info and for scrolling of virtual screens
	vc->mode.h_display_start = h_display_start & ~7;
	vc->mode.v_display_start = v_display_start;

	// do it
	moveOneDisplay( ai, &vc->heads[0] );

	if( vc->independant_heads > 1 )
		moveOneDisplay( ai, &vc->heads[1] );
		
	// overlay position must be adjusted 
	Radeon_UpdateOverlay( ai );
	
	return B_OK;
}

// public function: pan display
status_t MOVE_DISPLAY( uint16 h_display_start, uint16 v_display_start ) 
{
	shared_info *si = ai->si;
	status_t result;
		
	ACQUIRE_BEN( si->engine.lock );
	
	// TBD: we should probably lock card first; in this case, we must
	//      split this function into locking and worker part, as this
	//      function is used internally as well
	result = Radeon_MoveDisplay( ai, h_display_start, v_display_start );
		
	RELEASE_BEN( si->engine.lock );
			
	return result;
}
