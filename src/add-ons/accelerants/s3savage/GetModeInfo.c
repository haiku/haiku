/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Gerald Zajac 2006-2007
*/

#include "GlobalData.h"
#include "AccelerantPrototypes.h"

/*
	Return the current display mode.  The only time you might return an
	error is if a mode hasn't been set.
*/
status_t 
GET_DISPLAY_MODE (display_mode *current_mode)
{
	/* easy for us, we return the last mode we set */
	*current_mode = si->dm;
	return B_OK;
}


/*
	Return the frame buffer configuration information.
*/
status_t 
GET_FRAME_BUFFER_CONFIG(frame_buffer_config *pFBC)
{
//	TRACE(("GET_FRAME_BUFFER_CONFIG called\n"));

	*pFBC = si->fbc;
	return B_OK;
}


/*
	Return the maximum and minium pixel clock limits for the specified mode.
*/
status_t 
GET_PIXEL_CLOCK_LIMITS(display_mode *dm, uint32 *low, uint32 *high)
{
	/*
		 Note that we're not making any guarantees about the ability of the attached
		 display to handle pixel clocks within the limits we return.  A future monitor
		 capablilities database will post-process this information.
	*/
	uint32 total_pix = (uint32)dm->timing.h_total * (uint32)dm->timing.v_total;
	uint32 clock_limit;

	/* max pixel clock is pixel depth dependant */
	switch (dm->space & ~0x3000) {
	case B_RGB32:
		clock_limit = si->pix_clk_max32;
		break;
	case B_RGB15:
	case B_RGB16:
		clock_limit = si->pix_clk_max16;
		break;
	case B_CMAP8:
		clock_limit = si->pix_clk_max8;
		break;
	default:
		clock_limit = 0;
	}
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
sem_id 
ACCELERANT_RETRACE_SEMAPHORE(void)
{
//	TRACE(("ACCELERANT_RETRACE_SEMAPHORE() called\n"));

//	if (si->bInterruptAssigned && si->vblank >= 0)
//		return si->vblank;
//	else
		return B_ERROR;
}

