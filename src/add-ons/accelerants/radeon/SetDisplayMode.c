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

void Radeon_SetMode( accelerator_info *ai, virtual_port *port, display_mode *mode );
void Radeon_EnableIRQ( accelerator_info *ai, bool enable );


// Radeon's DACs share same public registers, this function
// selects the DAC you'll talk to
static void selectDAC( accelerator_info *ai, virtual_port *port )
{
	Radeon_WriteRegCP( ai, RADEON_DAC_CNTL2, 
		(port->is_crtc2 ? RADEON_DAC2_PALETTE_ACC_CTL : 0) |
		(ai->si->dac_cntl2 & ~RADEON_DAC2_PALETTE_ACC_CTL) );
}


// set standard colour palette (needed for non-palette modes)
static void initDAC( accelerator_info *ai, virtual_port *port )
{
	int i;
	
	selectDAC( ai, port );
	
	Radeon_WriteRegCP( ai, RADEON_PALETTE_INDEX, 0 );
	
	for( i = 0; i < 256; ++i )
		Radeon_WriteRegCP( ai, RADEON_PALETTE_DATA, (i << 16) | (i << 8) | i );
}



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
	{ RADEON_GEN_INT_CNTL, 0 },
	{ RADEON_CAP0_TRIG_CNTL, 0 },
};

static void Radeon_InitCommonRegs( accelerator_info *ai )
{
	vuint8 *regs = ai->regs;
	uint i;
	
	for( i = 0; i < sizeof( common_regs) / sizeof( common_regs[0] ); ++i )
		OUTREG( regs, common_regs[i].reg, common_regs[i].val );
}

