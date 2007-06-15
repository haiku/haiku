/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Gerald Zajac 2006-2007
*/

#include "GlobalData.h"
#include "AccelerantPrototypes.h"
#include "savage.h"
#include <sys/ioctl.h>
#include <stdio.h>

/*
	Enable/Disable interrupts.	Just a wrapper around the
	ioctl() to the kernel driver.
	*/
static void 
interrupt_enable(bool bEnable)
{
	status_t result;
	SavageSetBoolState sbs;

	if (si->bInterruptAssigned) {
		/* set the magic number so the driver knows we're for real */
		sbs.magic = SAVAGE_PRIVATE_DATA_MAGIC;
		sbs.bEnable = bEnable;
		/* contact driver and get a pointer to the registers and shared data */
		result = ioctl(driverFileDesc, SAVAGE_RUN_INTERRUPTS, &sbs, sizeof(sbs));
	}
}

/*
	Calculates the number of bits for a given color_space.
	Usefull for mode setup routines, etc.
	*/
static uint32 
CalcBitsPerPixel(uint32 cs)
{
	uint32 bpp = 0;

	switch (cs) {
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
		bpp = 32;
		break;
	case B_RGB24_BIG:
	case B_RGB24_LITTLE:
		bpp = 24;
		break;
	case B_RGB16_BIG:
	case B_RGB16_LITTLE:
		bpp = 16;
		break;
	case B_RGB15_BIG:
	case B_RGBA15_BIG:
	case B_RGB15_LITTLE:
	case B_RGBA15_LITTLE:
		bpp = 15;
		break;
	case B_CMAP8:
		bpp = 8;
		break;
	}
	return bpp;
}


/*
	The code to actually configure the display.
	Do all of the error checking in PROPOSE_DISPLAY_MODE(),
	and just assume that the values I get here are acceptable.
*/
static void 
SetDisplayMode(display_mode* dm)
{
	// The code to actually configure the display.
	// All the error checking must be done in PROPOSE_DISPLAY_MODE(),
	// and assume that the mode values we get here are acceptable.

	uint		bpp = CalcBitsPerPixel(dm->space);
	DisplayMode	mode;

	TRACE(("SetDisplayMode begin;  depth = %d, width = %d, height = %d\n",
		  bpp, dm->virtual_width, dm->virtual_height));

	interrupt_enable(false);	// disable interrupts using kernel driver


	mode.timing = dm->timing;
	mode.bpp = bpp;
	mode.width = dm->virtual_width;
	mode.bytesPerRow = dm->virtual_width * ((bpp + 7) / 8);
	TRACE(("TIMING = { %d  %d %d %d %d  %d %d %d %d  0x%x }\n",
		  mode.timing.pixel_clock,
		  mode.timing.h_display, mode.timing.h_sync_start, mode.timing.h_sync_end,
		  mode.timing.h_total,
		  mode.timing.v_display, mode.timing.v_sync_start, mode.timing.v_sync_end,
		  mode.timing.v_total, mode.timing.flags));

	if (SavageModeInit(&mode)) {
		TRACE(("SavageModeInit succeeded\n"));
	} else {
		TRACE(("SavageModeInit failed\n"));
	}

	interrupt_enable(true);		// enable interrupts using kernel driver

	si->fbc.bytes_per_row = dm->virtual_width * ((bpp + 7) / 8);
	si->bitsPerPixel = bpp;

	SavageAdjustFrame(dm->h_display_start, dm->v_display_start);

	TRACE(("SetDisplayMode done\n"));
}


status_t 
SET_DISPLAY_MODE (display_mode* mode_to_set)
{
	// The exported mode setting routine.	First validate the mode,
	// then call our private routine to hammer the registers.

	display_mode	bounds, target;

	/* ask for the specific mode */
	target = bounds = *mode_to_set;
	if (PROPOSE_DISPLAY_MODE(&target, &bounds, &bounds) == B_ERROR)
		return B_ERROR;

	SetDisplayMode(&target);
	si->dm = target;
	return B_OK;
}

/*
	Set which pixel of the virtual frame buffer will show up in the
	top left corner of the display device.	Used for page-flipping
	games and virtual desktops.
	*/
status_t 
MOVE_DISPLAY(uint16 h_display_start, uint16 v_display_start)
{
	/*
	Many devices have limitations on the granularity of the horizontal offset.
	Make any checks for this here.  A future revision of the driver API will
	add a hook to return the granularity for a given display mode.
	*/
	/* most cards can handle multiples of 8 */
	if (h_display_start & 0x07)
		return B_ERROR;
	/* do not run past end of display */
	if ((si->dm.timing.h_display + h_display_start) > si->dm.virtual_width)
		return B_ERROR;
	if ((si->dm.timing.v_display + v_display_start) > si->dm.virtual_height)
		return B_ERROR;

	/* everybody remember where we parked... */
	si->dm.h_display_start = h_display_start;
	si->dm.v_display_start = v_display_start;

	/* actually set the registers */
	SavageAdjustFrame(h_display_start, v_display_start);

	return B_OK;
}

/*
	Set the indexed color palette.
	*/
void 
SET_INDEXED_COLORS(uint count, uint8 first, uint8* colorData, uint32 flags)
{
	(void)flags;		// avoid compiler warning for unused arg

	/*
	Some cards use the indexed color regisers in the DAC for gamma correction.
	If this is true with your device (and it probably is), you need to protect
	against setting these registers when not in an indexed mode.
	*/
	if (si->dm.space != B_CMAP8)
		return ;

	/*
	There isn't any need to keep a copy of the data being stored,
	as the app_server will set the colors each time it switches to
	an 8bpp mode, or takes ownership of an 8bpp mode after a GameKit
	app has used it.
	*/

	OUTREG16(VGA_SEQ_INDEX, 0x101b);

	while (count--) {
		OUTREG8(0x83c8, first++);			// color index
		OUTREG8(0x83c9, colorData[0]);		// red
		OUTREG8(0x83c9, colorData[1]);		// green
		OUTREG8(0x83c9, colorData[2]);		// blue
		colorData += 3;
	}
}

