/*
 * Copyright 1999, Be Incorporated.
 * Copyright 2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Be Incorporated
 *		Eric Petit <eric.petit@lapsus.org>
 */

#include <string.h>
#include "GlobalData.h"


status_t
SET_DISPLAY_MODE(display_mode * modeToSet)
{
	display_mode bounds, target;
	int bpp;

	TRACE("SET_DISPLAY_MODE\n");

	/* Ask for the specific mode */
	target = bounds = *modeToSet;
	if (PROPOSE_DISPLAY_MODE(&target, &bounds, &bounds) == B_ERROR)
		return B_ERROR;

	bpp = BppForSpace(target.space);

	TRACE("setting %dx%dx%d\n", target.virtual_width,
		target.virtual_height, bpp);

	gSi->dm = target;

	/* Disable SVGA while we switch */
	ioctl(gFd, VMWARE_FIFO_STOP, NULL, 0);

	/* Re-init fifo */
	FifoInit();

	/* Set resolution. This also updates the frame buffer config in the
	 * shared area */
	ioctl(gFd, VMWARE_SET_MODE, &target, sizeof(target));

	/* Blank the screen to avoid ugly glitches until the app_server redraws */
	memset(gSi->fb, 0, target.virtual_height * gSi->bytesPerRow);
	FifoUpdateFullscreen();

	/* Re-enable SVGA */
	ioctl(gFd, VMWARE_FIFO_START, NULL, 0);

	return B_OK;
}


status_t
MOVE_DISPLAY(uint16 hDisplayStart, uint16 vDisplayStart)
{
	TRACE("MOVE_DISPLAY (%d, %d)\n", hDisplayStart, vDisplayStart);
	return B_ERROR;
}


void
SET_INDEXED_COLORS(uint count, uint8 first, uint8 *color_data, uint32 flags)
{
	// TODO: Take first and count into account.
	// Currently we always pass 256 colors (random, most of the times) to the driver. Cool!
	uint8 colors[256];
	uint32 i;	
	TRACE("SET_INDEXED_COLORS\n");

	for (i = 0; i < count; i++) {
		colors[i + first] = color_data[i];
	}
 
	ioctl(gFd, VMWARE_SET_PALETTE, colors, sizeof(count));
}


/*--------------------------------------------------------------------*/
/* DPMS_CAPABILITIES: reports DPMS capabilities (on, stand by,
 * suspend, off) */
uint32
DPMS_CAPABILITIES()
{
	/* We stay always on */
	return B_DPMS_ON;
}


/*--------------------------------------------------------------------*/
/* DPMS_MODE: reports the current DPMS mode */
uint32
DPMS_MODE()
{
	return B_DPMS_ON;
}


/*--------------------------------------------------------------------*/
/* SET_DPMS_MODE: puts the display into one of the DPMS modes */
status_t
SET_DPMS_MODE(uint32 flags)
{
	return B_ERROR;
}

