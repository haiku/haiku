/*
	Copyright (c) 2002, Thomas Kurschel
	

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

#include "overlay_regs.h"
#include "capture_regs.h"
#include "rbbm_regs.h"
#include "dac_regs.h"

#include <string.h>


// round virtual width up to next valid size
uint32 Radeon_RoundVWidth( int virtual_width, int bpp )
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
	{ RADEON_OV0_SCALE_CNTL, 0 },
	{ RADEON_SUBPIC_CNTL, 0 },
	{ RADEON_VIPH_CONTROL, 0 },
	{ RADEON_I2C_CNTL_1, 0 },
	//{ RADEON_GEN_INT_CNTL, 0 },	// VBI irqs are handled seperately
	//{ RADEON_CAP0_TRIG_CNTL, 0 },	// leave capturing on during mode switch
};

static void Radeon_InitCommonRegs( accelerator_info *ai )
{
	vuint8 *regs = ai->regs;
	uint i;
	
	for( i = 0; i < sizeof( common_regs) / sizeof( common_regs[0] ); ++i )
		OUTREG( regs, common_regs[i].reg, common_regs[i].val );
}

// set display mode of one head;
// port restrictions, like fixed-sync TFTs connected to it, are taken care of
void Radeon_SetMode( accelerator_info *ai, physical_head *head, display_mode *mode )
{
	virtual_card *vc = ai->vc;
	shared_info *si = ai->si;
	vuint8 *regs = ai->regs;
	int    format;
	int    bpp;
	display_device_e disp_devices;
	fp_info *fp_info;
	port_regs values;
	tv_params tv_params;
	tv_standard tv_format = ts_ntsc;
	tv_timing *tv_timing = &Radeon_std_tv_timing[tv_format];
	bool internal_tv_encoder;
	
	head->mode = *mode;
	
	// don't destroy passed values, use our copy instead
	mode = &head->mode;
	
	disp_devices = head->chosen_displays;
	fp_info = &si->flatpanels[head->flatpanel_port];
	
	// if using an flat panel or LCD, maximum resolution
	// is determined by the physical resolution; 
	// also, all timing is fixed
	if( (disp_devices & (dd_lvds | dd_dvi | dd_dvi_ext )) != 0 ) {
		if( mode->timing.h_display > fp_info->panel_xres )
			mode->timing.h_display = fp_info->panel_xres;
		if(	mode->timing.v_display > fp_info->panel_yres )
			mode->timing.v_display = fp_info->panel_yres;
		
		mode->timing.h_total = mode->timing.h_display + fp_info->h_blank;
		mode->timing.h_sync_start = mode->timing.h_display + fp_info->h_over_plus;
		mode->timing.h_sync_end = mode->timing.h_sync_start + fp_info->h_sync_width;
		mode->timing.v_total = mode->timing.v_display + fp_info->v_blank;
		mode->timing.v_sync_start = mode->timing.v_display + fp_info->v_over_plus;
		mode->timing.v_sync_end = mode->timing.v_sync_start + fp_info->v_sync_width;
		
		mode->timing.pixel_clock = fp_info->dot_clock;
	}

	// if using TV-Out, the timing of the source signal must be tweaked to
	// get proper timing
	internal_tv_encoder = si->tv_chip != tc_external_rt1;
	
	// we need higher accuracy then Be thought of;
	mode->timing.pixel_clock *= 1000;

	if( (disp_devices & (dd_ctv | dd_stv)) != 0 ) {
		display_mode tweaked_mode;
		
		Radeon_CalcTVParams( &si->pll, &tv_params, tv_timing, internal_tv_encoder, 
			mode, &tweaked_mode );
			
		*mode = tweaked_mode;
	}
	
	Radeon_GetFormat( mode->space, &format, &bpp );
	
	vc->bpp = bpp;
	vc->datatype = format;  
	
	// time to read original register content
	// lock hardware so noone bothers us
	Radeon_WaitForIdle( ai, true );
	
	Radeon_ReadCRTCRegisters( ai, head, &values );
	Radeon_ReadMonitorRoutingRegs( ai, head, &values );
	
	if( (disp_devices & (dd_dvi | dd_lvds | dd_dvi_ext)) != 0 ) {
		if( !head->is_crtc2 )
			Radeon_ReadRMXRegisters( ai, &values );
			
		Radeon_ReadFPRegisters( ai, &values );
	}

	// calculate all hardware register values
	Radeon_CalcCRTCRegisters( ai, head, mode, &values );

	values.surface_cntl = RADEON_SURF_TRANSLATION_DIS;

	// for flat panels, we may not have pixel clock if DDC data is missing;
	// as we don't change effective resolution we can leave it as set by BIOS
	if( mode->timing.pixel_clock ) {
		Radeon_CalcPLLRegisters( &si->pll, mode/*->timing.pixel_clock / 10*/, 
			(/*(disp_devices & (dd_stv | dd_ctv)) != 0 ? &tv_params.crt_dividers : */NULL),
			&values );
	}
	
	// for first CRTC1, we need to setup RMX properly
	if( !head->is_crtc2 )
		Radeon_CalcRMXRegisters( fp_info, mode, 
			(disp_devices & (dd_lvds | dd_dvi | dd_dvi_ext)) != 0,
			&values );
			
	if( (disp_devices & (dd_lvds | dd_dvi | dd_dvi_ext)) != 0 )
		Radeon_CalcFPRegisters( ai, head, fp_info, &values );

	if( (disp_devices & (dd_ctv | dd_stv)) != 0 ) {
		Radeon_CalcTVRegisters( ai, mode, tv_timing, &tv_params, &values, 
			head, internal_tv_encoder, tv_format );
	}

	Radeon_CalcMonitorRouting( ai, head, &values );
	
	// we don't use pixel clock anymore, so it can be reset to Be's kHz
	mode->timing.pixel_clock /= 1000;
	
	// write values to registers
	// we first switch off all output, so the monitor(s) won't get invalid signals
	Radeon_SetDPMS( ai, head, B_DPMS_SUSPEND );
	
	Radeon_InitCommonRegs( ai );
		
	Radeon_ProgramCRTCRegisters( ai, head, &values );

	OUTREG( regs, RADEON_SURFACE_CNTL, values.surface_cntl );

	if( !head->is_crtc2 )
		Radeon_ProgramRMXRegisters( ai, &values );

	if( (disp_devices & (dd_lvds | dd_dvi | dd_dvi_ext)) != 0 )
		Radeon_ProgramFPRegisters( ai, head, fp_info, &values );

	//if( mode->timing.pixel_clock )
	Radeon_ProgramPLL( ai, head, &values );
		
	if( (disp_devices & (dd_ctv | dd_stv)) != 0 )
		Radeon_ProgramTVRegisters( ai, &values, internal_tv_encoder );
		
	Radeon_ProgramMonitorRouting( ai, head, &values );
	
	head->active_displays = disp_devices;
	
	// programming is over, so hardware can be used again
	RELEASE_BEN( si->cp.lock );
	
	// well done - switch display(s) on
	Radeon_SetDPMS( ai, head, B_DPMS_ON );
	
	// overlay must be setup again after modeswitch (whoever was using it)
	// TBD: this won't work if another virtual card was using it,
	// but currently, virtual cards don't work anyway...
	si->active_overlay.head = -1;
}


