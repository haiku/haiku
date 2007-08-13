/*
	Copyright (c) 2002-2004, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Flat panel support
*/

#include "radeon_accelerant.h"
#include "mmio.h"
#include "fp_regs.h"
#include "memcntrl_regs.h"
#include "utils.h"
#include "set_mode.h"
#include "pll_regs.h"
#include "pll_access.h"


void Radeon_ReadRMXRegisters( 
	accelerator_info *ai, fp_regs *values )
{
	vuint8 *regs = ai->regs;

	values->fp_horz_stretch = INREG( regs, RADEON_FP_HORZ_STRETCH );
	values->fp_vert_stretch = INREG( regs, RADEON_FP_VERT_STRETCH );
}

void Radeon_CalcRMXRegisters( 
	fp_info *flatpanel, display_mode *mode, bool use_rmx, fp_regs *values )
{
	uint xres = mode->timing.h_display;
	uint yres = mode->timing.v_display;
	uint64 Hratio, Vratio;

	if( !use_rmx ) {
		// disable RMX unit if requested
		values->fp_horz_stretch &= 
			~(RADEON_HORZ_STRETCH_BLEND |
			  RADEON_HORZ_STRETCH_ENABLE);

		values->fp_vert_stretch &= 
			~(RADEON_VERT_STRETCH_ENABLE |
			  RADEON_VERT_STRETCH_BLEND);
			  
		return;
	}

	// RMX unit can only upscale, not downscale
	if( xres > flatpanel->panel_xres ) 
		xres = flatpanel->panel_xres;
	if( yres > flatpanel->panel_yres ) 
		yres = flatpanel->panel_yres;
	
	Hratio = FIX_SCALE * (uint32)xres / flatpanel->panel_xres;
	Vratio = FIX_SCALE * (uint32)yres / flatpanel->panel_yres;

	// save it for overlay unit (overlays must be vertically scaled manually)	
	flatpanel->h_ratio = Hratio;
	flatpanel->v_ratio = Vratio;
	
	values->fp_horz_stretch = flatpanel->panel_xres << RADEON_HORZ_PANEL_SIZE_SHIFT;
	
	if( Hratio == FIX_SCALE ) {
		values->fp_horz_stretch &= 
			~(RADEON_HORZ_STRETCH_BLEND |
			  RADEON_HORZ_STRETCH_ENABLE);
	} else {   
		uint32 stretch;
		
		stretch = (uint32)((Hratio * RADEON_HORZ_STRETCH_RATIO_MAX +
			FIX_SCALE / 2) >> FIX_SHIFT) & RADEON_HORZ_STRETCH_RATIO_MASK;
			
		values->fp_horz_stretch = stretch
			| (values->fp_horz_stretch & (RADEON_HORZ_PANEL_SIZE |
				RADEON_HORZ_FP_LOOP_STRETCH |
				RADEON_HORZ_AUTO_RATIO_INC));
		values->fp_horz_stretch |= 
			RADEON_HORZ_STRETCH_BLEND |
			RADEON_HORZ_STRETCH_ENABLE;
	}
	values->fp_horz_stretch &= ~RADEON_HORZ_AUTO_RATIO;
	
	values->fp_vert_stretch = flatpanel->panel_yres << RADEON_VERT_PANEL_SIZE_SHIFT;
	
	if( Vratio == FIX_SCALE ) {
		values->fp_vert_stretch &= 
			~(RADEON_VERT_STRETCH_ENABLE |
			  RADEON_VERT_STRETCH_BLEND);
	} else {
		uint32 stretch;

		stretch = (uint32)((Vratio * RADEON_VERT_STRETCH_RATIO_MAX +
			FIX_SCALE / 2) >> FIX_SHIFT) & RADEON_VERT_STRETCH_RATIO_MASK;
			
		values->fp_vert_stretch = stretch
			| (values->fp_vert_stretch & (RADEON_VERT_PANEL_SIZE |
				RADEON_VERT_STRETCH_RESERVED));
		values->fp_vert_stretch |=  
			RADEON_VERT_STRETCH_ENABLE |
			RADEON_VERT_STRETCH_BLEND;
	}
	values->fp_vert_stretch &= ~RADEON_VERT_AUTO_RATIO_EN;
}

