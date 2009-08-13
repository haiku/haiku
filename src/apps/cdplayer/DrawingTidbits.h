/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

// TODO: remove this file again.... It originates from Be Sample code, 
// but was added to the VLC Media Player BeOS interface, I added some stuff
// during my work on VLC, but I am not sure anymore if this file still 
// contains work done by Tony Castley, which would be GPL!

#ifndef __DRAWING_TIBITS__
#define __DRAWING_TIBITS__

#include <GraphicsDefs.h>

class BBitmap;

const rgb_color kBlack			= {   0,   0,   0, 255 };

// dims bitmap (in place) by finding the distance of
// the color at each pixel to the provided "center" color
// and shortens that distance by dimLevel
//   (dimLevel < 1 -> less contrast)
//   (dimLevel > 1 -> more contrast)
//   (dimLevel < 0 -> inverted colors)
// currently supported colorspaces:
//   B_RGB32
//   B_RGBA32
//   B_CMAP8
status_t dim_bitmap(BBitmap* bitmap, rgb_color center,
					float dimLevel);

rgb_color dimmed_color_cmap8(rgb_color color, rgb_color center,
							 float dimLevel);

#endif	// __DRAWING_TIBITS__
