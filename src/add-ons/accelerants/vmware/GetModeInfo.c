/*
 * Copyright 1999, Be Incorporated.
 * Copyright 2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Be Incorporated
 *		Eric Petit <eric.petit@lapsus.org>
 */


#include <SupportDefs.h>
#include "GlobalData.h"


status_t
GET_DISPLAY_MODE(display_mode *currentMode)
{
	TRACE("GET_DISPLAY_MODE\n");

	*currentMode = gSi->dm;
	return B_OK;
}


status_t
GET_FRAME_BUFFER_CONFIG(frame_buffer_config *afb)
{
	TRACE("GET_FRAME_BUFFER_CONFIG\n");

	afb->frame_buffer = (uint8 *)gSi->fb + gSi->fbOffset;
	afb->frame_buffer_dma = (uint8 *)gSi->fbDma + gSi->fbOffset;
	afb->bytes_per_row = gSi->bytesPerRow;

	TRACE("fb: %p, fb_dma: %p, row: %"B_PRIu32" bytes\n", afb->frame_buffer,
		afb->frame_buffer_dma, afb->bytes_per_row);

	return B_OK;
}


status_t
GET_PIXEL_CLOCK_LIMITS(display_mode *dm, uint32 *low, uint32 *high)
{
	TRACE("GET_PIXEL_CLOCK_LIMITS\n");

	/* ? */
	*low = 1;
	*high = 0xFFFFFFFF;
	return B_OK;
}


sem_id
ACCELERANT_RETRACE_SEMAPHORE()
{
	return B_ERROR;
}

