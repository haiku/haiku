/*
	Copyright (c) 2002-2004, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Multi-monitor management
*/

#include "radeon_accelerant.h"
#include "generic.h"
#include "GlobalData.h"


// transform official mode to internal, multi-screen mode enhanced mode
void Radeon_DetectMultiMode( virtual_card *vc, display_mode *mode )
{
	(void)vc;
	
	mode->timing.flags &= ~RADEON_MODE_MASK;
	
	// combine mode is used if virtual area is twice as visible area 
	// and if scrolling is enabled; if combining is impossible, use
	// cloning instead
	if( (mode->flags & B_SCROLL) == 0 )
		return;
	
	SHOW_FLOW0( 3, "possibly combine mode" );

	// remove scroll flag - we don't need it anymore
	mode->flags &= ~B_SCROLL;
		
	mode->timing.flags &= ~RADEON_MODE_POSITION_MASK;

	if( mode->virtual_width == 2 * mode->timing.h_display ) {
		SHOW_FLOW0( 2, "horizontal combine mode" );
		mode->timing.flags |= RADEON_MODE_POSITION_HORIZONTAL;
		mode->timing.flags &= ~RADEON_MODE_MASK;
		mode->timing.flags |= RADEON_MODE_COMBINE;
	} else if( mode->virtual_height == 2 * mode->timing.v_display ) {
		SHOW_FLOW0( 2, "vertical combine mode" );
		mode->timing.flags |= RADEON_MODE_POSITION_VERTICAL;
		mode->timing.flags &= ~RADEON_MODE_MASK;
		mode->timing.flags |= RADEON_MODE_COMBINE;
	} else {
		// ups, this isn't really a combine mode - restore flags
		SHOW_FLOW0( 2, "wasn't really a combine mode" );
		mode->timing.flags &= ~RADEON_MODE_MASK;
		mode->flags |= B_SCROLL;
	}
}

// make sure selected multi-screen mode is valid; adapt it if needed
void Radeon_VerifyMultiMode( virtual_card *vc, shared_info *si, display_mode *mode )
{
	// if there is no second port or no second monitor connected,
	// fall back to standard mode
	int num_usable_crtcs = vc->assigned_crtc[0] && si->crtc[0].chosen_displays != dd_none;
	
	if( si->num_crtc > 1 )
		num_usable_crtcs += vc->assigned_crtc[1] && si->crtc[1].chosen_displays != dd_none;
		
	if( num_usable_crtcs < 2 ) {
		SHOW_FLOW0( 2, "only one monitor - disabling any multi-mon mode" );
		// restore flags if combine mode is selected
		if( (mode->timing.flags & RADEON_MODE_MASK) == RADEON_MODE_COMBINE )
			mode->flags |= B_SCROLL;

		mode->timing.flags &= ~RADEON_MODE_MASK;
		mode->timing.flags |= RADEON_MODE_STANDARD;
	}
}

// transform internal, multi-screen enabled display mode
// to official mode
void Radeon_HideMultiMode( virtual_card *vc, display_mode *mode )
{
	(void) vc;
	
	// restore flags for combine mode
	if( (mode->timing.flags & RADEON_MODE_MASK) == RADEON_MODE_COMBINE )
		mode->flags |= B_SCROLL;
}


// initialize multi-screen mode dependant variables
void Radeon_InitMultiModeVars( 
	accelerator_info *ai, display_mode *mode )
{
	virtual_card *vc = ai->vc;
	shared_info *si = ai->si;
	uint32 x, y;

	// setup single-screen mode
	vc->eff_width = mode->timing.h_display;
	vc->eff_height = mode->timing.v_display;
	
	if( vc->used_crtc[0] ) {
		si->crtc[0].rel_x = 0;
		si->crtc[0].rel_y = 0;
	} 
	
	if( vc->used_crtc[1] ) {
		si->crtc[1].rel_x = 0;
		si->crtc[1].rel_y = 0;
	}
	
	switch( mode->timing.flags & RADEON_MODE_MASK ) {
	case RADEON_MODE_COMBINE:
		// detect where second screen must be located and
		// adapt total visible area accordingly
		if( (mode->timing.flags & RADEON_MODE_POSITION_MASK) == RADEON_MODE_POSITION_HORIZONTAL ) {
			vc->eff_width = 2 * mode->timing.h_display;
			x = mode->timing.h_display;
			y = 0;
		} else {
			vc->eff_height = 2 * mode->timing.v_display;
			x = 0;
			y = mode->timing.v_display;
		}
		
		SHOW_FLOW( 3, "relative position of second screen: %d, %d", x, y );
		
		// set relative offset
		if( !vc->swap_displays ) {
			si->crtc[1].rel_x = x;
			si->crtc[1].rel_y = y;
		} else {
			si->crtc[0].rel_x = x;
			si->crtc[0].rel_y = y;
		}
		break;
		
	default:
		// else, ports are independant but show the same
		break;
	}
}


