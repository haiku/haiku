/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Public mode-specific info functions
*/


#include "radeon_accelerant.h"
#include "GlobalData.h"
#include "generic.h"
#include <sys/ioctl.h>
#include <GraphicsDefs.h>


// public function: return current display mode
status_t GET_DISPLAY_MODE( display_mode *current_mode )
{
	virtual_card *vc = ai->vc;
	
	// TBD: there is a race condition if someone else is just setting it
	//      we won't lock up but return non-sense

	*current_mode = vc->mode;
	
	// we hide multi-monitor-mode because :-
	// - we want to look like an ordinary single-screen driver
	// - the multi-mode is already adapted to current screen configuration,
	//   and the mode should be configuration-independant
	Radeon_HideMultiMode( vc, current_mode );

	return B_OK;
}

// public function: return configuration of frame buffer
status_t GET_FRAME_BUFFER_CONFIG( frame_buffer_config *afb )
{
	virtual_card *vc = ai->vc;

	// TBD: race condition again
	
	// easy again, as the last mode set stored the info in a convienient form
	*afb = vc->fbc;
	return B_OK;
}

// public function: return clock limits for given display mode
status_t GET_PIXEL_CLOCK_LIMITS(display_mode *dm, uint32 *low, uint32 *high) 
{
	// we ignore stuff like DVI/LCD restrictions - 
	// they are handled	automatically on set_display_mode
	uint32 total_pix = (uint32)dm->timing.h_total * (uint32)dm->timing.v_total;
	uint32 clock_limit = ai->si->pll.max_pll_freq * 10;

	/* lower limit of about 48Hz vertical refresh */
	*low = (total_pix * 48L) / 1000L;
	if (*low > clock_limit) 
		return B_ERROR;

	*high = clock_limit;
	return B_OK;
}

/*
	Return the semaphore id that will be used to signal a vertical retrace
	occured.
*/
sem_id ACCELERANT_RETRACE_SEMAPHORE(void) 
{
//	virtual_card *vc = ai->vc;

	/*
	NOTE:
		The kernel driver created this for us.  We don't know if the system is
		using real interrupts, or if we're faking it, and we don't care.
		If we choose not to support this at all, we'd just return B_ERROR here,
		and the user wouldn't get any kind of vertical retrace support.
	*/
	// with multi-monitor mode, we have two vertical blanks!
	// until we find a better solution, we always return virtual port 0,
	// which may be either physical port 0 or 1
//	int physical_port = vc->ports[0].physical_port;
	
	//SHOW_INFO( 3, "semaphore: %x", ai->si->ports[physical_port].vblank );
	
	//return ai->si->ports[physical_port].vblank;
	return 0;
}
