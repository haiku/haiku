/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Display Power Management (DPMS) support
*/

#include "radeon_accelerant.h"
#include "mmio.h"
#include "crtc_regs.h"
#include "fp_regs.h"
#include "GlobalData.h"

// these static functions are moved to end of file to 
// make sure gcc doesn't inline them - we prefer size and not
// speed for this file
static status_t Radeon_SetDPMS_CRTC1( accelerator_info *di, int mode );
static status_t Radeon_SetDPMS_CRTC2( accelerator_info *di, int mode );
static uint32 Radeon_GetDPMS_CRTC1( accelerator_info *di );
static uint32 Radeon_GetDPMS_CRTC2( accelerator_info *di );

status_t SET_DPMS_MODE(uint32 dpms_flags);
uint32 DPMS_CAPABILITIES(void);
uint32 DPMS_MODE(void);


// public function: set DPMS mode
status_t SET_DPMS_MODE(uint32 dpms_flags) 
{
	virtual_card *vc = ai->vc;
	status_t result1, result2;
	
	result1 = Radeon_SetDPMS( ai, &vc->ports[0], dpms_flags );
	
	if( vc->independant_ports > 1 )
		result2 = Radeon_SetDPMS( ai, &vc->ports[1], dpms_flags );
	else
		result2 = B_OK;

	if( result1 == B_OK && result2 == B_OK )
		return B_OK;
	else
		return B_ERROR;
}

// public function: report DPMS capabilities
uint32 DPMS_CAPABILITIES(void) 
{
	return 	B_DPMS_ON | B_DPMS_STAND_BY  | B_DPMS_SUSPEND | B_DPMS_OFF;
}


// public function: get current DPMS mode
uint32 DPMS_MODE(void) 
{
	// we just ask the primary port what status it is in
	return Radeon_GetDPMS( ai, &ai->vc->ports[0] );
}


// set DPMS mode of one port
status_t Radeon_SetDPMS( accelerator_info *ai, virtual_port *port, int mode )
{
	// if we have a laptop panel
	// and we have a second screen connected
	// and they both show the same content, 
	// then switch the laptop display always off
	if( ai->si->ports[port->physical_port].disp_type == dt_lvds &&
		ai->vc->independant_ports > 1 && 
		ai->vc->different_ports == 1 )
	{
		mode = B_DPMS_OFF;
	}

	if( port->is_crtc2 )
		return Radeon_SetDPMS_CRTC2( ai, mode );
	else
		return Radeon_SetDPMS_CRTC1( ai, mode );
}


// get DPMS mode of one port
uint32 Radeon_GetDPMS( accelerator_info *ai, virtual_port *port )
{
	if( port->is_crtc2 )
		return Radeon_GetDPMS_CRTC2( ai );
	else
		return Radeon_GetDPMS_CRTC1( ai );
}


// set DPMS mode for first port
status_t Radeon_SetDPMS_CRTC1( accelerator_info *ai, int mode )
{
	vuint8 *regs = ai->regs;
	shared_info *si = ai->si;
	
	int mask = RADEON_CRTC_DISPLAY_DIS
		| RADEON_CRTC_HSYNC_DIS
		| RADEON_CRTC_VSYNC_DIS;
		
	switch( mode ) {
	case B_DPMS_ON:
		/* Screen: On; HSync: On, VSync: On */
		OUTREGP( regs, RADEON_CRTC_EXT_CNTL, 0, ~mask );
		break;
	case B_DPMS_STAND_BY:
		/* Screen: Off; HSync: Off, VSync: On */
		OUTREGP( regs, RADEON_CRTC_EXT_CNTL,
			RADEON_CRTC_DISPLAY_DIS | RADEON_CRTC_HSYNC_DIS, ~mask );
		break;
	case B_DPMS_SUSPEND:
		/* Screen: Off; HSync: On, VSync: Off */
		OUTREGP( regs, RADEON_CRTC_EXT_CNTL,
			RADEON_CRTC_DISPLAY_DIS | RADEON_CRTC_VSYNC_DIS, ~mask );
		break;
	case B_DPMS_OFF:
		/* Screen: Off; HSync: Off, VSync: Off */
		OUTREGP( regs, RADEON_CRTC_EXT_CNTL, mask, ~mask );
		break;
	default:
		return B_BAD_VALUE;
	}
	
	// if this is a flat panel, switch off backlight too
	if( si->ports[0].disp_type == dt_dvi_1 || si->ports[0].disp_type == dt_lvds ) {
		switch( mode ) {
		case B_DPMS_ON:
			// on my laptop, the display has problems to wake-up, this
			// should hopefully cure that
			// (you get a dark picture first that becomes brighter step by step,
			//  after a couple of seconds you have full brightness again)
			OUTREGP( regs, RADEON_LVDS_GEN_CNTL, RADEON_LVDS_BLON, ~RADEON_LVDS_BLON );
			//snooze( ai->si->fp_port.panel_pwr_delay * 1000 );
			OUTREGP( regs, RADEON_LVDS_GEN_CNTL, RADEON_LVDS_BLON | RADEON_LVDS_ON, 
				~(RADEON_LVDS_DISPLAY_DIS | RADEON_LVDS_BLON | RADEON_LVDS_ON) );
			break;
		case B_DPMS_STAND_BY:
		case B_DPMS_SUSPEND:
		case B_DPMS_OFF:
			OUTREGP( regs, RADEON_LVDS_GEN_CNTL, RADEON_LVDS_DISPLAY_DIS,
				~(RADEON_LVDS_DISPLAY_DIS | RADEON_LVDS_BLON | RADEON_LVDS_ON) );
			break;
		}
	}

	// it seems that DPMS doesn't work on DVI, so we disable FP completely
	// (according to specs this is the official way to handle DVI though DPMS
	// *should* be supported as well)
	if( si->ports[0].disp_type == dt_dvi_1 ) {
		switch( mode ) {
		case B_DPMS_ON:
			OUTREGP( regs, RADEON_FP_GEN_CNTL, RADEON_FP_FPON, ~RADEON_FP_FPON );
			break;
		case B_DPMS_STAND_BY:
		case B_DPMS_SUSPEND:
		case B_DPMS_OFF:
			OUTREGP( regs, RADEON_FP_GEN_CNTL, 0, ~RADEON_FP_FPON );
			break;
		}
	}
	
	return B_OK;
}