// mapping of internal TV standard code to public TV standard code
static const uint32 private2be[] = {
	0, 1, 3, 4, 103, 3/* PAL SCART - no public id, so I use PAL BDGHI */, 102 };

// check and execute tunnel settings command
status_t Radeon_CheckMultiMonTunnel( virtual_card *vc, display_mode *mode, 
	const display_mode *low, const display_mode *high, bool *isTunneled )
{
	if( (mode->timing.flags & RADEON_MODE_MULTIMON_REQUEST) != 0 &&
		(mode->timing.flags & RADEON_MODE_MULTIMON_REPLY) == 0 )
	{
		mode->timing.flags &= ~RADEON_MODE_MULTIMON_REQUEST;
		mode->timing.flags |= RADEON_MODE_MULTIMON_REPLY;
		
		// still process request, just in case someone set this flag
		// combination by mistake
		
		// TBD: disabled to shorten syslog 
		*isTunneled = true;
		return B_OK;
	}

	// check magic params
	if( mode->space != 0 || low->space != 0 || high->space != 0 
		|| low->virtual_width != 0xffff || low->virtual_height != 0xffff
		|| high->virtual_width != 0 || high->virtual_height != 0
		|| mode->timing.pixel_clock != 0 
		|| low->timing.pixel_clock != 'TKTK' || high->timing.pixel_clock != 'KTKT' )
	{
		*isTunneled = false;
		return B_OK;
	}
	
	*isTunneled = true;
	
	/*SHOW_FLOW( 1, "tunnel access code=%d, command=%d", 
		mode->h_display_start, mode->v_display_start );*/
	
	switch( mode->h_display_start ) {
	case ms_swap:
		switch( mode->v_display_start ) {
		case 0:
			mode->timing.flags = vc->swap_displays;
			return B_OK;
			
		case 1:
			vc->swap_displays = mode->timing.flags != 0;
			vc->enforce_mode_change = true;
			// write settings instantly
			Radeon_WriteSettings( vc );
			return B_OK;
		}
		break;
		
	case ms_use_laptop_panel:
		// we must refuse this setting if there is no laptop panel;
		// else, the preferences dialog would show this (useless) option
		if( (vc->connected_displays & dd_lvds) == 0 )
			return B_ERROR;
			
		switch( mode->v_display_start ) {
		case 0:
			mode->timing.flags = vc->use_laptop_panel;
			//SHOW_FLOW( 1, "get use_laptop_panel settings (%d)", mode->timing.flags );
			return B_OK;
			
		case 1:
			vc->use_laptop_panel = mode->timing.flags != 0;
			//SHOW_FLOW( 1, "set use_laptop_panel settings (%d)", vc->use_laptop_panel );
			vc->enforce_mode_change = true;
			Radeon_WriteSettings( vc );
			return B_OK;
		}
		break;
		
	case ms_tv_standard:
		switch( mode->v_display_start ) {
		case 0:
			mode->timing.flags = private2be[vc->tv_standard];
			/*SHOW_FLOW( 1, "read tv_standard (internal %d, public %d)", 
				vc->tv_standard, mode->timing.flags );*/
			return B_OK;
			
		case 1:
			switch( mode->timing.flags ) {
			case 0: vc->tv_standard = ts_off; break;
			case 1:	vc->tv_standard = ts_ntsc; break;
			case 2: break; // ntsc j
			case 3: vc->tv_standard = ts_pal_bdghi; break;
			case 4: vc->tv_standard = ts_pal_m; break;
			case 5: break; // pal n
			case 6: break; // secam - I reckon not supported by hardware
			case 101: break; // ntsc 443
			case 102: vc->tv_standard = ts_pal_60; break;
			case 103: vc->tv_standard = ts_pal_nc; break;
			}
			
			SHOW_FLOW( 1, "set tv_standard (internal %d, public %d)", 
				vc->tv_standard, mode->timing.flags );
		
			vc->enforce_mode_change = true;	
			Radeon_WriteSettings( vc );
			return B_OK;
			
		case 2: {
			uint32 idx = mode->timing.flags;
			
			// we limit it explicetely to NTSC and PAL as all other
			// modes are not fully implemented
			if( idx < sizeof( private2be ) / sizeof( private2be[0] ) && 
				idx < 3 ) {
				mode->timing.flags = private2be[idx];
				return B_OK;
			} else
				return B_ERROR;
			}
		}
	}
	
	return B_ERROR;
}


// return true if both ports must be programmed
bool Radeon_NeedsSecondPort( display_mode *mode )
{
	switch( mode->timing.flags & RADEON_MODE_MASK ) {
	case RADEON_MODE_COMBINE:
		return true;
	default:
		return false;
	}
}


// return number of ports showing differents parts of frame buffer
bool Radeon_DifferentPorts( display_mode *mode )
{
	switch( mode->timing.flags & RADEON_MODE_MASK ) {
	case RADEON_MODE_COMBINE:
		return 2;
	default:
		return 1;
	}
}
