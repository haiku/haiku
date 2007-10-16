/*
 * Copyright (c) 2002, Thomas Kurschel
 * Distributed under the terms of the MIT License.
 */

/*!	
	Contains entry point to get public functions.
	(directly copied from sample driver)
*/

#include "generic.h"

/*

The standard entry point.  Given a uint32 feature identifier, this routine
returns a pointer to the function that implements the feature.  Some features
require more information than just the identifier to select the proper
function.  The extra information (which is specific to the feature) is
pointed at by the void *data parameter.  By default, no extra information
is available.  Any extra information available to choose the function will be
noted on a case by case below.

*/
void *
get_accelerant_hook(uint32 feature, void *data)
{
	(void)data;

	switch (feature) {
/*
These definitions are out of pure lazyness.
*/
#define HOOK(x) case B_##x: return (void *)x
#define ZERO(x) case B_##x: return (void *)0

/*
One of either B_INIT_ACCELERANT or B_CLONE_ACCELERANT will be requested and
subsequently called before any other hook is requested.  All other feature
hook selections can be predicated on variables assigned during the accelerant
initialization process.
*/
		/* initialization */
		HOOK(INIT_ACCELERANT);
		HOOK(CLONE_ACCELERANT);

		HOOK(ACCELERANT_CLONE_INFO_SIZE);
		HOOK(GET_ACCELERANT_CLONE_INFO);
		HOOK(UNINIT_ACCELERANT);
		HOOK(GET_ACCELERANT_DEVICE_INFO);
		HOOK(ACCELERANT_RETRACE_SEMAPHORE);

		/* mode configuration */
		HOOK(ACCELERANT_MODE_COUNT);
		HOOK(GET_MODE_LIST);
		HOOK(PROPOSE_DISPLAY_MODE);
		HOOK(SET_DISPLAY_MODE);
		HOOK(GET_DISPLAY_MODE);
		HOOK(GET_FRAME_BUFFER_CONFIG);
		HOOK(GET_PIXEL_CLOCK_LIMITS);
		HOOK(MOVE_DISPLAY);
		HOOK(SET_INDEXED_COLORS);
		//HOOK(GET_TIMING_CONSTRAINTS);
#ifdef __HAIKU__
		case B_GET_PREFERRED_DISPLAY_MODE:
			return (void*)radeon_get_preferred_display_mode;
		case B_GET_EDID_INFO:
			return (void*)radeon_get_edid_info;
#endif

		HOOK(DPMS_CAPABILITIES);
		HOOK(DPMS_MODE);
		HOOK(SET_DPMS_MODE);

		/* cursor managment */
		HOOK(SET_CURSOR_SHAPE);
		HOOK(MOVE_CURSOR);
		HOOK(SHOW_CURSOR);

		/* synchronization */
		HOOK(ACCELERANT_ENGINE_COUNT);
		HOOK(ACQUIRE_ENGINE);
		HOOK(RELEASE_ENGINE);
		HOOK(WAIT_ENGINE_IDLE);
		HOOK(GET_SYNC_TOKEN);
		HOOK(SYNC_TO_TOKEN);

/*
When requesting an acceleration hook, the calling application provides a
pointer to the display_mode for which the acceleration function will be used.
Depending on the engine architecture, you may choose to provide a different
function to be used with each bit-depth.  In the sample driver we return
the same function all the time.
*/
		/* 2D acceleration */
		HOOK(SCREEN_TO_SCREEN_BLIT);
		HOOK(FILL_RECTANGLE);
		HOOK(INVERT_RECTANGLE);
		HOOK(FILL_SPAN);
		
		// overlay
		HOOK(OVERLAY_COUNT);
		HOOK(OVERLAY_SUPPORTED_SPACES);
		HOOK(OVERLAY_SUPPORTED_FEATURES);
		HOOK(ALLOCATE_OVERLAY_BUFFER);
		HOOK(RELEASE_OVERLAY_BUFFER);
		HOOK(GET_OVERLAY_CONSTRAINTS);
		HOOK(ALLOCATE_OVERLAY);
		HOOK(RELEASE_OVERLAY);
		HOOK(CONFIGURE_OVERLAY);
#undef HOOK
#undef ZERO
	}

	return NULL;
}
