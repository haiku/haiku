/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Flat panel support
*/

#include "radeon_accelerant.h"
#include <malloc.h>
#include "mmio.h"
#include "fp_regs.h"
#include "ddc_regs.h"
#include "utils.h"
#include "ddc.h"

// calculcate flat panel crtc registers
void Radeon_CalcFPRegisters( accelerator_info *ai, virtual_port *port, fp_info *fp_port, display_mode *mode, port_regs *values )
{
	vuint8 *regs = ai->regs;
	uint xres = mode->timing.h_display;
	uint yres = mode->timing.v_display;
	uint64 Hratio, Vratio;

	// we read old values first, as we only want to change
	// some bits of them
	// (in general, we could setup all of them, but noone
	//  else does it, so we don't mess around with them as well)
    values->fp_gen_cntl          = INREG( regs, RADEON_FP_GEN_CNTL );
    values->fp_horz_stretch      = INREG( regs, RADEON_FP_HORZ_STRETCH );
    values->fp_vert_stretch      = INREG( regs, RADEON_FP_VERT_STRETCH );
    values->lvds_gen_cntl        = INREG( regs, RADEON_LVDS_GEN_CNTL );
    
    SHOW_FLOW( 2, "before: fp_gen_cntl=%lx, horz=%lx, vert=%lx, lvds_gen_cntl=%lx",
    	values->fp_gen_cntl, values->fp_horz_stretch, values->fp_vert_stretch, 
    	values->lvds_gen_cntl );

	if( xres > fp_port->panel_xres ) 
		xres = fp_port->panel_xres;
	if( yres > fp_port->panel_yres ) 
		yres = fp_port->panel_yres;
	
	// ouch: we must not use floating point in kernel,
	// we obey and use fixed point instead
	Hratio = FIX_SCALE * (uint32)xres / fp_port->panel_xres;
	Vratio = FIX_SCALE * (uint32)yres / fp_port->panel_yres;
	
	fp_port->h_ratio = Hratio;
	fp_port->v_ratio = Vratio;
	
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
	
	values->fp_gen_cntl = values->fp_gen_cntl & (uint32)
		~(RADEON_FP_SEL_CRTC2 |
		  RADEON_FP_RMX_HVSYNC_CONTROL_EN |
		  RADEON_FP_DFP_SYNC_SEL |	
		  RADEON_FP_CRT_SYNC_SEL | 
		  RADEON_FP_CRTC_LOCK_8DOT |
		  RADEON_FP_USE_SHADOW_EN |
		  RADEON_FP_CRTC_USE_SHADOW_VEND |
		  RADEON_FP_CRT_SYNC_ALT);
	values->fp_gen_cntl |= 
		RADEON_FP_CRTC_DONT_SHADOW_VPAR |
		RADEON_FP_CRTC_DONT_SHADOW_HEND;
		
	values->fp_gen_cntl |= port->is_crtc2 ? RADEON_FP_SEL_CRTC2 : 0;
/*	values->fp_gen_cntl |= RADEON_FP_SEL_CRTC2;
	values->fp_gen_cntl &= ~RADEON_FP_USE_SHADOW_EN;*/
	
	SHOW_FLOW( 3, "FP2: %d", INREG( ai->regs, RADEON_FP2_GEN_CNTL ));
		
	if( fp_port->disp_type == dt_lvds ) {
		values->lvds_gen_cntl  |= (RADEON_LVDS_ON | RADEON_LVDS_BLON);
		values->fp_gen_cntl    &= ~(RADEON_FP_FPON | RADEON_FP_TMDS_EN);
	} else if( fp_port->disp_type == dt_dvi_1 ) {
		values->fp_gen_cntl    |= (RADEON_FP_FPON | RADEON_FP_TMDS_EN);
		// enabling 8 bit data may be dangerous; BIOS should have taken care of that
		values->fp_gen_cntl |= RADEON_FP_PANEL_FORMAT;
	}
		
	/*values->fp_gen_cntl = RADEON_FP_SEL_CRTC2
		| RADEON_FP_CRTC_LOCK_8DOT;*/
		
    SHOW_FLOW( 2, "after: fp_gen_cntl=%lx, horz=%lx, vert=%lx, lvds_gen_cntl=%lx",
    	values->fp_gen_cntl, values->fp_horz_stretch, values->fp_vert_stretch, 
    	values->lvds_gen_cntl );
}