// set DPMS mode of second port
status_t Radeon_SetDPMS_CRTC2( accelerator_info *di, int mode )
{
	vuint8 *regs = di->regs;
	
	int mask = RADEON_CRTC2_DISP_DIS
		| RADEON_CRTC2_HSYNC_DIS
		| RADEON_CRTC2_VSYNC_DIS;
		
	switch( mode ) {
	case B_DPMS_ON:
		/* Screen: On; HSync: On, VSync: On */
		OUTREGP( regs, RADEON_CRTC2_GEN_CNTL, 0, ~mask );
		break;
	case B_DPMS_STAND_BY:
		/* Screen: Off; HSync: Off, VSync: On */
		OUTREGP( regs, RADEON_CRTC2_GEN_CNTL,
			RADEON_CRTC2_DISP_DIS | RADEON_CRTC2_HSYNC_DIS, ~mask );
		break;
	case B_DPMS_SUSPEND:
		/* Screen: Off; HSync: On, VSync: Off */
		OUTREGP( regs, RADEON_CRTC2_GEN_CNTL,
			RADEON_CRTC2_DISP_DIS | RADEON_CRTC2_VSYNC_DIS, ~mask );
		break;
	case B_DPMS_OFF:
		/* Screen: Off; HSync: Off, VSync: Off */
		OUTREGP( regs, RADEON_CRTC2_GEN_CNTL, mask, ~mask );
		break;
	default:
		return B_BAD_VALUE;
	}
	
	return B_OK;
}


// get DPMS mode of first port
uint32 Radeon_GetDPMS_CRTC1( accelerator_info *di )
{
	uint32 tmp;
	
	tmp = INREG( di->regs, RADEON_CRTC_EXT_CNTL );
	
	if( (tmp & RADEON_CRTC_DISPLAY_DIS) == 0 )
		return B_DPMS_ON;
		
	if( (tmp & RADEON_CRTC_VSYNC_DIS) == 0 )
		return B_DPMS_STAND_BY;
		
	if( (tmp & RADEON_CRTC_HSYNC_DIS) == 0 )
		return B_DPMS_SUSPEND;
		
	return B_DPMS_OFF;
}


// get DPMS mode of second port
uint32 Radeon_GetDPMS_CRTC2( accelerator_info *di )
{
	uint32 tmp;
	
	tmp = INREG( di->regs, RADEON_CRTC2_GEN_CNTL );
	
	if( (tmp & RADEON_CRTC2_DISP_DIS) == 0 )
		return B_DPMS_ON;
		
	if( (tmp & RADEON_CRTC2_VSYNC_DIS) == 0 )
		return B_DPMS_STAND_BY;
		
	if( (tmp & RADEON_CRTC2_HSYNC_DIS) == 0 )
		return B_DPMS_SUSPEND;
		
	return B_DPMS_OFF;
}
