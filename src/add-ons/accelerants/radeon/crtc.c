/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon accelerant
		
	CRTC programming
*/

#include "radeon_accelerant.h"
//#include "../include/radeon_regs.h"
#include "mmio.h"
#include "crtc_regs.h"
#include "dac_regs.h"

// hammer CRTC registers
void Radeon_ProgramCRTCRegisters( accelerator_info *ai, virtual_port *port, 
	port_regs *values )
{
	vuint8 *regs = ai->regs;
	
	SHOW_FLOW0( 2, "" );

	if( port->is_crtc2 ) {
		OUTREGP( regs, RADEON_CRTC2_GEN_CNTL, values->crtc_gen_cntl,
			RADEON_CRTC2_VSYNC_DIS |
			RADEON_CRTC2_HSYNC_DIS |
			RADEON_CRTC2_DISP_DIS );

		switch( ai->si->asic ) {
		case rt_r200:
		case rt_r300:
			OUTREG( regs, RADEON_DISP_OUTPUT_CNTL, values->disp_output_cntl );
			break;
		default:
			OUTREG( regs, RADEON_DAC_CNTL2, values->dac_cntl );
		}

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
			RADEON_CRTC_DISPLAY_DIS );
			
		OUTREGP( regs, RADEON_DAC_CNTL, values->dac_cntl,
			RADEON_DAC_RANGE_CNTL | RADEON_DAC_BLANKING );
			
		OUTREG( regs, RADEON_CRTC_H_TOTAL_DISP, values->crtc_h_total_disp );
		OUTREG( regs, RADEON_CRTC_H_SYNC_STRT_WID, values->crtc_h_sync_strt_wid );
		OUTREG( regs, RADEON_CRTC_V_TOTAL_DISP, values->crtc_v_total_disp );
		OUTREG( regs, RADEON_CRTC_V_SYNC_STRT_WID, values->crtc_v_sync_strt_wid );
		OUTREG( regs, RADEON_CRTC_OFFSET_CNTL, values->crtc_offset_cntl );
		OUTREG( regs, RADEON_CRTC_PITCH, values->crtc_pitch );
	}
}


// get required hsync delay depending on bit depth and output device
uint16 Radeon_GetHSyncFudge( shared_info *si, physical_port *port, int datatype )
{
	static int hsync_fudge_default[] = { 0x00, 0x12, 0x09, 0x09, 0x06, 0x05 };
	static int hsync_fudge_fp[]      = { 0x02, 0x02, 0x00, 0x00, 0x05, 0x05 };
	
	// there is an sync delay which depends on colour-depth and output device
	if( port->disp_type == dt_dvi_1 || port->disp_type == dt_dvi_2 || 
		port->disp_type == dt_lvds )
		return hsync_fudge_fp[datatype - 1];
    else               
    	return hsync_fudge_default[datatype - 1];	
}


// calculate CRTC register content
void Radeon_CalcCRTCRegisters( accelerator_info *ai, virtual_port *port, 
	display_mode *mode, port_regs *values )
{
	virtual_card *vc = ai->vc;
	shared_info *si = ai->si;
    int    hsync_start;
    int    hsync_wid;
    int    hsync_fudge;
    int    vsync_wid;
    display_type_e disp_type;
    physical_port *phys_port = &si->ports[port->physical_port];

	disp_type = si->ports[port->physical_port].disp_type;
	hsync_fudge = Radeon_GetHSyncFudge( si, phys_port, vc->datatype );

	if( port->is_crtc2 ) {
		values->crtc_gen_cntl = (RADEON_CRTC2_EN
			| RADEON_CRTC2_CRT2_ON
			| (vc->datatype << 8)
			| (0/*doublescan*/ ? RADEON_CRTC2_DBL_SCAN_EN	: 0)
			| ((mode->timing.flags & B_TIMING_INTERLACED)
				? RADEON_CRTC2_INTERLACE_EN	: 0));
				
		//values->crtc_gen_cntl &= ~RADEON_CRTC2_CRT2_ON;

		// make ports independant of each other
		switch( si->asic ) {
		case rt_r200:
		case rt_r300:
			values->disp_output_cntl = INREG( ai->regs, RADEON_DISP_OUTPUT_CNTL );
			values->disp_output_cntl = 
				(values->disp_output_cntl & ~RADEON_DISP_DAC_SOURCE_MASK)
				| RADEON_DISP_DAC_SOURCE_CRTC2;
			break;
		default:
			// we always use CRTC1 for FP and CRTC2 for CRT
			// a better way were to take output device into consideration too
			values->dac_cntl = INREG( ai->regs, RADEON_DAC_CNTL2 ) &
				~RADEON_DAC_CLK_SEL;
			values->dac_cntl |= RADEON_DAC_CLK_SEL_CRTC2;
		}
		
	} else {
		// here, we should set interlace/double scan mode
		// but we don't support them (anyone missing them?)
	    values->crtc_gen_cntl = (RADEON_CRTC_EXT_DISP_EN
			| RADEON_CRTC_EN
			| (vc->datatype << 8));

		// we shouldn't set CRT_ON if a flat panel is connected,
		// but this flag seems to be independant of CRTC the
		// CRT is connected to
		values->crtc_ext_cntl = 
	    	RADEON_VGA_ATI_LINEAR | 
			RADEON_XCRT_CNT_EN |
			RADEON_CRTC_CRT_ON;

		values->dac_cntl = RADEON_DAC_MASK_ALL
			| RADEON_DAC_VGA_ADR_EN
			| RADEON_DAC_8BIT_EN;
	}
	
    values->crtc_h_total_disp = 
    	((mode->timing.h_total / 8 - 1) & RADEON_CRTC_H_TOTAL)
		| (((mode->timing.h_display / 8 - 1) << RADEON_CRTC_H_DISP_SHIFT) & RADEON_CRTC_H_DISP);

    hsync_wid = (mode->timing.h_sync_end - mode->timing.h_sync_start) / 8;

    hsync_start = mode->timing.h_sync_start - 8 + hsync_fudge;

	// TBD: the sync may be the other way around
    values->crtc_h_sync_strt_wid = 
    	(hsync_start & (RADEON_CRTC_H_SYNC_STRT_CHAR | RADEON_CRTC_H_SYNC_STRT_PIX))
		| (hsync_wid << RADEON_CRTC_H_SYNC_WID_SHIFT)
		| ((mode->flags & B_POSITIVE_HSYNC) == 0 ? RADEON_CRTC_H_SYNC_POL : 0);
  
   	values->crtc_v_total_disp = 
    	((mode->timing.v_total - 1) & RADEON_CRTC_V_TOTAL)
		| (((mode->timing.v_display - 1) << RADEON_CRTC_V_DISP_SHIFT) & RADEON_CRTC_V_DISP);

    vsync_wid = mode->timing.v_sync_end - mode->timing.v_sync_start;

	// TBD: vertial sync may be the other way around
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
