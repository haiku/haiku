/*
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef GLOBAL_SUBPIXEL_SETTINGS_H
#define GLOBAL_SUBPIXEL_SETTINGS_H

#include <SupportDefs.h>

// TODO: these global settings need to be removed - once we have more than one
//	user, we also must support more than one setting. That's why there is a
//	DesktopSettings class in the first place...

enum {
	HINTING_MODE_OFF = 0,
	HINTING_MODE_ON,
	HINTING_MODE_MONOSPACED_ONLY
};

//#define AVERAGE_BASED_SUBPIXEL_FILTERING

extern bool gSubpixelAntialiasing;
extern uint8 gDefaultHintingMode;

// The weight with which the average of the subpixels is applied to counter
// color fringes (0 = full sharpness ... 255 = grayscale anti-aliasing)
extern uint8 gSubpixelAverageWeight;

// There are two types of LCD displays in general - the more common have
// sub - pixels physically ordered as RGB within a pixel, but some are BGR.
// Sub - pixel antialiasing optimised for one ordering obviously doesn't work
// on the other.
extern bool gSubpixelOrderingRGB;

#endif // GLOBAL_SUBPIXEL_SETTINGS_H
