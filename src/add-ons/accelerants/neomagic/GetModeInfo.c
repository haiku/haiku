/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Rudolf Cornelissen 4/2003-1/2006
*/

#define MODULE_BIT 0x02000000

#include "acc_std.h"

/*
	Return the current display mode.  The only time you might return an
	error is if a mode hasn't been set. Or if the system hands you a NULL pointer.
*/
status_t GET_DISPLAY_MODE(display_mode *current_mode)
{
	/* check for NULL pointer */
	if (current_mode == NULL) return B_ERROR;

	*current_mode = si->dm;
	return B_OK;
}

/* Return the frame buffer configuration information. */
status_t GET_FRAME_BUFFER_CONFIG(frame_buffer_config *afb)
{
	/* check for NULL pointer */
	if (afb == NULL) return B_ERROR;

	*afb = si->fbc;
	return B_OK;
}

/* Return the maximum and minium pixelclock limits for the specified mode. */
status_t GET_PIXEL_CLOCK_LIMITS(display_mode *dm, uint32 *low, uint32 *high) 
{
	uint32 max_pclk = 0;
	uint32 min_pclk = 0;

	/* check for NULL pointers */
	if ((dm == NULL) || (low == NULL) || (high == NULL)) return B_ERROR;

	/* specify requested info assuming CRT-only mode
	 * (if panel is active, CRTC and pixelclock are not programmed!) */
	{
		/* find min. value */
		switch (si->ps.card_type)
		{
			default:
				*low = (si->ps.min_pixel_vco * 1000);
				break;
		}
		/* find max. value */
		switch (dm->space)
		{
			case B_CMAP8:
				max_pclk = si->ps.max_dac1_clock_8;
				break;
			case B_RGB15_LITTLE:
			case B_RGB16_LITTLE:
				max_pclk = si->ps.max_dac1_clock_16;
				break;
			case B_RGB24_LITTLE:
				max_pclk = si->ps.max_dac1_clock_24;
				break;
			default:
				/* use fail-safe value */
				max_pclk = si->ps.max_dac1_clock_24;
				break;
		}
		/* return values in kHz */
		*high = max_pclk * 1000;
	}

	/* clamp lower limit to 48Hz vertical refresh for now.
	 * Apparantly the BeOS screenprefs app does limit the upper refreshrate to 90Hz,
	 * while it does not limit the lower refreshrate. */
	min_pclk = ((uint32)dm->timing.h_total * (uint32)dm->timing.v_total * 48) / 1000;
	if (min_pclk > *low) *low = min_pclk;

	return B_OK;
}

/* Return the semaphore id that will be used to signal a vertical sync occured.  */
sem_id ACCELERANT_RETRACE_SEMAPHORE(void)
{
	if (si->ps.int_assigned)
//		return si->vblank;
//temp:
		return B_ERROR;
	else
		return B_ERROR;
}