// write RMX registers
void Radeon_ProgramRMXRegisters( 
	accelerator_info *ai, fp_regs *values )
{
	vuint8 *regs = ai->regs;
	SHOW_FLOW0( 2, "" );
	OUTREG( regs, RADEON_FP_HORZ_STRETCH, values->fp_horz_stretch );
	OUTREG( regs, RADEON_FP_VERT_STRETCH, values->fp_vert_stretch );
}


void Radeon_ReadFPRegisters( 
	accelerator_info *ai, fp_regs *values )
{
	vuint8 *regs = ai->regs;

	values->fp_gen_cntl = INREG( regs, RADEON_FP_GEN_CNTL );
	values->fp2_gen_cntl = INREG( regs, RADEON_FP2_GEN_CNTL );
	values->lvds_gen_cntl = INREG( regs, RADEON_LVDS_GEN_CNTL );
	values->tmds_pll_cntl = INREG( regs, RADEON_TMDS_PLL_CNTL );
	values->tmds_trans_cntl = INREG( regs, RADEON_TMDS_TRANSMITTER_CNTL );
	values->fp_h_sync_strt_wid = INREG( regs, RADEON_FP_H_SYNC_STRT_WID );
	values->fp_v_sync_strt_wid = INREG( regs, RADEON_FP_V_SYNC_STRT_WID );
	values->fp2_h_sync_strt_wid = INREG( regs, RADEON_FP_H2_SYNC_STRT_WID );
	values->fp2_v_sync_strt_wid = INREG( regs, RADEON_FP_V2_SYNC_STRT_WID );
	values->bios_4_scratch =  INREG( regs, RADEON_BIOS_4_SCRATCH );
	values->bios_5_scratch =  INREG( regs, RADEON_BIOS_5_SCRATCH );
	values->bios_6_scratch =  INREG( regs, RADEON_BIOS_6_SCRATCH );

	if (ai->si->asic == rt_rv280) {
		// bit 22 of TMDS_PLL_CNTL is read-back inverted
		values->tmds_pll_cntl ^= (1 << 22);
	}

	SHOW_FLOW( 2, "before: fp_gen_cntl=%08lx, horz=%08lx, vert=%08lx, lvds_gen_cntl=%08lx",
    		values->fp_gen_cntl, values->fp_horz_stretch, values->fp_vert_stretch, 
    		values->lvds_gen_cntl );
}

