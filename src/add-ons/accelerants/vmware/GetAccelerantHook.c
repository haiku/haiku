/*
 * Copyright 1999, Be Incorporated.
 * Copyright 2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Be Incorporated
 *		Eric Petit <eric.petit@lapsus.org>
 */

#include "GlobalData.h"


void *
get_accelerant_hook(uint32 feature, void *data)
{
	switch (feature) {
#define HOOK(x) case B_##x: return (void *)x
#define ZERO(x) case B_##x: return (void *)0

		/* initialization */
		HOOK(INIT_ACCELERANT);
		HOOK(CLONE_ACCELERANT);

		HOOK(ACCELERANT_CLONE_INFO_SIZE);
		HOOK(GET_ACCELERANT_CLONE_INFO);
		HOOK(UNINIT_ACCELERANT);
		//HOOK(GET_ACCELERANT_DEVICE_INFO);
		HOOK(ACCELERANT_RETRACE_SEMAPHORE);

		/* mode configuration */
		HOOK(ACCELERANT_MODE_COUNT);
		HOOK(GET_MODE_LIST);
		HOOK(PROPOSE_DISPLAY_MODE);
		HOOK(SET_DISPLAY_MODE);
		HOOK(GET_DISPLAY_MODE);
		HOOK(GET_FRAME_BUFFER_CONFIG);
		HOOK(GET_PIXEL_CLOCK_LIMITS);
		ZERO(MOVE_DISPLAY);
		HOOK(SET_INDEXED_COLORS);
		ZERO(GET_TIMING_CONSTRAINTS);

		/* Power Managment (SetDisplayMode.c) */
		HOOK(DPMS_CAPABILITIES);
		HOOK(DPMS_MODE);
		HOOK(SET_DPMS_MODE);

		/* Cursor managment (Cursor.c) */
#if 0
		if (gSi->capabilities & SVGA_CAP_ALPHA_CURSOR) {
			HOOK(SET_CURSOR_SHAPE);
			HOOK(MOVE_CURSOR);
			HOOK(SHOW_CURSOR);
		}
#endif

		/* synchronization */
		HOOK(ACCELERANT_ENGINE_COUNT);
		HOOK(ACQUIRE_ENGINE);
		HOOK(RELEASE_ENGINE);
		HOOK(WAIT_ENGINE_IDLE);
		HOOK(GET_SYNC_TOKEN);
		HOOK(SYNC_TO_TOKEN);

		/* 2D acceleration (Acceleration.c) */
		if (gSi->capabilities & SVGA_CAP_RECT_COPY)
			HOOK(SCREEN_TO_SCREEN_BLIT);
		ZERO(FILL_RECTANGLE);
		ZERO(INVERT_RECTANGLE);
		ZERO(FILL_SPAN);
		ZERO(SCREEN_TO_SCREEN_TRANSPARENT_BLIT);
		ZERO(SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT);

#undef HOOK
#undef ZERO
	}

	return 0;
}
