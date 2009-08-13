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

#include "DrawingTidbits.h"

#include <Bitmap.h>
#include <Debug.h>
#include <Screen.h>

#include <math.h>
#include <string.h>


// clip_float
inline uint8
clip_float(float value)
{
	if (value < 0)
		value = 0;
	if (value > 255)
		value = 255;
	return (uint8)value;
}

// dim_bitmap
status_t
dim_bitmap(BBitmap* bitmap, rgb_color center, float dimLevel)
{
	status_t status = B_BAD_VALUE;
	if (bitmap && bitmap->IsValid())
	{
		switch (bitmap->ColorSpace())
		{
			case B_CMAP8:
			{
				BScreen screen(B_MAIN_SCREEN_ID);
				if (screen.IsValid())
				{
					// iterate over each pixel, get the respective
					// color from the screen object, find the distance
					// to the "center" color and shorten the distance
					// by "dimLevel"
					int32 length = bitmap->BitsLength();
					uint8* bits = (uint8*)bitmap->Bits();
					for (int32 i = 0; i < length; i++)
					{
						// preserve transparent pixels
						if (bits[i] != B_TRANSPARENT_MAGIC_CMAP8)
						{
							// get color for this index
							rgb_color c = screen.ColorForIndex(bits[i]);
							// red
							float dist = (c.red - center.red) * dimLevel;
							c.red = clip_float(center.red + dist);
							// green
							dist = (c.green - center.green) * dimLevel;
							c.green = clip_float(center.green + dist);
							// blue
							dist = (c.blue - center.blue) * dimLevel;
							c.blue = clip_float(center.blue + dist);
							// write correct index of the dimmed color
							// back into bitmap (and hope the match is close...)
							bits[i] = screen.IndexForColor(c);
						}
					}
					status = B_OK;
				}
				break;
			}
			case B_RGB32:
			case B_RGBA32:
			{
				// iterate over each color component, find the distance
				// to the "center" color and shorten the distance
				// by "dimLevel"
				uint8* bits = (uint8*)bitmap->Bits();
				int32 bpr = bitmap->BytesPerRow();
				int32 pixels = bitmap->Bounds().IntegerWidth() + 1;
				int32 lines = bitmap->Bounds().IntegerHeight() + 1;
				// iterate over color components
				for (int32 y = 0; y < lines; y++) {
					for (int32 x = 0; x < pixels; x++) {
						int32 offset = 4 * x; // four bytes per pixel
						// blue
						float dist = (bits[offset + 0] - center.blue) * dimLevel;
						bits[offset + 0] = clip_float(center.blue + dist);
						// green
						dist = (bits[offset + 1] - center.green) * dimLevel;
						bits[offset + 1] = clip_float(center.green + dist);
						// red
						dist = (bits[offset + 2] - center.red) * dimLevel;
						bits[offset + 2] = clip_float(center.red + dist);
						// ignore alpha channel
					}
					// next line
					bits += bpr;
				}
				status = B_OK;
				break;
			}
			default:
				status = B_ERROR;
				break;
		}
	}
	return status;
}

// dimmed_color_cmap8
rgb_color
dimmed_color_cmap8(rgb_color color, rgb_color center, float dimLevel)
{
	BScreen screen(B_MAIN_SCREEN_ID);
	if (screen.IsValid())
	{
		// red
		float dist = (color.red - center.red) * dimLevel;
		color.red = clip_float(center.red + dist);
		// green
		dist = (color.green - center.green) * dimLevel;
		color.green = clip_float(center.green + dist);
		// blue
		dist = (color.blue - center.blue) * dimLevel;
		color.blue = clip_float(center.blue + dist);
		// get color index for dimmed color
		int32 index = screen.IndexForColor(color);
		// put color at index (closest match in palette
		// to dimmed result) into returned color
		color = screen.ColorForIndex(index);
	}
	return color;
}