// calculcate flat panel crtc registers;
// must be called after normal CRTC registers are determined
void Radeon_CalcFPRegisters( 
	accelerator_info *ai, crtc_info *crtc, 
	fp_info *fp_port, crtc_regs *crtc_values, fp_regs *values )
{
	int i;
	uint32 tmp = values->tmds_pll_cntl & 0xfffff;

	// setup synchronization position
	// (most values are ignored according to fp_gen_cntl, but at least polarity
	//  and pixel precise horizontal sync position are always used)
	if( fp_port->is_fp2 ) {
		SHOW_FLOW0( 2, "is_fp2" );
		values->fp2_h_sync_strt_wid = crtc_values->crtc_h_sync_strt_wid;
		values->fp2_v_sync_strt_wid = crtc_values->crtc_v_sync_strt_wid;
	} else {
		SHOW_FLOW0( 2, "fp1" );
		values->fp_h_sync_strt_wid = crtc_values->crtc_h_sync_strt_wid;
		values->fp_v_sync_strt_wid = crtc_values->crtc_v_sync_strt_wid;
	}

	if( fp_port->is_fp2 ) {
		// should retain POST values (esp bit 28)
		values->fp2_gen_cntl &= (0xFFFF0000);

	} else {
		// setup magic CRTC shadowing 	
		values->fp_gen_cntl &= 
			~(RADEON_FP_RMX_HVSYNC_CONTROL_EN |
			  RADEON_FP_DFP_SYNC_SEL |	
			  RADEON_FP_CRT_SYNC_SEL | 
			  RADEON_FP_CRTC_LOCK_8DOT |
			  RADEON_FP_USE_SHADOW_EN |
			  RADEON_FP_CRTC_USE_SHADOW_VEND |
			  RADEON_FP_CRT_SYNC_ALT);
		values->fp_gen_cntl |= 
			RADEON_FP_CRTC_DONT_SHADOW_VPAR |
			RADEON_FP_CRTC_DONT_SHADOW_HEND;
	}
	
	for (i = 0; i < 4; i++) {
		if (ai->si->tmds_pll[i].freq == 0) 
			break;
		if ((uint32)(fp_port->dot_clock) < ai->si->tmds_pll[i].freq) {
			tmp = ai->si->tmds_pll[i].value ;
			break;
		}
	}

	if (IS_R300_VARIANT || (ai->si->asic == rt_rv280)) {
		if (tmp & 0xfff00000) {
			values->tmds_pll_cntl = tmp;
		} else {
			values->tmds_pll_cntl = ai->si->tmds_pll_cntl & 0xfff00000;
			values->tmds_pll_cntl |= tmp;
		}
	} else {
		values->tmds_pll_cntl = tmp;
	}

	values->tmds_trans_cntl = ai->si->tmds_transmitter_cntl 
		& ~(RADEON_TMDS_TRANSMITTER_PLLRST);

	if (IS_R300_VARIANT || (ai->si->asic == rt_r200) || (ai->si->num_crtc == 1))
		values->tmds_trans_cntl &= ~(RADEON_TMDS_TRANSMITTER_PLLEN);
	else // weird, RV chips got this bit reversed?
		values->tmds_trans_cntl |= (RADEON_TMDS_TRANSMITTER_PLLEN);


	// enable proper transmitter	
	if( (crtc->chosen_displays & dd_lvds) != 0 ) {
		// using LVDS means there cannot be a DVI monitor
		SHOW_FLOW0( 2, "lvds" );
		values->lvds_gen_cntl |= (RADEON_LVDS_ON | RADEON_LVDS_BLON);
		values->fp_gen_cntl &= ~(RADEON_FP_FPON | RADEON_FP_TMDS_EN);
		
	} else if( !fp_port->is_fp2 ) {
		// DVI on internal transmitter
		SHOW_FLOW0( 2, "DVI INT" );
		values->fp_gen_cntl |= RADEON_FP_FPON | RADEON_FP_TMDS_EN;
		// enabling 8 bit data may be dangerous; BIOS should have taken care of that
		values->fp_gen_cntl |= RADEON_FP_PANEL_FORMAT;
		
	} else {
		// DVI on external transmitter
		SHOW_FLOW0( 2, "DVI EXT" );
		values->fp2_gen_cntl |= RADEON_FP2_FPON | RADEON_FP_PANEL_FORMAT;
		values->fp2_gen_cntl &= ~RADEON_FP2_BLANK_EN;
		
		//hack in missing bits test...
		//values->fp2_gen_cntl |= (1 << 22) | (1 << 28);

		if( ai->si->asic >= rt_r200 )
			values->fp2_gen_cntl |= RADEON_FP2_DV0_EN;
	}
		
    SHOW_FLOW( 2, "after: fp_gen_cntl=%08lx, fp2_gen_cntl=%08lx, horz=%08lx, vert=%08lx, lvds_gen_cntl=%08lx",
    	values->fp_gen_cntl, values->fp2_gen_cntl, values->fp_horz_stretch, values->fp_vert_stretch, 
    	values->lvds_gen_cntl );
}


