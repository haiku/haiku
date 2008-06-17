/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
	
	Other authors:
	Mark Watson,
	Rudolf Cornelissen 10/2002-6/2008
*/

#define MODULE_BIT 0x08000000

#include "acc_std.h"

/*
The standard entry point.  Given a uint32 feature identifier, this routine
returns a pointer to the function that implements the feature.  Some features
require more information than just the identifier to select the proper
function.  The extra information (which is specific to the feature) is
pointed at by the void *data parameter.  By default, no extra information
is available.  Any extra information available to choose the function will be
noted on a case by case below.
*/

/*
These definitions are out of pure lazyness.
*/
#define CHKA(x) case B_##x: \
	if (check_acc_capability(B_##x) == B_OK) \
		return (void *)x##_DMA; \
	else						\
		return (void *)0
#define CHKS(x) case B_##x: return (void *)x##_DMA
#define HOOK(x) case B_##x: return (void *)x
#define ZERO(x) case B_##x: return (void *)0
#define HRDC(x) case B_##x: return si->settings.hardcursor? (void *)x: (void *)0; // apsed

void *	get_accelerant_hook(uint32 feature, void *data)
{
	switch (feature)
	{
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
		HOOK(GET_TIMING_CONSTRAINTS);

		HOOK(DPMS_CAPABILITIES);
		HOOK(DPMS_MODE);
		HOOK(SET_DPMS_MODE);

		/* cursor managment */
		//HRDC(SET_CURSOR_SHAPE);
		//HRDC(MOVE_CURSOR);
		//HRDC(SHOW_CURSOR);

		/* synchronization */
		HOOK(ACCELERANT_ENGINE_COUNT);
		CHKS(ACQUIRE_ENGINE);
		HOOK(RELEASE_ENGINE);
		HOOK(WAIT_ENGINE_IDLE);
		HOOK(GET_SYNC_TOKEN);
		HOOK(SYNC_TO_TOKEN);

		/*
		Depending on the engine architecture, you may choose to provide a different
		function to be used with each bit-depth for example.

		Note: These hooks are re-acquired by the app_server after each mode switch.
		*/

		/* video overlay functions are not supported */

		/*
		When requesting an acceleration hook, the calling application provides a
		pointer to the display_mode for which the acceleration function will be used.
		Depending on the engine architecture, you may choose to provide a different
		function to be used with each bit-depth.  In the sample driver we return
		the same function all the time.

		Note: These hooks are re-acquired by the app_server after each mode switch.
		*/

		/* only export 2D acceleration functions in modes that are capable of it */
		/* used by the app_server and applications (BWindowScreen) */
		//CHKA(SCREEN_TO_SCREEN_BLIT);
		//CHKA(FILL_RECTANGLE);
		//CHKA(INVERT_RECTANGLE);
		//CHKA(FILL_SPAN);
		/* not (yet) used by the app_server:
		 * so just for application use (BWindowScreen) */
//		CHKA(SCREEN_TO_SCREEN_TRANSPARENT_BLIT);
		//CHKA(SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT);
	}

	/* Return a null pointer for any feature we don't understand. */
	return 0;
}
#undef CHKA
#undef CHKD
#undef HOOK
#undef ZERO
#undef HRDC

status_t check_acc_capability(uint32 feature)
{
	char *msg = "";

	/* setup logmessage text */
	switch (feature)
	{
	case B_SCREEN_TO_SCREEN_BLIT:
		msg = "B_SCREEN_TO_SCREEN_BLIT";
		break;
	case B_FILL_RECTANGLE:
		msg = "B_FILL_RECTANGLE";
		break;
	case B_INVERT_RECTANGLE:
		msg = "B_INVERT_RECTANGLE";
		break;
	case B_FILL_SPAN:
		msg = "B_FILL_SPAN";
		break;
	case B_SCREEN_TO_SCREEN_TRANSPARENT_BLIT:
		msg = "B_SCREEN_TO_SCREEN_TRANSPARENT_BLIT";
		break;
	case B_SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT:
		msg = "B_SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT";
		/* this function doesn't support the B_CMAP8 colorspace */
		//fixme: checkout B_CMAP8 support sometime, as some cards seem to support it?
		if (si->dm.space == B_CMAP8)
		{
			LOG(4, ("Acc: Not exporting hook %s.\n", msg));
			return B_ERROR;
		}
		break;
	default:
		msg = "UNKNOWN";
		break;
	}

	/* hardware acceleration is only supported in modes with upto a certain
	 * memory pitch.. */
	if (si->acc_mode)
	{
		LOG(4, ("Acc: Exporting hook %s.\n", msg));
		return B_OK;
	}
	else
	{
		LOG(4, ("Acc: Not exporting hook %s.\n", msg));
		return B_ERROR;
	}
}