// enable or disable VBlank interrupts
void Radeon_EnableIRQ( accelerator_info *ai, bool enable )
{
	shared_info *si = ai->si;
	uint32 int_cntl, int_mask;
	
	int_cntl = INREG( ai->regs, RADEON_GEN_INT_CNTL );
	int_mask = 
		RADEON_CRTC_VBLANK_MASK
		| (si->num_heads > 1 ? RADEON_CRTC2_VBLANK_MASK : 0);
		
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
status_t SET_DISPLAY_MODE( display_mode *mode_in ) 
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
	if( memcmp( &mode, &vc->mode, sizeof( display_mode )) == 0 ) {
		RELEASE_BEN( si->engine.lock );
		return B_OK;
	}
	
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
	vc->independant_heads = Radeon_NeedsSecondPort( &mode ) ? 2 : 1;
	vc->different_heads = Radeon_DifferentPorts( &mode );
	SHOW_FLOW( 2, "independant heads: %d", vc->independant_heads );
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
	Radeon_InitMultiModeVars( vc, &mode );
	
	// GO!	
	Radeon_SetMode( ai, &si->heads[vc->heads[0].physical_head], &mode );
	
	if( vc->independant_heads > 1 )
		Radeon_SetMode( ai, &si->heads[vc->heads[1].physical_head], &mode );
   	
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
	Radeon_InitPalette( ai, &si->heads[vc->heads[0].physical_head] );
	if( vc->independant_heads > 1 )
		Radeon_InitPalette( ai, &si->heads[vc->heads[1].physical_head] );
	
	// initialize cursor data
	Radeon_SetCursorColors( ai, &si->heads[vc->heads[0].physical_head] );
	if( vc->independant_heads > 1 )
		Radeon_SetCursorColors( ai, &si->heads[vc->heads[1].physical_head] );
		
	// sync should be settled now, so we can reenable IRQs
	// TBD: IRQ handling doesn't work correctly and doesn't make sense with two 
	// displays connected, so let's leave them disabled for now
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
