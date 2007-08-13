/*
	Copyright (c) 2002-2004, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Sets display modes, colour palette and handles DPMS
*/


#include "GlobalData.h"
#include "generic.h"
#include <sys/ioctl.h>
#include "radeon_regs.h"
#include "mmio.h"
#include "crtc_regs.h"
#include <GraphicsDefs.h>

#include "crtc_regs.h"
#include "overlay_regs.h"
#include "rbbm_regs.h"
#include "dac_regs.h"
#include "fp_regs.h"
#include "gpiopad_regs.h"

#include "pll_regs.h"
#include "tv_out_regs.h"
#include "config_regs.h"
#include "ddc_regs.h"
#include "pll_access.h"
#include "theatre_regs.h"
#include "set_mode.h"
#include "ddc.h"

#include "set_mode.h"

#include <string.h>

status_t Radeon_SetMode( 
	accelerator_info *ai, crtc_info *crtc, display_mode *mode, impactv_params *tv_params );

// round virtual width up to next valid size
uint32 Radeon_RoundVWidth( 
	int virtual_width, int bpp )
{
	// we have to make both the CRTC and the accelerator happy:
	// - the CRTC wants virtual width in pixels to be a multiple of 8
	// - the accelerator expects width in bytes to be a multiple of 64

	// to put that together, width (in bytes) must be a multiple of the least 
	// common nominator of bytes-per-pixel*8 (CRTC) and 64 (accelerator);
	
	// if bytes-per-pixel is a power of two and less than 8, the LCM is 64;
	// almost all colour depth satisfy that apart from 24 bit; in this case,
	// the LCM is 64*3=192
	
	// after dividing by bytes-per-pixel we get pixels: in first case,
	// width must be multiple of 64/bytes-per-pixel; in second case,
	// width must be multiple of 64*3/3=64
	
	if( bpp != 3 )
		return (virtual_width + 64/bpp - 1) & ~(64/bpp - 1);
	else
		return (virtual_width + 63) & ~63;
}


// list of registers that must be reset before display mode switch
// to avoid interferences
static struct {
	uint16 reg;
	uint32 val;
} common_regs[] = {
	{ RADEON_OVR_CLR, 0 },	
	{ RADEON_OVR_WID_LEFT_RIGHT, 0 },
	{ RADEON_OVR_WID_TOP_BOTTOM, 0 },
	{ RADEON_OV0_SCALE_CNTL, 0 },		// disable overlay
	{ RADEON_SUBPIC_CNTL, 0 },
	{ RADEON_I2C_CNTL_1, 0 },
};


static void Radeon_InitCommonRegs( 
	accelerator_info *ai )
{
	vuint8 *regs = ai->regs;
	uint i;
	
	for( i = 0; i < sizeof( common_regs) / sizeof( common_regs[0] ); ++i )
		OUTREG( regs, common_regs[i].reg, common_regs[i].val );
		
	// enable extended display modes
	OUTREGP( regs, RADEON_CRTC_GEN_CNTL, 
		RADEON_CRTC_EXT_DISP_EN, ~RADEON_CRTC_EXT_DISP_EN );
		
	// disable flat panel auto-centering 
	// (if we have a CRT on CRTC1, this must be disabled;
	//  if we have a flat panel on CRTC1, we setup CRTC manually, not
	//  using the auto-centre, automatic-sync-override magic)
	OUTREG( regs, RADEON_CRTC_MORE_CNTL, 0 );
}


