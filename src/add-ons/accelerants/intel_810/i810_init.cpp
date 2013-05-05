/*
 * Copyright 2012 Haiku, Inc.  All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
 */

/*!
	Haiku Intel-810 video driver was adapted from the X.org intel driver which
	has the following copyright.

	Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
	All Rights Reserved.
 */


#include "accelerant.h"
#include "i810_regs.h"


bool
I810_GetColorSpaceParams(int colorSpace, uint8& bitsPerPixel,
	uint32& maxPixelClock)
{
	// Get parameters for a color space which is supported by the i810 chips.
	// Argument maxPixelClock is in KHz.
	// Return true if the color space is supported;  else return false.

	switch (colorSpace) {
		case B_RGB16:
			bitsPerPixel = 16;
			maxPixelClock = 163000;
			break;
			break;
		case B_CMAP8:
			bitsPerPixel = 8;
			maxPixelClock = 203000;
			break;
		default:
			TRACE("Unsupported color space: 0x%X\n", colorSpace);
			return false;
	}

	return true;
}


status_t
I810_Init(void)
{
	TRACE("I810_Init()\n");

	SharedInfo& si = *gInfo.sharedInfo;

	// Use all of video memory for the frame buffer.

	si.maxFrameBufferSize = si.videoMemSize;

	// Set up the array of the supported color spaces.

	si.colorSpaces[0] = B_CMAP8;
	si.colorSpaces[1] = B_RGB16;
	si.colorSpaceCount = 2;

	// Setup the mode list.

	return CreateModeList(IsModeUsable);
}
