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
const rgb_color kWhite			= { 255, 255, 255, 255 };

rgb_color ShiftColor(rgb_color , float );

inline rgb_color
Color(int32 r, int32 g, int32 b, int32 alpha = 255)
{
	rgb_color result;
	result.red = r;
	result.green = g;
	result.blue = b;
	result.alpha = alpha;

	return result;
}

const float kDarkness = 1.06;
const float kDimLevel = 0.6;

void ReplaceColor(BBitmap *bitmap, rgb_color from, rgb_color to);
void ReplaceTransparentColor(BBitmap *bitmap, rgb_color with);

// function can be used to scale the upper left part of
// a bitmap to fill the entire bitmap, ie fromWidth
// and fromHeight must be smaller or equal to the bitmaps size!
// only supported colorspaces are B_RGB32 and B_RGBA32
status_t scale_bitmap( BBitmap* bitmap,
					   uint32 fromWidth, uint32 fromHeight );

// bitmaps need to be the same size, or this function will fail
// currently supported conversions:
//   B_YCbCr422	-> B_RGB32
//   B_RGB32	-> B_RGB32
//   B_RGB16	-> B_RGB32
// not yet implemented conversions:
//   B_YCbCr420	-> B_RGB32
//   B_YUV422	-> B_RGB32
status_t convert_bitmap(BBitmap* inBitmap, BBitmap* outBitmap);

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