// set display mode of one head;
// port restrictions, like fixed-sync TFTs connected to it, are taken care of
status_t Radeon_SetMode( 
	accelerator_info *ai, crtc_info *crtc, display_mode *mode, impactv_params *tv_params )
{
	virtual_card *vc = ai->vc;
	shared_info *si = ai->si;
	vuint8 *regs = ai->regs;
	int    format;
	int    bpp;
	display_device_e disp_devices;
	fp_info *fp_info;
	
	crtc_regs		crtc_values;
	pll_regs		pll_values;
	fp_regs			fp_values;
	impactv_regs	impactv_values;
	uint32			surface_cntl;
	
	bool internal_tv_encoder;
	pll_dividers dividers;
	
	crtc->mode = *mode;
	
	// don't destroy passed values, use our copy instead
	mode = &crtc->mode;
	
	disp_devices = crtc->chosen_displays;
	fp_info = &si->flatpanels[crtc->flatpanel_port];
	
	// if using an flat panel or LCD, maximum resolution
	// is determined by the physical resolution; 
	// also, all timing is fixed
	if( (disp_devices & (dd_lvds | dd_dvi | dd_dvi_ext)) != 0 ) {
		SHOW_FLOW0( 0, "requested resolution higher than native panel" );
		if( mode->timing.h_display > fp_info->panel_xres )
			mode->timing.h_display = fp_info->panel_xres;
		if(	mode->timing.v_display > fp_info->panel_yres )
			mode->timing.v_display = fp_info->panel_yres;
		
		if( (disp_devices & dd_dvi_ext) != 0 ) {
			SHOW_FLOW0( 0, "requested resolution less than second native panel" );
			if( mode->timing.h_display < fp_info->panel_xres )
				mode->timing.h_display = fp_info->panel_xres;
			if(	mode->timing.v_display < fp_info->panel_yres )
				mode->timing.v_display = fp_info->panel_yres;

			//TODO at this point we know we are going to do centered timing
			//need to set flags to a. blank the unused memory, b.center screen
			//for now it's in the top corner, and surrounded by garbage.
			// although if the DVI panels are the same size and we are cloning
			// we can switch the FP2 source to RMX, and drive both screens from
			// the RMX unit.
		}
		mode->timing.h_total = mode->timing.h_display + fp_info->h_blank;
		mode->timing.h_sync_start = mode->timing.h_display + fp_info->h_over_plus;
		mode->timing.h_sync_end = mode->timing.h_sync_start + fp_info->h_sync_width;
		mode->timing.v_total = mode->timing.v_display + fp_info->v_blank;
		mode->timing.v_sync_start = mode->timing.v_display + fp_info->v_over_plus;
		mode->timing.v_sync_end = mode->timing.v_sync_start + fp_info->v_sync_width;
		
		mode->timing.pixel_clock = fp_info->dot_clock;
	}
	
	// TV-out supports at most 1024x768
	if( (disp_devices & (dd_ctv | dd_stv)) != 0 ) {
		if( mode->timing.h_display > 1024 )
			mode->timing.h_display = 1024;
			
		if( mode->timing.v_display > 768 )
			mode->timing.v_display = 768;
	}

	// if using TV-Out, the timing of the source signal must be tweaked to
	// get proper timing
	internal_tv_encoder = IS_INTERNAL_TV_OUT( si->tv_chip );
	
	// we need higher accuracy then Be thought of
	mode->timing.pixel_clock *= 1000;

	// TV stuff must be done first as it tweaks the display mode
	if( (disp_devices & (dd_ctv | dd_stv)) != 0 ) {
		display_mode tweaked_mode;
		
		Radeon_CalcImpacTVParams( 
			&si->pll, tv_params, vc->tv_standard, internal_tv_encoder, 
			mode, &tweaked_mode );
			
		*mode = tweaked_mode;
	}
	
	Radeon_GetFormat( mode->space, &format, &bpp );
	
	vc->bpp = bpp;
	vc->datatype = format;  
	
	// time to read original register content
	// lock hardware so noone bothers us
	Radeon_WaitForIdle( ai, true );
		
	if( (disp_devices & (dd_dvi | dd_lvds | dd_dvi_ext)) != 0 ) {
		if( crtc->crtc_idx == 0 )
			Radeon_ReadRMXRegisters( ai, &fp_values );
			
		Radeon_ReadFPRegisters( ai, &fp_values );
	}

	if( (disp_devices & (dd_ctv | dd_stv)) != 0 ) {
		// some register's content isn't created from scratch but
		// only modified, so we need the original content first
		if( internal_tv_encoder )
			Radeon_InternalTVOutReadRegisters( ai, &impactv_values );
		else
			Radeon_TheatreReadTVRegisters( ai, &impactv_values );
	}


	// calculate all hardware register values
	Radeon_CalcCRTCRegisters( ai, crtc, mode, &crtc_values );

	surface_cntl = RADEON_SURF_TRANSLATION_DIS;

	if( (disp_devices & (dd_ctv | dd_stv)) != 0 ) {
		Radeon_CalcImpacTVRegisters( ai, mode, tv_params, &impactv_values, 
			crtc->crtc_idx, internal_tv_encoder, vc->tv_standard, disp_devices );
	}

	if( (disp_devices & (dd_stv | dd_ctv)) == 0 )
		Radeon_CalcCRTPLLDividers( &si->pll, mode, &dividers );
	else
		dividers = tv_params->crt_dividers;

	if( (disp_devices & dd_lvds) != 0 && si->flatpanels[0].fixed_dividers ) {
		SHOW_FLOW0( 0, "Using fixed dividers for laptop panel" );
		dividers.feedback = si->flatpanels[0].feedback_div;
		dividers.post_code = si->flatpanels[0].post_div;
		dividers.ref = si->flatpanels[0].ref_div;
	}
	
	Radeon_CalcPLLRegisters( mode, &dividers, &pll_values );
	
	// for first CRTC1, we need to setup RMX properly
	if( crtc->crtc_idx == 0 )
		Radeon_CalcRMXRegisters( fp_info, mode, 
			(disp_devices & (dd_lvds | dd_dvi | dd_dvi_ext)) != 0,
			&fp_values );
			
	if( (disp_devices & (dd_lvds | dd_dvi | dd_dvi_ext)) != 0 )
		Radeon_CalcFPRegisters( ai, crtc, fp_info, &crtc_values, &fp_values );
	
	// we don't use pixel clock anymore, so it can be reset to Be's kHz
	mode->timing.pixel_clock /= 1000;
	
	// write values to registers
	
	Radeon_InitCommonRegs( ai );
	
	Radeon_ProgramCRTCRegisters( ai, crtc->crtc_idx, &crtc_values );
	
	OUTREG( regs, RADEON_SURFACE_CNTL, surface_cntl );

	if( crtc->crtc_idx == 0 )
		Radeon_ProgramRMXRegisters( ai, &fp_values );

	if( (disp_devices & (dd_lvds | dd_dvi | dd_dvi_ext)) != 0 )
		Radeon_ProgramFPRegisters( ai, crtc, fp_info, &fp_values );
		
	//if( mode->timing.pixel_clock )
	Radeon_ProgramPLL( ai, crtc->crtc_idx, &pll_values );
	
	if( (disp_devices & (dd_ctv | dd_stv)) != 0 ) {
		if( internal_tv_encoder )
			Radeon_InternalTVOutProgramRegisters( ai, &impactv_values );
		else
			Radeon_TheatreProgramTVRegisters( ai, &impactv_values );
	}

	// spit out some debug stuff in a radeontool stylee
	SHOW_FLOW0( 0, "" );
	SHOW_FLOW( 0, "RADEON_DAC_CNTL %08X ", INREG( regs, RADEON_DAC_CNTL ));
	SHOW_FLOW( 0, "RADEON_DAC_CNTL2 %08X ", INREG( regs, RADEON_DAC_CNTL2 ));
	SHOW_FLOW( 0, "RADEON_TV_DAC_CNTL %08X ", INREG( regs, RADEON_TV_DAC_CNTL ));
	SHOW_FLOW( 0, "RADEON_DISP_OUTPUT_CNTL %08X ", INREG( regs, RADEON_DISP_OUTPUT_CNTL ));
	SHOW_FLOW( 0, "RADEON_AUX_SC_CNTL %08X ", INREG( regs, RADEON_AUX_SC_CNTL ));
	SHOW_FLOW( 0, "RADEON_CRTC_EXT_CNTL %08X ", INREG( regs, RADEON_CRTC_EXT_CNTL ));
	SHOW_FLOW( 0, "RADEON_CRTC_GEN_CNTL %08X ", INREG( regs, RADEON_CRTC_GEN_CNTL ));
	SHOW_FLOW( 0, "RADEON_CRTC2_GEN_CNTL %08X ", INREG( regs, RADEON_CRTC2_GEN_CNTL ));
	SHOW_FLOW( 0, "RADEON_DISP_MISC_CNTL %08X ", INREG( regs, RADEON_DISP_MISC_CNTL ));
	SHOW_FLOW( 0, "RADEON_FP_GEN_CNTL %08X ", INREG( regs, RADEON_FP_GEN_CNTL ));
	SHOW_FLOW( 0, "RADEON_FP2_GEN_CNTL %08X ", INREG( regs, RADEON_FP2_GEN_CNTL ));
	SHOW_FLOW( 0, "RADEON_LVDS_GEN_CNTL %08X ", INREG( regs, RADEON_LVDS_GEN_CNTL ));
	SHOW_FLOW( 0, "RADEON_TMDS_PLL_CNTL %08X ", INREG( regs, RADEON_TMDS_PLL_CNTL ));
	SHOW_FLOW( 0, "RADEON_TMDS_TRANSMITTER_CNTL %08X ", INREG( regs, RADEON_TMDS_TRANSMITTER_CNTL ));
	SHOW_FLOW( 0, "RADEON_FP_H_SYNC_STRT_WID %08X ", INREG( regs, RADEON_FP_H_SYNC_STRT_WID ));
	SHOW_FLOW( 0, "RADEON_FP_V_SYNC_STRT_WID %08X ", INREG( regs, RADEON_FP_V_SYNC_STRT_WID ));
	SHOW_FLOW( 0, "RADEON_FP_H2_SYNC_STRT_WID %08X ", INREG( regs, RADEON_FP_H2_SYNC_STRT_WID ));
	SHOW_FLOW( 0, "RADEON_FP_V2_SYNC_STRT_WID %08X ", INREG( regs, RADEON_FP_V2_SYNC_STRT_WID ));
	// spit end

	crtc->active_displays = disp_devices;
	
	// programming is over, so hardware can be used again
	RELEASE_BEN( si->cp.lock );
	
	// overlay must be setup again after modeswitch (whoever was using it)
	// TBD: this won't work if another virtual card was using it,
	// but currently, virtual cards don't work anyway...
	si->active_overlay.crtc_idx = -1;

	return B_OK;
}


