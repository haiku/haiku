/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Multi-monitor management
*/

#include "radeon_accelerant.h"
#include "generic.h"
#include "GlobalData.h"


// transform official mode to internal, multi-screen mode enhanced mode
void Radeon_DetectMultiMode( virtual_card *vc, display_mode *mode )
{
//	uint32 x, y, offset;
	
	mode->timing.flags &= ~RADEON_MODE_MASK;
	
	switch( vc->wanted_multi_mode ) {
	case mm_mirror:
		mode->timing.flags |= RADEON_MODE_MIRROR;
		break;
	case mm_clone:
		mode->timing.flags |= RADEON_MODE_CLONE;
		break;
	case mm_combine:
		mode->timing.flags |= RADEON_MODE_COMBINE;
		break;
	case mm_none:
	default:
	}
	
	// swap displays if asked for
	if( vc->swapDisplays )
		mode->timing.flags |= RADEON_MODE_DISPLAYS_SWAPPED;
	else
		mode->timing.flags &= ~RADEON_MODE_DISPLAYS_SWAPPED;

	// combine mode is used if virtual area is twice as visible area 
	// and if scrolling is enabled; if combining is impossible, use
	// cloning instead
	if( (mode->flags & B_SCROLL) == 0 ) {
		if( (mode->timing.flags & RADEON_MODE_MASK) == RADEON_MODE_COMBINE ) {
			SHOW_FLOW0( 3, "This isn't a combine mode, falling back to clone" );
			mode->timing.flags &= ~RADEON_MODE_MASK;
			mode->timing.flags |= RADEON_MODE_CLONE;
		}
		return;
	}
	
	SHOW_FLOW0( 3, "possibly combine mode" );

	// remove scroll flag - we don't need it anymore
	mode->flags &= ~B_SCROLL;
		
	mode->timing.flags &= ~RADEON_MODE_POSITION_MASK;

	if( mode->virtual_width == 2 * mode->timing.h_display ) {
		SHOW_FLOW0( 3, "horizontal combine mode" );
		mode->timing.flags |= RADEON_MODE_POSITION_HORIZONTAL;
		mode->timing.flags &= ~RADEON_MODE_MASK;
		mode->timing.flags |= RADEON_MODE_COMBINE;
	} else if( mode->virtual_height == 2 * mode->timing.v_display ) {
		SHOW_FLOW0( 3, "vertical combine mode" );
		mode->timing.flags |= RADEON_MODE_POSITION_VERTICAL;
		mode->timing.flags &= ~RADEON_MODE_MASK;
		mode->timing.flags |= RADEON_MODE_COMBINE;
	} else {
		// ups, this isn't really a combine mode - restore flags
		SHOW_FLOW0( 3, "wasn't really a combine mode" );
		mode->timing.flags &= ~RADEON_MODE_MASK;
		mode->timing.flags |= RADEON_MODE_CLONE;
		mode->flags |= ~B_SCROLL;
	}
}

// make sure selected multi-screen mode is valid; adapt it if needed
void Radeon_VerifyMultiMode( virtual_card *vc, shared_info *si, display_mode *mode )
{
	// if there is no second port or no second monitor connected,
	// fall back to standard mode
	if( vc->num_ports == 1 || 
		(si->ports[vc->ports[0].physical_port].disp_type == dt_none ||
		 si->ports[vc->ports[1].physical_port].disp_type == dt_none) )
	{
		SHOW_FLOW0( 3, "only one monitor - disabling any multi-mon mode" );
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
	// restore flags for combine mode
	if( (mode->timing.flags & RADEON_MODE_MASK) == RADEON_MODE_COMBINE )
		mode->flags |= B_SCROLL;
}


// initialize multi-screen mode dependant variables
void Radeon_InitMultiModeVars( virtual_card *vc, display_mode *mode )
{
//	uint32 offset;
	uint32 x, y;

	// setup single-screen mode
	vc->eff_width = mode->timing.h_display;
	vc->eff_height = mode->timing.v_display;
		
	vc->ports[0].rel_x = 0;
	vc->ports[0].rel_y = 0;
	
	switch( mode->timing.flags & RADEON_MODE_MASK ) {
	case RADEON_MODE_CLONE:
		// in clone mode, ports are independant but show the same
		vc->ports[1].rel_x = 0;
		vc->ports[1].rel_y = 0;
		break;
		
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
		
		vc->ports[1].rel_x = 0;
		vc->ports[1].rel_y = 0;

		// set relative offset
		if( (mode->timing.flags & RADEON_MODE_DISPLAYS_SWAPPED) == 0 ) {
			vc->ports[1].rel_x = x;
			vc->ports[1].rel_y = y;
		} else {
			vc->ports[0].rel_x = x;
			vc->ports[0].rel_y = y;
		}
		break;
		
	case RADEON_MODE_STANDARD:
	case RADEON_MODE_MIRROR:
		break;
	}
}


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
	
	switch( mode->h_display_start ) {
	case ms_swap:
		if( mode->v_display_start != 0 )
			vc->swapDisplays = mode->timing.flags != 0;
		else
			mode->timing.flags = vc->swapDisplays;
			
		// write settings instantly
		Radeon_WriteSettings( vc );
		return B_OK;
		
/*	case ms_overlay_port:
		if( mode->v_display_start != 0 )
			vc->whished_overlay_port = mode->timing.flags;
		else
			mode->timing.flags = vc->whished_overlay_port;
			
		Radeon_WriteSettings( vc );
		return B_OK;*/
		
	default:
		return B_BAD_INDEX;
	}
}


// return true if both ports must be programmed
bool Radeon_NeedsSecondPort( display_mode *mode )
{
	switch( mode->timing.flags & RADEON_MODE_MASK ) {
	case RADEON_MODE_COMBINE:
	case RADEON_MODE_CLONE:
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
