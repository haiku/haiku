//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		ColorUtils.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Miscellaneous useful functions for working with colors
//					
//  
//------------------------------------------------------------------------------
#ifndef COLORUTILS_H_
#define COLORUTILS_H_

#include <GraphicsDefs.h>

void SetRGBColor(rgb_color *col,uint8 r, uint8 g, uint8 b, uint8 a=255);
void SetRGBColor15(rgb_color *col,uint16 color);
void SetRGBColor16(rgb_color *col,uint16 color);
void SetRGBColor(rgb_color *col,uint32 color);

uint8 FindClosestColor(rgb_color *palette, rgb_color color);
uint16 FindClosestColor15(rgb_color color);
uint16 FindClosestColor16(rgb_color color);

rgb_color MakeBlendColor(rgb_color col, rgb_color col2, float position);

#endif
