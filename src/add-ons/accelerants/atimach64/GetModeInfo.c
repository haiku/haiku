#include "GlobalData.h"
#include "generic.h"

extern uint32 calcBitsPerPixel(uint32 cs);

/*
	Return the current display mode.  The only time you might return an
	error is if a mode hasn't been set.
*/
status_t GET_DISPLAY_MODE(display_mode *current_mode) {
	/* easy for us, we return the last mode we set */
	*current_mode = si->dm;
	return B_OK;
}

/*
	Return the frame buffer configuration information.
*/
status_t GET_FRAME_BUFFER_CONFIG(frame_buffer_config *afb) {
	/* easy again, as the last mode set stored the info in a convienient form */
	*afb = si->fbc;
	return B_OK;
}

/*
	Return the maximum and minium pixel clock limits for the specified mode.
*/
status_t GET_PIXEL_CLOCK_LIMITS(display_mode *dm, uint32 *low, uint32 *high) {
/*
	Note that we're not making any guarantees about the ability of the attached
	display to handle pixel clocks within the limits we return.  A future monitor
	capablilities database will post-process this information.
*/
	uint32 total_pix = (uint32)dm->timing.h_total * (uint32)dm->timing.v_total;
	uint32 clock_limit;
   float i;

	/* max pixel clock is pixel depth dependant */
	switch (calcBitsPerPixel(dm->space)) {
		case 32: clock_limit = si->pix_clk_max32; break;
		case 15:
		case 16: clock_limit = si->pix_clk_max16; break;
		case 8: clock_limit = si->pix_clk_max8; break;
		default:
			clock_limit = 0;
	}
	/*   printf("low %d, high %d %d \n", *low, clock_limit,  si->pix_clk_max8);*/
	/* lower limit of about 48Hz vertical refresh */
	*low = (total_pix * 48L) / 1000L;
	i = (total_pix * 48L) / 1000L;

	/*   printf("low %d, high %d %08X \n", *low, clock_limit, dm->space);*/

	if (*low > clock_limit) return B_ERROR;
	*high = clock_limit;
	return B_OK;
}

/*
	Return the semaphore id that will be used to signal a vertical retrace
	occured.
*/
sem_id ACCELERANT_RETRACE_SEMAPHORE(void) {
	/*
	NOTE:
		The kernel driver created this for us.  We don't know if the system is
		using real interrupts, or if we're faking it, and we don't care.
		If we choose not to support this at all, we'd just return B_ERROR here,
		and the user wouldn't get any kind of vertical retrace support.
	*/
  /*	return si->vblank;*/
  return B_ERROR;
}