// enable or disable VBlank interrupts
void Radeon_EnableIRQ( 
	accelerator_info *ai, bool enable )
{
	shared_info *si = ai->si;
	uint32 int_cntl, int_mask;
	
	int_cntl = INREG( ai->regs, RADEON_GEN_INT_CNTL );
	int_mask = 
		RADEON_CRTC_VBLANK_MASK
		| (si->num_crtc > 1 ? RADEON_CRTC2_VBLANK_MASK : 0);
		
	if( enable )
		int_cntl |= int_mask;
	else
		int_cntl &= ~int_mask;
		
	OUTREG( ai->regs, RADEON_GEN_INT_CNTL, int_cntl );
	
	if( enable ) {
		// on enable, we have to acknowledge all IRQs as the graphics card
		// waits for that before it issues further IRQs
		OUTREG( ai->regs, RADEON_GEN_INT_STATUS, int_cntl );
	}

	si->enable_virtual_irq = enable;
}


// public function: set display mode
status_t SET_DISPLAY_MODE( 
	display_mode *mode_in ) 
{
	virtual_card *vc = ai->vc;
	shared_info *si = ai->si;
	display_mode bounds, mode;

	mode = bounds = *mode_in;
	
	ACQUIRE_BEN( si->engine.lock );
	
	SHOW_FLOW( 2, "width=%d, height=%d", mode.timing.h_display, mode.timing.v_display );

	// check mode and tweak parameters so we can program hardware
	// without any further checks			
	if( PROPOSE_DISPLAY_MODE( &mode, &bounds, &bounds ) == B_ERROR ) {
		SHOW_ERROR0( 2, "invalid mode" );
		
		RELEASE_BEN( si->engine.lock );
		return B_ERROR;
	}

	// already done by propose_display_mode, but it was undone on return;
	// do this before equality check to recognize changes of multi-monitor mode 
	Radeon_DetectMultiMode( vc, &mode );
	
	// mode switches can take quite long and are visible, 
	// so avoid them if possible
	if( memcmp( &mode, &vc->mode, sizeof( display_mode )) == 0 &&
		!vc->enforce_mode_change ) {
		RELEASE_BEN( si->engine.lock );
		return B_OK;
	}
	
	// this flag was set when some internal parameter has changed that
	// affects effective display mode
	vc->enforce_mode_change = false;
	
	// make sure, we don't get disturbed
	//Radeon_Finish( ai );
	Radeon_EnableIRQ( ai, false );

	// free cursor and framebuffer memory
	{
		radeon_free_mem fm;
		
		fm.magic = RADEON_PRIVATE_DATA_MAGIC;
		fm.memory_type = mt_local;
		fm.global = true;
	
		if( vc->cursor.mem_handle ) {
			fm.handle = vc->cursor.mem_handle;
			ioctl( ai->fd, RADEON_FREE_MEM, &fm );
		}
		
		if( vc->fb_mem_handle ) {
			fm.handle = vc->fb_mem_handle;
			ioctl( ai->fd, RADEON_FREE_MEM, &fm );
		}
	}
	
	memcpy( &vc->mode, &mode, sizeof( display_mode ));
	
	// verify hardware restrictions *after* saving mode
	// e.g. if you want a span mode but have one monitor disconnected,
	// configuration shouldn't be touched, so you can continue working
	// with two monitors later on just like nothing has happened
	Radeon_VerifyMultiMode( vc, si, &mode );

	// set main flags	
	vc->independant_heads = vc->assigned_crtc[0] && si->crtc[0].chosen_displays != dd_none;
	
	if( si->num_crtc > 1 )
		vc->independant_heads += vc->assigned_crtc[1] && si->crtc[1].chosen_displays != dd_none;
		
	vc->different_heads = Radeon_DifferentPorts( &mode );
	SHOW_FLOW( 2, "independant heads: %d, different heads: %d", 
		vc->independant_heads, vc->different_heads );
	vc->scroll = mode.flags & B_SCROLL;
	SHOW_FLOW( 2, "scrolling %s", vc->scroll ? "enabled" : "disabled" );

	// allocate frame buffer and cursor image memory
	{
		radeon_alloc_mem am;
		int format, bpp;
		
		// alloc cursor memory		
		am.magic = RADEON_PRIVATE_DATA_MAGIC;
		am.size = 1024;
		am.memory_type = mt_local;
		am.global = true;
		
		if( ioctl( ai->fd, RADEON_ALLOC_MEM, &am ) == B_OK ) {
			vc->cursor.mem_handle = am.handle;
			vc->cursor.fb_offset = am.offset;
		} else {
			// too bad that we are out of mem -> set reasonable values as
			// it's too late to give up (ouch!)
			SHOW_ERROR0( 2, "no memory for cursor image!" );
			vc->cursor.mem_handle = 0;
			vc->cursor.fb_offset = 0;
		}
		
		vc->cursor.data = si->local_mem + vc->cursor.fb_offset;
	
		// alloc frame buffer
		Radeon_GetFormat( mode.space, &format, &bpp );
		vc->pitch = Radeon_RoundVWidth( mode.virtual_width, bpp ) * bpp;
		am.size = vc->pitch * mode.virtual_height;
		
		if( ioctl( ai->fd, RADEON_ALLOC_MEM, &am ) == B_OK ) {
			vc->fb_mem_handle = am.handle;
			vc->fb_offset = am.offset;
		} else 	{
			// ouch again - set reasonable values
			SHOW_ERROR0( 2, "no memory for frame buffer!" );
			vc->fb_mem_handle = 0;
			vc->fb_offset = 1024;
		}
		
		vc->fbc.frame_buffer = si->local_mem + vc->fb_offset;
		vc->fbc.frame_buffer_dma = (void *)((uint8 *)si->framebuffer_pci + vc->fb_offset);
		vc->fbc.bytes_per_row = vc->pitch;
		
		SHOW_FLOW( 0, "frame buffer CPU-address=%x, phys-address=%x", 
			vc->fbc.frame_buffer, vc->fbc.frame_buffer_dma );
	}
	
	// multi-screen stuff
	Radeon_InitMultiModeVars( ai, &mode );
	
	// GO!	

	{
		routing_regs routing_values;
		impactv_params tv_params;
		status_t err1 , err2;
		err1 = err2 = B_OK;
	
		// we first switch off all output, so the monitor(s) won't get invalid signals
		if( vc->assigned_crtc[0] ) {
			// overwrite list of active displays to switch off displays 
			// someone else turned on
			si->crtc[0].active_displays = vc->controlled_displays;
			Radeon_SetDPMS( ai, 0, B_DPMS_SUSPEND );
		}
		if( vc->assigned_crtc[1] ) {
			si->crtc[1].active_displays = vc->controlled_displays;
			Radeon_SetDPMS( ai, 1, B_DPMS_SUSPEND );
		}
		
		// mark crtc that will be used from now on
		vc->used_crtc[0] = vc->assigned_crtc[0] && si->crtc[0].chosen_displays != dd_none;
		vc->used_crtc[1] = vc->assigned_crtc[1] && si->crtc[1].chosen_displays != dd_none;
	
		// then change the mode
		if( vc->used_crtc[0] )
			err1 = Radeon_SetMode( ai, &si->crtc[0], &mode, &tv_params );
		if( vc->used_crtc[1] )
			err2 = Radeon_SetMode( ai, &si->crtc[1], &mode, &tv_params );
			
		
		SHOW_FLOW( 2, "SetModes 1=%s, 2=%s", 
			(err1 == B_OK) ? "OK" : "FAIL", (err2 == B_OK) ? "OK" : "FAIL");

		// setup signal routing
		Radeon_ReadMonitorRoutingRegs( ai, &routing_values );
		Radeon_CalcMonitorRouting( ai, &tv_params, &routing_values );
		Radeon_ProgramMonitorRouting( ai, &routing_values );
	   	
		// finally, switch display(s) on
		if( vc->used_crtc[0] )
			Radeon_SetDPMS( ai, 0, (err1 == B_OK) ? B_DPMS_ON : B_DPMS_SUSPEND );
		if( vc->used_crtc[1] )
			Radeon_SetDPMS( ai, 1, (err2 == B_OK) ? B_DPMS_ON : B_DPMS_SUSPEND );
			
		OUTREGP( ai->regs, RADEON_CRTC_EXT_CNTL, 0, ~RADEON_CRTC_DISPLAY_DIS );
	}
	
   	SHOW_FLOW( 3, "pitch=%ld", vc->pitch );
   	
   	// we'll modify bits of this reg, so save it for async access
	si->dac_cntl2 = INREG( ai->regs, RADEON_DAC_CNTL2 );
	
	// setup 2D registers
	Radeon_Init2D( ai );
	// setup position of framebuffer for 2D commands
	Radeon_FillStateBuffer( ai, vc->datatype );
	
	// remember that 2D accelerator is not prepared for any virtual card
	si->active_vc = -1;
	
	// first move to well-defined position (to setup CRTC offset)
	Radeon_MoveDisplay( ai, 0, 0 );		
	// then to (probably faulty) user-defined pos
	Radeon_MoveDisplay( ai, mode.h_display_start, mode.v_display_start );

	// set standard palette in direct-colour modes
	if( vc->used_crtc[0] )
		Radeon_InitPalette( ai, 0 );
	if( vc->used_crtc[1] )
		Radeon_InitPalette( ai, 1 );
	
	// initialize cursor data
	if( vc->used_crtc[0] )
		Radeon_SetCursorColors( ai, 0 );
	if( vc->used_crtc[1] )
		Radeon_SetCursorColors( ai, 1 );
		
	// sync should be settled now, so we can reenable IRQs
	Radeon_EnableIRQ( ai, true );
		
	RELEASE_BEN( si->engine.lock );

	// !! all this must be done after lock has been 
	//    released to avoid dead-lock !!
	// TBD: any invalid intermediate states?

	// move_cursor sets all cursor-related variables and registers
	vc->cursor.is_visible = false;
	MOVE_CURSOR( 0, 0 );

	return B_OK;
}
