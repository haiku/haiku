//------------------------------------------------------------------------------
//	Copyright (c) 2001-2004, Haiku
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		GraphicsDefs.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Caz <turok2@currantbun.com>
//					Axel Dörfler <axeld@pinc-software.de>
//	Description:	Graphics functions and variables for the Interface Kit
//
//------------------------------------------------------------------------------

#include <GraphicsDefs.h>

// patterns
const pattern B_SOLID_HIGH = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
const pattern B_MIXED_COLORS = {{0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55}};
const pattern B_SOLID_LOW = {{0, 0, 0, 0, 0, 0, 0, 0}};

// colors
const rgb_color B_TRANSPARENT_COLOR = {0x77, 0x74, 0x77, 0x00};
const rgb_color B_TRANSPARENT_32_BIT = {0x77, 0x74, 0x77, 0x00};
const uint8 B_TRANSPARENT_8_BIT = 0xff;

const uint8 B_TRANSPARENT_MAGIC_CMAP8 = 0xff;
const uint16 B_TRANSPARENT_MAGIC_RGBA15 = 0x39ce;
const uint16 B_TRANSPARENT_MAGIC_RGBA15_BIG = 0xce39;
const uint32 B_TRANSPARENT_MAGIC_RGBA32 = 0x00777477;
const uint32 B_TRANSPARENT_MAGIC_RGBA32_BIG = 0x77747700;

// misc.
const struct screen_id B_MAIN_SCREEN_ID = {0};

// GraphicsDefs.h

_IMPEXP_BE status_t
get_pixel_size_for(color_space space, size_t * pixel_chunk, size_t * row_alignment, size_t * pixels_per_chunk)
{
	// TODO: Implement
}


_IMPEXP_BE bool
bitmaps_support_space(color_space space, uint32 * support_flags)
{
	// TODO: Implement
}
