/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
	
	Other authors:
	Mark Watson,
	Rudolf Cornelissen 10/2002-8/2009
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
#define CHKO(x) case B_##x: \
	if (check_overlay_capability(B_##x) == B_OK) return (void *)x; else return (void *)0
#define CHKA(x) case B_##x: \
	if (check_acc_capability(B_##x) == B_OK) \
	{if(!si->settings.dma_acc) return (void *)x##_PIO; else return (void *)x##_DMA;} \
	else return (void *)0
#define CHKS(x) case B_##x: \
	if(!si->settings.dma_acc) return (void *)x##_PIO; else return (void *)x##_DMA
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
#ifdef __HAIKU__
		HOOK(GET_EDID_INFO);
		HOOK(GET_PREFERRED_DISPLAY_MODE);
#endif
		HOOK(GET_FRAME_BUFFER_CONFIG);
		HOOK(GET_PIXEL_CLOCK_LIMITS);
		HOOK(MOVE_DISPLAY);
		HOOK(SET_INDEXED_COLORS);
		HOOK(GET_TIMING_CONSTRAINTS);

		HOOK(DPMS_CAPABILITIES);
		HOOK(DPMS_MODE);
		HOOK(SET_DPMS_MODE);

		/* cursor managment */
		HRDC(SET_CURSOR_SHAPE);
		HRDC(MOVE_CURSOR);
		HRDC(SHOW_CURSOR);

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

		/* only export video overlay functions if card is capable of it */
		CHKO(OVERLAY_COUNT);
		CHKO(OVERLAY_SUPPORTED_SPACES);
		CHKO(OVERLAY_SUPPORTED_FEATURES);
		CHKO(ALLOCATE_OVERLAY_BUFFER);
		CHKO(RELEASE_OVERLAY_BUFFER);
		CHKO(GET_OVERLAY_CONSTRAINTS);
		CHKO(ALLOCATE_OVERLAY);
		CHKO(RELEASE_OVERLAY);
		CHKO(CONFIGURE_OVERLAY);

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
		CHKA(SCREEN_TO_SCREEN_BLIT);
		CHKA(FILL_RECTANGLE);
		CHKA(INVERT_RECTANGLE);
		CHKA(FILL_SPAN);
		/* not (yet) used by the app_server:
		 * so just for application use (BWindowScreen) */
//		CHKA(SCREEN_TO_SCREEN_TRANSPARENT_BLIT);
		CHKA(SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT);
	}

	/* Return a null pointer for any feature we don't understand. */
	return 0;
}
#undef CHKO
#undef CHKA
#undef CHKD
#undef HOOK
#undef ZERO
#undef HRDC

status_t check_overlay_capability(uint32 feature)
{
	char *msg = "";

	/* setup logmessage text */
	switch (feature)
	{
	case B_OVERLAY_COUNT:
		msg = "B_OVERLAY_COUNT";
		break;
	case B_OVERLAY_SUPPORTED_SPACES:
		msg = "B_OVERLAY_SUPPORTED_SPACES";
		break;
	case B_OVERLAY_SUPPORTED_FEATURES:
		msg = "B_OVERLAY_SUPPORTED_FEATURES";
		break;
	case B_ALLOCATE_OVERLAY_BUFFER:
		msg = "B_ALLOCATE_OVERLAY_BUFFER";
		break;
	case B_RELEASE_OVERLAY_BUFFER:
		msg = "B_RELEASE_OVERLAY_BUFFER";
		break;
	case B_GET_OVERLAY_CONSTRAINTS:
		msg = "B_GET_OVERLAY_CONSTRAINTS";
		break;
	case B_ALLOCATE_OVERLAY:
		msg = "B_ALLOCATE_OVERLAY";
		break;
	case B_RELEASE_OVERLAY:
		msg = "B_RELEASE_OVERLAY";
		break;
	case B_CONFIGURE_OVERLAY:
		msg = "B_CONFIGURE_OVERLAY";
		break;
	default:
		msg = "UNKNOWN";
		break;
	}

	/* all older cards have a supported bes */
	if ((si->ps.card_type <= NV40) || (si->ps.card_type == NV45))
	{
		LOG(4, ("Overlay: Exporting hook %s.\n", msg));
		return B_OK;
	}

	/* all newer NV40 architecture cards have a new HDTV capable bes except for
	 * GeForce 6800's. Unfortunately we have no info about the new bes yet. */
	LOG(4, ("Overlay: Not exporting hook %s.\n", msg));
	return B_ERROR;
}

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
		/* this function is only defined for DMA acceleration,
		 * but doesn't support the B_CMAP8 colorspace */
		//fixme: checkout B_CMAP8 support sometime, as some cards seem to support it?
		if (!si->settings.dma_acc || (si->dm.space == B_CMAP8))
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
	 * memory pitch.. and acceleration must not be blocked */
	if (si->acc_mode && !si->settings.block_acc)
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
