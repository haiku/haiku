/*
 * Copyright (c) 2002, Thomas Kurschel
 * Distributed under the terms of the MIT License.
 */

/*!	Public mode-specific info functions */


#include "radeon_accelerant.h"
#include "GlobalData.h"
#include "generic.h"

#include <GraphicsDefs.h>


status_t
GET_DISPLAY_MODE(display_mode *mode)
{
	virtual_card *vc = ai->vc;

	// TBD: there is a race condition if someone else is just setting it
	//      we won't lock up but return non-sense

	*mode = vc->mode;

	// we hide multi-monitor-mode because :-
	// - we want to look like an ordinary single-screen driver
	// - the multi-mode is already adapted to current screen configuration,
	//   and the mode should be configuration-independant
	Radeon_HideMultiMode(vc, mode);
	return B_OK;
}


status_t
GET_FRAME_BUFFER_CONFIG(frame_buffer_config *afb)
{
	virtual_card *vc = ai->vc;

	// TBD: race condition again

	// easy again, as the last mode set stored the info in a convienient form
	*afb = vc->fbc;
	return B_OK;
}


status_t
GET_PIXEL_CLOCK_LIMITS(display_mode *dm, uint32 *low, uint32 *high)
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


#ifdef __HAIKU__

status_t
radeon_get_edid_info(void* info, size_t size, uint32* _version)
{
	disp_entity* routes = &ai->si->routing;
	int32 index;

	if (size < sizeof(struct edid1_info))
		return B_BUFFER_OVERFLOW;

	if (routes->port_info[0].edid_valid)
		index = 0;
	else if (routes->port_info[1].edid_valid)
		index = 1;
	else
		return B_ERROR;

	memcpy(info, &routes->port_info[index].edid, sizeof(struct edid1_info));
	*_version = EDID_VERSION_1;
	return B_OK;
}

#endif	// __HAIKU__

/*
	Return the semaphore id that will be used to signal a vertical retrace
	occured.
*/
sem_id
ACCELERANT_RETRACE_SEMAPHORE(void) 
{
	virtual_card *vc = ai->vc;

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
	int crtcIndex;

	if( vc->used_crtc[0] )
		crtcIndex = 0;
	else
		crtcIndex = 1;

	//SHOW_INFO( 3, "semaphore: %x", ai->si->ports[physical_port].vblank );

	return ai->si->crtc[crtcIndex].vblank;
}