// write flat panel registers
void Radeon_ProgramFPRegisters( accelerator_info *ai, fp_info *fp_port, port_regs *values )
{
	uint32 tmp;
	vuint8 *regs = ai->regs;
	
	SHOW_FLOW0( 2, "" );
	
	OUTREG( regs, RADEON_FP_HORZ_STRETCH,      values->fp_horz_stretch );
	OUTREG( regs, RADEON_FP_VERT_STRETCH,      values->fp_vert_stretch );
	OUTREG( regs, RADEON_FP_GEN_CNTL,          values->fp_gen_cntl );
	
	if( fp_port->disp_type == dt_lvds ) {
		tmp = INREG( regs, RADEON_LVDS_GEN_CNTL );
		
		SHOW_FLOW( 3, "old: %x, new: %x", tmp, values->lvds_gen_cntl );
		
		if((tmp & (RADEON_LVDS_ON | RADEON_LVDS_BLON)) ==
			(values->lvds_gen_cntl & (RADEON_LVDS_ON | RADEON_LVDS_BLON)) )
		{
			SHOW_FLOW0( 3, "Write through" );
			OUTREG( regs, RADEON_LVDS_GEN_CNTL, values->lvds_gen_cntl );
		} else {
			if( values->lvds_gen_cntl & (RADEON_LVDS_ON | RADEON_LVDS_BLON) ) {
				SHOW_FLOW0( 3, "Switching off" );
				//snooze( fp_port->panel_pwr_delay * 1000);
				OUTREG( regs, RADEON_LVDS_GEN_CNTL, values->lvds_gen_cntl );
			} else {
				SHOW_FLOW0( 3, "Switching on" );
				OUTREG( regs, RADEON_LVDS_GEN_CNTL,
					values->lvds_gen_cntl | RADEON_LVDS_BLON );
				//snooze( fp_port->panel_pwr_delay * 1000 );
				OUTREG( regs, RADEON_LVDS_GEN_CNTL, values->lvds_gen_cntl );
			}
		}
	}
}

typedef struct {
	accelerator_info *ai;
	uint32 port;
} ddc_port_info;

static status_t get_signals( void *cookie, int *clk, int *data )
{
	ddc_port_info *info = (ddc_port_info *)cookie;
	vuint8 *regs = info->ai->regs;
	uint32 value;
	
	value = INREG( regs, info->port );
	
	*clk = (value >> RADEON_GPIO_Y_SHIFT_1) & 1;
	*data = (value >> RADEON_GPIO_Y_SHIFT_0) & 1;
	
	return B_OK;
}

static status_t set_signals( void *cookie, int clk, int data )
{
	ddc_port_info *info = (ddc_port_info *)cookie;
	vuint8 *regs = info->ai->regs;
	uint32 value;

	value = INREG( regs, info->port );
	value &= ~(RADEON_GPIO_A_1 | RADEON_GPIO_A_0);
	value &= ~(RADEON_GPIO_EN_0 | RADEON_GPIO_EN_1);		
	value |= ((1-clk) << RADEON_GPIO_EN_SHIFT_1) | ((1-data) << RADEON_GPIO_EN_SHIFT_0);

	OUTREG( regs, info->port, value );
	
	return B_OK;
}

// read edid data of flat panel and setup its timing accordingly
status_t Radeon_ReadFPEDID( accelerator_info *ai, shared_info *si )
{
	i2c_bus bus;
	ddc_port_info info;
	edid1_info edid;
	fp_info *fp = &si->fp_port;
	status_t res;
	void *vdif;
	size_t vdif_len;
	uint32 max_hsize, max_vsize;
	int i;

	info.ai = ai;
	info.port = RADEON_GPIO_DVI_DDC;
//	info.port = RADEON_GPIO_VGA_DDC;
	
	bus.cookie = &info;
	bus.set_signals = &set_signals;
	bus.get_signals = &get_signals;

	// get edid	
	res = ddc2_read_edid1( &bus, &edid, &vdif, &vdif_len );
	if( res != B_OK )
		return res;
		
	if( vdif != NULL )
		free( vdif );

	SHOW_FLOW0( 2, "EDID data read from DVI port via DDC2:" );
	edid_dump( &edid );

	// find detailed timing with maximum resolution
	max_hsize = max_vsize = 0;
		
	for( i = 0; i < EDID1_NUM_DETAILED_MONITOR_DESC; ++i ) {
		if( edid.detailed_monitor[i].monitor_desc_type == edid1_is_detailed_timing ) {
			edid1_detailed_timing *timing = &edid.detailed_monitor[i].data.detailed_timing;
			
			if( timing->h_size > max_hsize && timing->v_size > max_vsize ) {
				SHOW_FLOW( 2, "Found DDC data for mode %dx%d", 
					(int)timing->h_active, (int)timing->v_active );
					
				max_hsize = timing->h_active;
				max_vsize = timing->v_active;

				// copy it to timing specification			
				fp->panel_xres = timing->h_active;
				fp->h_blank = timing->h_blank;
				fp->h_over_plus = timing->h_sync_off;
				fp->h_sync_width = timing->h_sync_width;
				
				fp->panel_yres = timing->v_active;
				fp->v_blank = timing->v_blank;
				fp->v_over_plus = timing->v_sync_off;
				fp->v_sync_width = timing->v_sync_width;

				// BeOS uses kHz, but the timing is in 10 kHz
				fp->dot_clock = timing->pixel_clock * 10;
			}
		}
	}

	if( max_hsize == 0 )
		return B_ERROR;
			
	SHOW_INFO( 2, "h_disp=%d, h_blank=%d, h_over_plus=%d, h_sync_width=%d", 
		fp->panel_xres, fp->h_blank, fp->h_over_plus, fp->h_sync_width );
	SHOW_INFO( 2, "v_disp=%d, v_blank=%d, v_over_plus=%d, v_sync_width=%d", 
		fp->panel_yres, fp->v_blank, fp->v_over_plus, fp->v_sync_width );			
	SHOW_INFO( 2, "pixel_clock=%d kHz", fp->dot_clock );

	return B_OK;
}
