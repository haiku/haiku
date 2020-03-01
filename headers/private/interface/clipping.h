/*
 * Copyright 2001-2004, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Stefano Ceccherini, burton666@libero.it
 */
#ifndef __CLIPPING_H
#define __CLIPPING_H

#include <Region.h>
#include <SupportDefs.h>


/*	Some methods to manipulate clipping_rects.
	basically you can do almost everything you do with
	BRects, just that clipping_rects can only have integer
	coordinates (a thing that makes these perfect for drawing
	calculations).
*/


// Returns the union of the given rects.
static inline clipping_rect
union_rect(const clipping_rect &r1, const clipping_rect &r2)
{
	clipping_rect rect;

	rect.left = min_c(r1.left, r2.left);
	rect.top = min_c(r1.top, r2.top);
	rect.right = max_c(r1.right, r2.right);
	rect.bottom = max_c(r1.bottom, r2.bottom);

	return rect;
}


// Returns the intersection of the given rects.
// The caller should check if the returned rect is valid. If it isn't valid,
// then the two rectangles don't intersect.
static inline clipping_rect
sect_rect(const clipping_rect &r1, const clipping_rect &r2)
{
	clipping_rect rect;

	rect.left = max_c(r1.left, r2.left);
	rect.top = max_c(r1.top, r2.top);
	rect.right = min_c(r1.right, r2.right);
	rect.bottom = min_c(r1.bottom, r2.bottom);

	return rect;
}


// Adds the given offsets to the given rect.
static inline void
offset_rect(clipping_rect &rect, int32 x, int32 y)
{
	rect.left += x;
	rect.top += y;
	rect.right += x;
	rect.bottom += y;
}


static inline void
scale_rect(clipping_rect& rect, float x, float y)
{
	rect.left = (int)(rect.left * x);
	rect.top = (int)(rect.top * y);
	rect.right = (int)((rect.right + 1) * x) - 1;
	rect.bottom = (int)((rect.bottom + 1) * y) - 1;
}


// Converts the given clipping_rect to a BRect
static inline BRect
to_BRect(const clipping_rect &rect)
{
	return BRect((float)rect.left, (float)rect.top,
				 (float)rect.right, (float)rect.bottom);
}


// Converts the given BRect to a clipping_rect.
static inline clipping_rect
to_clipping_rect(const BRect &rect)
{
	clipping_rect clipRect;

// NOTE: test fractional coords BRects -> BRegion on R5
// and compare with this implementation...
//	clipRect.left = (int32)floorf(rect.left);
//	clipRect.top = (int32)floorf(rect.top);
//	clipRect.right = (int32)ceilf(rect.right);
//	clipRect.bottom = (int32)ceilf(rect.bottom);

	// NOTE: clipping_rects are used as "pixel indices"
	// therefor, it should be ok to convert them like this:
	clipRect.left = (int32)rect.left;
	clipRect.top = (int32)rect.top;
	clipRect.right = (int32)rect.right;
	clipRect.bottom = (int32)rect.bottom;

	return clipRect;
}


// Checks if the given point lies in the given rect's area
static inline bool
point_in(const clipping_rect &rect, int32 px, int32 py)
{
	if (px >= rect.left && px <= rect.right
			&& py >= rect.top && py <= rect.bottom)
		return true;
	return false;
}


// Same as above, but it accepts a BPoint parameter
static inline bool
point_in(const clipping_rect &rect, const BPoint &pt)
{
	if (pt.x >= rect.left && pt.x <= rect.right
			&& pt.y >= rect.top && pt.y <= rect.bottom)
		return true;
	return false;
}


static inline bool
rect_contains(const clipping_rect &rect, const clipping_rect &testRect)
{
	return rect.top <= testRect.top && rect.bottom >= testRect.bottom
			&& rect.left <= testRect.left && rect.right >= testRect.right;
}


// Checks if the rect is valid
static inline bool
valid_rect(const clipping_rect &rect)
{
	if (rect.left <= rect.right && rect.top <= rect.bottom)
		return true;
	return false;
}


// Checks if the two rects intersect.
static inline bool
rects_intersect(const clipping_rect &rectA, const clipping_rect &rectB)
{
	// We behave like BRect::Intersects() does:
	// we return false if one of the two rects is not valid
	if (!valid_rect(rectA) || !valid_rect(rectB))
		return false;

	// TODO: Is there a better algorithm ?
	// the one we used is faster than
	// ' return valid_rect(sect_rect(rectA, rectB)); ', though.

	return !(rectA.left > rectB.right || rectA.top > rectB.bottom
			|| rectA.right < rectB.left || rectA.bottom < rectB.top);
}


// Returns the width of the given rect.
static inline int32
rect_width(const clipping_rect &rect)
{
	return rect.right - rect.left;
}


// Returns the height of the given rect.
static inline int32
rect_height(const clipping_rect &rect)
{
	return rect.bottom - rect.top;
}

#endif // __CLIPPING_H
