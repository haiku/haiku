/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 graphics driver for BeOS 5.
 * Copyright (c) 2003-2004, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#include "GlobalData.h"
#include "generic.h"
#include <sys/ioctl.h>


/*****************************************************************************/
/*
 * The code to actually configure the display.
 */
static status_t doSetDisplayMode(display_mode *dm) {
ET6000DisplayMode mode;

    mode.magic = ET6000_PRIVATE_DATA_MAGIC;
    mode.mode = *dm;
    mode.pciConfigSpace = si->pciConfigSpace;

    return ioctl(fd, ET6000_SET_DISPLAY_MODE, &mode, sizeof(mode));
}
/*****************************************************************************/
/*
 * The exported mode setting routine. First validate the mode,
 * then call our private routine to hammer the registers.
 */
status_t SET_DISPLAY_MODE(display_mode *mode_to_set) {
display_mode bounds, target;
status_t result;
uint8 bpp;

    /* ask for the specific mode */
    target = bounds = *mode_to_set;
    if (PROPOSE_DISPLAY_MODE(&target, &bounds, &bounds) != B_OK) /* ==B_ERROR???/// */
	return B_ERROR;

    result = doSetDisplayMode(&target);

    if (result == B_OK) {
        switch (target.space) {
            case B_RGB24_LITTLE:
            case B_RGB24_BIG:
                bpp = 3;
                break;
            case B_RGB16_LITTLE:
            case B_RGB16_BIG:
            case B_RGB15_LITTLE:
            case B_RGB15_BIG:
                bpp = 2;
                break;
			default:
				return B_BAD_VALUE;
        }
        si->fbc.bytes_per_row = target.virtual_width * bpp;
        si->dm = target;
        si->bytesPerPixel = bpp;
        et6000aclInit(bpp);
    }

    return result;
}
/*****************************************************************************/
