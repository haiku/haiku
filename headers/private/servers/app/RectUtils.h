//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//	File Name:		RectUtils.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Utilities for work around R5 Interface Kit bugs
//  
//------------------------------------------------------------------------------
#ifndef RECTUTILS_H_
#define RECTUTILS_H_

#include <Rect.h>
#include <Region.h>

bool TestLineIntersection(const BRect&r, float x1, float y1, float x2, float y2,bool vertical=true);
bool TestRectIntersection(const BRect &r,const BRect &r2);
bool TestRegionIntersection(BRegion *r,const BRect &r2);
void IntersectRegionWith(BRegion *r,const BRect &r2);
void ValidateRect(BRect *r);
#endif
