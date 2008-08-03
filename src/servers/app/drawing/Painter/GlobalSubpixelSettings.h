/*
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Global settings of the app server regarding antialiasing and font hinting
 *
 */

#ifndef GLOBAL_SUBPIXEL_SETTINGS_H
#define GLOBAL_SUBPIXEL_SETTINGS_H

#define AVERAGE_BASED_SUBPIXEL_FILTERING

extern bool gSubpixelAntialiasing;
extern bool gDefaultHinting;
	
// The weight with which the average of the subpixels is applied to counter
// color fringes (0 = full sharpness ... 255 = grayscale anti-aliasing)
extern unsigned char gSubpixelAverageWeight;

// There are two types of LCD displays in general - the more common have
// sub - pixels physically ordered as RGB within a pixel, but some are BGR.
// Sub - pixel antialiasing optimised for one ordering obviously doesn't work
// on the other.
extern bool gSubpixelOrderingRGB;

#endif // GLOBAL_SUBPIXEL_SETTINGS_H