// set display mode of one port;
// port restrictions, like fixed-sync TFTs connected to it, are taken care of
void Radeon_SetMode( accelerator_info *ai, virtual_port *port, display_mode *mode )
{
	virtual_card *vc = ai->vc;
	shared_info *si = ai->si;
	vuint8 *regs = ai->regs;
	int    format;
	int    bpp;
	display_type_e disp_type;
	port_regs values;
	
	port->mode = *mode;
	
	// don't destroy passed values, use our copy instead
	mode = &port->mode;
	
	disp_type = si->ports[port->physical_port].disp_type;
	
	// if using an flat panel or LCD, maximum resolution
	// is determined by the physical resolution; 
	// also, all timing is fixed
	if( disp_type == dt_dvi_1 || disp_type == dt_lvds ) {
		fp_info *fp_info = &si->fp_port;
		
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
	
	Radeon_GetFormat( mode->space, &format, &bpp );
	
	vc->bpp = bpp;
	vc->datatype = format;    

	// calculate all hardware register values
	Radeon_CalcCRTCRegisters( ai, port, mode, &values );

	values.surface_cntl = RADEON_SURF_TRANSLATION_DIS;

	// for flat panels, we may not have pixel clock if DDC data is missing;
	// as we don't change effective resolution we can leave it as set by BIOS
	if( mode->timing.pixel_clock )
		Radeon_CalcPLLDividers( &si->pll, mode->timing.pixel_clock / 10, &values );
	
	if( disp_type == dt_dvi_1 || disp_type == dt_lvds )
		Radeon_CalcFPRegisters( ai, port, &si->fp_port, mode, &values );

	
	// write values to registers
	Radeon_SetDPMS( ai, port, B_DPMS_SUSPEND );
	
	Radeon_InitCommonRegs( ai );
		
	Radeon_ProgramCRTCRegisters( ai, port, &values );

	OUTREG( regs, RADEON_SURFACE_CNTL, values.surface_cntl );

	if( disp_type == dt_dvi_1 || disp_type == dt_lvds ) 
		Radeon_ProgramFPRegisters( ai, &si->fp_port, &values );


	if( mode->timing.pixel_clock )
		Radeon_ProgramPLL( ai, port, &values );
	
	Radeon_SetDPMS( ai, port, B_DPMS_ON );
	
	// overlay must be setup again after modeswitch (whoever was using it)
	// TBD: this won't work if another virtual card was using it,
	// but currently, virtual cards don't work anyway...
	si->active_overlay.port = -1;
}


// enable or disable VBlank interrupts
void Radeon_EnableIRQ( accelerator_info *ai, bool enable )
{
	shared_info *si = ai->si;
	uint32 int_cntl, int_mask;
	
	int_cntl = INREG( ai->regs, RADEON_GEN_INT_CNTL );
	int_mask = 
		RADEON_CRTC_VBLANK_MASK
		| (si->has_crtc2 ? RADEON_CRTC2_VBLANK_MASK : 0);
		
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
	// do this before equality check to recognize changed to multi-monitor mode 
	Radeon_DetectMultiMode( vc, &mode );
	
	// mode switches can take quite long and are visible, 
	// so avoid them if possible
	if( memcmp( &mode, &vc->mode, sizeof( display_mode )) == 0 ) {
		RELEASE_BEN( si->engine.lock );
		return B_OK;
	}

	// make sure, we don't get disturbed
	Radeon_Finish( ai );
	Radeon_EnableIRQ( ai, false );

	// free cursor and framebuffer memory
	{
		radeon_free_local_mem fm;
		
		fm.magic = RADEON_PRIVATE_DATA_MAGIC;
	
		if( vc->cursor.mem_handle ) {
			fm.handle = vc->cursor.mem_handle;
			ioctl( ai->fd, RADEON_FREE_LOCAL_MEM, &fm );
		}
		
		if( vc->fb_mem_handle ) {
			fm.handle = vc->fb_mem_handle;
			ioctl( ai->fd, RADEON_FREE_LOCAL_MEM, &fm );
		}
	}
	
	memcpy( &vc->mode, &mode, sizeof( display_mode ));
	
	// verify hardware restrictions *after* saving mode
	// e.g. if you want a span mode but have one monitor disconnected,
	// configuration shouldn't be touched, so you can continue working
	// with two monitors later on just like nothing has happened
	Radeon_VerifyMultiMode( vc, si, &mode );

	// set main flags	
	vc->independant_ports = Radeon_NeedsSecondPort( &mode ) ? 2 : 1;
	vc->different_ports = Radeon_DifferentPorts( &mode );
	SHOW_FLOW( 2, "independant ports: %d", vc->independant_ports );
	vc->scroll = mode.flags & B_SCROLL;
	SHOW_FLOW( 2, "scrolling %s", vc->scroll ? "enabled" : "disabled" );

	// allocate frame buffer and cursor image memory
	{
		radeon_alloc_local_mem am;
		int format, bpp;
		
		// alloc cursor memory		
		am.magic = RADEON_PRIVATE_DATA_MAGIC;
		am.size = 1024;
		
		if( ioctl( ai->fd, RADEON_ALLOC_LOCAL_MEM, &am ) == B_OK ) {
			vc->cursor.mem_handle = am.handle;
			vc->cursor.fb_offset = am.fb_offset;
		} else {
			// too bad that we are out of mem -> set reasonable values as
			// it's too late to give up (ouch!)
			SHOW_ERROR0( 2, "no memory for cursor image!" );
			vc->cursor.mem_handle = 0;
			vc->cursor.fb_offset = 0;
		}
		
		vc->cursor.data = si->framebuffer + vc->cursor.fb_offset;
	
		// alloc frame buffer
		Radeon_GetFormat( mode.space, &format, &bpp );
		vc->pitch = Radeon_RoundVWidth( mode.virtual_width, bpp ) * bpp;
		am.size = vc->pitch * mode.virtual_height;
		
		if( ioctl( ai->fd, RADEON_ALLOC_LOCAL_MEM, &am ) == B_OK ) {
			vc->fb_mem_handle = am.handle;
			vc->fb_offset = am.fb_offset;
		} else 	{
			// ouch again - set reasonable values
			SHOW_ERROR0( 2, "no memory for frame buffer!" );
			vc->fb_mem_handle = 0;
			vc->fb_offset = 1024;
		}
		
		vc->fbc.frame_buffer = si->framebuffer + vc->fb_offset;
		vc->fbc.frame_buffer_dma = (void *)((uint8 *)si->framebuffer_pci + vc->fb_offset);
		vc->fbc.bytes_per_row = vc->pitch;
	}
	
	// multi-screen stuff
	Radeon_InitMultiModeVars( vc, &mode );
	
	// GO!	
	Radeon_SetMode( ai, &vc->ports[0], &mode );
	
	if( vc->independant_ports > 1 )
		Radeon_SetMode( ai, &vc->ports[1], &mode );
   	
   	SHOW_FLOW( 3, "pitch=%ld", vc->pitch );
   	
   	// we'll modify bits of this reg, so save it for async access
	si->dac_cntl2 = INREG( ai->regs, RADEON_DAC_CNTL2 );
	
	// init accelerator
	Radeon_Init2D( ai, vc->datatype );
	
	// remember that 2D accelerator is not prepared for any virtual card
	si->active_vc = -1;
	
	Radeon_ActivateVirtualCard( ai );

	// first move to well-defined position (to setup CRTC offset)
	Radeon_MoveDisplay( ai, 0, 0 );		
	// then to (probably faulty) user-defined pos
	Radeon_MoveDisplay( ai, mode.h_display_start, mode.v_display_start );

	// set standard palette in direct-colour modes
	initDAC( ai, &vc->ports[0] );
	if( vc->independant_ports > 1 )
		initDAC( ai, &vc->ports[1] );
	
	// initialize cursor data
	Radeon_SetCursorColors( ai, &vc->ports[0] );
	if( vc->independant_ports > 1 )
		Radeon_SetCursorColors( ai, &vc->ports[1] );
		
	// sync should be settled now, so we can reenable IRQs
	// TBD: IRQ handling doesn't work correctly and doesn't make sense with two 
	// displays connected, so let's leave them disabled for now
	//Radeon_EnableIRQ( ai, true );
		
	RELEASE_BEN( si->engine.lock );

	// !! all this must be done after lock has been 
	//    released to avoid dead-lock !!
	// TBD: any invalid intermediate states?

	// move_cursor sets all cursor-related variables and registers
	vc->cursor.is_visible = false;
	MOVE_CURSOR( 0, 0 );

	return B_OK;
}

// update shown are of one port
static void moveOneDisplay( accelerator_info *ai, virtual_port *port )
{
	virtual_card *vc = ai->vc;
	uint32 offset;
	
	offset = (vc->mode.v_display_start + port->rel_y) * vc->pitch + 
		(vc->mode.h_display_start + port->rel_x) * vc->bpp + 
		vc->fb_offset;
	
	SHOW_FLOW( 3, "Setting address %x on port %d", 
		offset,	port->is_crtc2 );
	
	Radeon_WaitForFifo( ai, 1 );

	OUTREG( ai->regs, port->is_crtc2 ? RADEON_CRTC2_OFFSET : RADEON_CRTC_OFFSET, offset );
/*	Radeon_WriteRegCP( ai, port->is_crtc2 ? RADEON_CRTC2_OFFSET : RADEON_CRTC_OFFSET, 
		offset );*/
}

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
	moveOneDisplay( ai, &vc->ports[0] );

	if( vc->independant_ports > 1 )
		moveOneDisplay( ai, &vc->ports[1] );
		
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

static void setPalette( accelerator_info *ai, virtual_port *port, 
	uint count, uint8 first, uint8 *color_data );

// public function: set colour palette
void SET_INDEXED_COLORS(uint count, uint8 first, uint8 *color_data, uint32 flags) 
{
	virtual_card *vc = ai->vc;
	shared_info *si = ai->si;
//	uint i;

	SHOW_FLOW( 3, "first=%d, count=%d", first, flags );
	
	if( vc->mode.space != B_CMAP8 ) {
		SHOW_ERROR0( 2, "Tried to set palette in non-palette mode" );
		return;
	} 

	// we need to lock card, though this isn't done	in sample driver
	ACQUIRE_BEN( si->engine.lock );

	setPalette( ai, &vc->ports[0], count, first, color_data );
	
	if( vc->independant_ports > 1 )
		setPalette( ai, &vc->ports[1], count, first, color_data );
		
	RELEASE_BEN( si->engine.lock );
}


// set palette of one DAC
static void setPalette( accelerator_info *ai, virtual_port *port, 
	uint count, uint8 first, uint8 *color_data )
{
	uint i;
	
	selectDAC( ai, port );
	
	Radeon_WriteRegCP( ai, RADEON_PALETTE_INDEX, first );
	
	for( i = 0; i < count; ++i, color_data += 3 )
		Radeon_WriteRegCP( ai, RADEON_PALETTE_DATA, 
			((uint32)color_data[0] << 16) | 
			((uint32)color_data[1] << 8) | 
			 color_data[2] );
}