// write flat panel registers
void Radeon_ProgramFPRegisters( 
	accelerator_info *ai, crtc_info *crtc,
	fp_info *fp_port, fp_regs *values )
{
	shared_info *si = ai->si;
	vuint8 *regs = ai->regs;
		
	SHOW_FLOW0( 2, "" );
	
	OUTREGP( regs, RADEON_FP_GEN_CNTL, values->fp_gen_cntl, RADEON_FP_SEL_CRTC2 );

	if( fp_port->is_fp2 ) {
		SHOW_FLOW0( 2, "is_fp2" );
		OUTREGP( regs, RADEON_FP2_GEN_CNTL, values->fp2_gen_cntl, 
			~(RADEON_FP2_SOURCE_SEL_MASK | RADEON_FP2_SRC_SEL_MASK));
		OUTREGP( regs, RADEON_FP2_GEN_CNTL, values->fp2_gen_cntl, 
			RADEON_FP2_SOURCE_SEL_CRTC2 | RADEON_FP2_SRC_SEL_CRTC2 );
		OUTREG( regs, RADEON_FP_H2_SYNC_STRT_WID, values->fp2_h_sync_strt_wid );
		OUTREG( regs, RADEON_FP_V2_SYNC_STRT_WID, values->fp2_v_sync_strt_wid );
	} else {
		SHOW_FLOW0( 2, "is_fp1" );
		OUTREG( regs, RADEON_FP_H_SYNC_STRT_WID, values->fp_h_sync_strt_wid );
		OUTREG( regs, RADEON_FP_V_SYNC_STRT_WID, values->fp_v_sync_strt_wid );
	}
	
	// workaround for old AIW Radeon having display buffer underflow 
	// in conjunction with DVI
	if( si->asic == rt_r100 ) {
		OUTREG( regs, RADEON_GRPH_BUFFER_CNTL, 
			INREG( regs, RADEON_GRPH_BUFFER_CNTL) & ~0x7f0000);
	}
	
	if ( ai->si->is_mobility ) {
		OUTREG( regs, RADEON_BIOS_4_SCRATCH, values->bios_4_scratch);
		OUTREG( regs, RADEON_BIOS_5_SCRATCH, values->bios_5_scratch);
		OUTREG( regs, RADEON_BIOS_6_SCRATCH, values->bios_6_scratch);
    }

	if( (crtc->chosen_displays & dd_lvds) != 0 ) {

		//OUTREGP( regs, RADEON_LVDS_GEN_CNTL, values->lvds_gen_cntl, 
		//	RADEON_LVDS_ON | RADEON_LVDS_BLON );

		uint32 old_pixclks_cntl;
		uint32 tmp;

		old_pixclks_cntl = Radeon_INPLL( ai->regs, ai->si->asic, RADEON_PIXCLKS_CNTL);
		
		// ASIC bug: when LVDS_ON is reset, LVDS_ALWAYS_ON must be zero
		if( ai->si->is_mobility || ai->si->is_igp ) 
		{
			if (!(values->lvds_gen_cntl & RADEON_LVDS_ON)) {
				Radeon_OUTPLLP( ai->regs, ai->si->asic, RADEON_PIXCLKS_CNTL, 0, ~RADEON_PIXCLK_LVDS_ALWAYS_ONb );
			}
		}

		// get current state of LCD
		tmp = INREG( regs, RADEON_LVDS_GEN_CNTL);
		
		// if LCD is on, and previous state was on, just write the state directly.
		if (( tmp & ( RADEON_LVDS_ON | RADEON_LVDS_BLON )) == 
			( values->lvds_gen_cntl & ( RADEON_LVDS_ON | RADEON_LVDS_BLON ))) {
			OUTREG( regs, RADEON_LVDS_GEN_CNTL, values->lvds_gen_cntl );
		} else {
			if ( values->lvds_gen_cntl & ( RADEON_LVDS_ON | RADEON_LVDS_BLON )) {
				snooze( ai->si->panel_pwr_delay * 1000 );
				OUTREG( regs, RADEON_LVDS_GEN_CNTL, values->lvds_gen_cntl );
			} else { 
			
				//turn on backlight, wait for stable before turning on data ???
				OUTREG( regs, RADEON_LVDS_GEN_CNTL,	values->lvds_gen_cntl | RADEON_LVDS_BLON );
				snooze( ai->si->panel_pwr_delay * 1000 );
				OUTREG( regs, RADEON_LVDS_GEN_CNTL, values->lvds_gen_cntl );
			}
		}
		
		if( ai->si->is_mobility || ai->si->is_igp ) {
			if (!(values->lvds_gen_cntl & RADEON_LVDS_ON)) {
				Radeon_OUTPLL( ai->regs, ai->si->asic, RADEON_PIXCLKS_CNTL, old_pixclks_cntl );
			}
		}
	}
}
