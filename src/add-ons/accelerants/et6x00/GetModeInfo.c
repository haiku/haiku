/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 graphics driver for BeOS 5.
 * Copyright (c) 2003-2004, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#include "GlobalData.h"
#include "generic.h"


/*****************************************************************************/
/*
 * Return the current display mode.  The only time you
 * might return an error is if a mode hasn't been set.
 */
status_t GET_DISPLAY_MODE(display_mode *current_mode) {
    /* easy for us, we return the last mode we set */
    *current_mode = si->dm;
    return B_OK;
}
/*****************************************************************************/
/*
 * Return the frame buffer configuration information.
 */
status_t GET_FRAME_BUFFER_CONFIG(frame_buffer_config *afb) {
    /* easy again, as the last mode set stored the info in a convienient form */
    *afb = si->fbc;
    return B_OK;
}
/*****************************************************************************/
/*
 * Return the maximum and minium pixel clock limits for the specified mode.
 * Note that we're not making any guarantees about the ability of the
 * attached display to handle pixel clocks within the limits we return.
 * A future monitor capablilities database will post-process this
 * information.
 */
status_t GET_PIXEL_CLOCK_LIMITS(display_mode *dm, uint32 *low, uint32 *high) {
uint32 clockLimit;
uint32 totalPix = (uint32)dm->timing.h_total * (uint32)dm->timing.v_total;

    /* max pixel clock is pixel depth dependant */
    switch (dm->space & ~0x3000) {
 	case B_RGB24: clockLimit = si->pixelClockMax24; break;
	case B_RGB15:
	case B_RGB16: clockLimit = si->pixelClockMax16; break;
	default:
	    clockLimit = 0;
    }

    /* lower limit of about 48Hz vertical refresh */
    *low = (totalPix * 48L) / 1000L;
    if (*low > clockLimit) return B_ERROR;
    *high = clockLimit;

    return B_OK;
}
/*****************************************************************************/
