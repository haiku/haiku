/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

/*******************************************************************************
/
/	File:			ColorTools.h
/
/   Description:    Additional experimental color manipulation functions.
/
/	Copyright 2000, Be Incorporated, All Rights Reserved
/
*******************************************************************************/


#ifndef _COLOR_TOOLS_H
#define _COLOR_TOOLS_H

#include <GraphicsDefs.h>

#if B_BEOS_VERSION <= B_BEOS_VERSION_5

namespace BExperimental {

// Comparison operators.

inline bool operator==(const rgb_color c1, const rgb_color c2)
{
	return (*((uint32*)&c1)) == (*((uint32*)&c2));
}

inline bool operator!=(const rgb_color c1, const rgb_color c2)
{
	return (*((uint32*)&c1)) != (*((uint32*)&c2));
}

#ifndef __HAIKU__
// Color creation.

inline rgb_color make_color(uint8 red, uint8 green, uint8 blue, uint8 alpha=255)
{
	rgb_color c;
	c.red = red;
	c.green = green;
	c.blue = blue;
	c.alpha = alpha;
	return c;
}
#endif

// Mix two colors together, ignoring their relative alpha channels.
// If amount is 0, the result is color1; if 255, the result is color2;
// if another value, it is somewhere in-between.  The resulting alpha
// channel is mixed exactly like the other color channels.
rgb_color mix_color(rgb_color color1, rgb_color color2, uint8 amount);

// Blend two colors together, weighting by their relative alpha channels.
// The resulting color is the same as mix_color(), except that the amount
// used from color1 and color2's color channels is dependent on that color's
// alpha channel.  For example, if color1.alpha is 0 and color2.alpha is
// 255, the resulting red, green, and blue values will be the same as those
// in color2, regardless of 'amount'.
rgb_color blend_color(rgb_color color1, rgb_color color2, uint8 amount);

// Return a color that is the disabled representation of 'color' when drawn
// on a solid color 'background'.
rgb_color disable_color(rgb_color color, rgb_color background);

}	// namespace BExperimental

using namespace BExperimental;

#endif

#endif
