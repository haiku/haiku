/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 graphics driver for BeOS 5.
 * Copyright (c) 2003-2004, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#include "generic.h"


/*****************************************************************************/
/*
 * The standard entry point.  Given a uint32 feature identifier, this routine
 * returns a pointer to the function that implements the feature. Some features
 * require more information than just the identifier to select the proper
 * function. The extra information (which is specific to the feature) is
 * pointed at by the void *data parameter. By default, no extra information
 * is available. Any extra information available to choose the function will
 * be noted on a case by case below.
 */
void *get_accelerant_hook(uint32 feature, void *data) {
/* These definition is out of pure lazyness.*/
#define HOOK(x) case B_##x: return (void *)x

    switch (feature) {

	/*
	 * One of either B_INIT_ACCELERANT or B_CLONE_ACCELERANT will be
         * requested and subsequently called before any other hook is
         * requested. All other feature hook selections can be predicated
         * on variables assigned during the accelerant initialization process.
	 */
	/* initialization */
	HOOK(INIT_ACCELERANT);
	HOOK(CLONE_ACCELERANT);

	HOOK(ACCELERANT_CLONE_INFO_SIZE);
	HOOK(GET_ACCELERANT_CLONE_INFO);
	HOOK(UNINIT_ACCELERANT);
	HOOK(GET_ACCELERANT_DEVICE_INFO);

///     HOOK(ACCELERANT_RETRACE_SEMAPHORE); /* Not implemented. Would be useful to have it implemented. */

	/* mode configuration */
	HOOK(ACCELERANT_MODE_COUNT);
	HOOK(GET_MODE_LIST);
///	HOOK(PROPOSE_DISPLAY_MODE);
	HOOK(SET_DISPLAY_MODE);
	HOOK(GET_DISPLAY_MODE);
	HOOK(GET_FRAME_BUFFER_CONFIG);
	HOOK(GET_PIXEL_CLOCK_LIMITS);

	/* synchronization */
	HOOK(ACCELERANT_ENGINE_COUNT);
	HOOK(ACQUIRE_ENGINE);
	HOOK(RELEASE_ENGINE);
	HOOK(WAIT_ENGINE_IDLE);
	HOOK(GET_SYNC_TOKEN);
	HOOK(SYNC_TO_TOKEN);

	/* 2D acceleration */
	HOOK(SCREEN_TO_SCREEN_BLIT);
	HOOK(FILL_RECTANGLE);
    }

    /* Return a null pointer for any feature we don't understand. */
    return 0;

#undef HOOK
}
/*****************************************************************************/
