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

static inline clipping_rect
union_rect(clipping_rect r1, clipping_rect r2)
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
sect_rect(clipping_rect r1, clipping_rect r2)
{
	clipping_rect rect;
	
	rect.left = max_c(r1.left, r2.left);
	rect.top = max_c(r1.top, r2.top);
	rect.right = min_c(r1.right, r2.right);
	rect.bottom = min_c(r1.bottom, r2.bottom);
	
	return rect;
}


static inline void
offset_rect(clipping_rect &rect, int32 x, int32 y)
{
	rect.left += x;
	rect.top += y;
	rect.right += x;
	rect.bottom += y;
}


static inline BRect
to_BRect(clipping_rect rect)
{
	return BRect(rect.left, rect.top, rect.right, rect.bottom);
}


static inline clipping_rect
to_clipping_rect(BRect rect)
{
	clipping_rect clipRect;
	
	clipRect.left = (int32)floor(rect.left);
	clipRect.top = (int32)floor(rect.top);
	clipRect.right = (int32)ceil(rect.right);
	clipRect.bottom = (int32)ceil(rect.bottom);
	
	return clipRect;
}


static inline bool
point_in(clipping_rect rect, int32 px, int32 py)
{
	if (px >= rect.left && px <= rect.right 
			&& py >= rect.top && py <= rect.bottom)
		return true;
	return false;
}


static inline bool
point_in(clipping_rect rect, BPoint pt)
{
	if (pt.x >= rect.left && pt.x <= rect.right 
			&& pt.y >= rect.top && pt.y <= rect.bottom)
		return true;
	return false;
}


static inline bool
valid_rect(clipping_rect rect)
{
	if (rect.left <= rect.right && rect.top <= rect.bottom)
		return true;
	return false;
}


static inline bool
rects_intersect(clipping_rect rectA, clipping_rect rectB)
{
	return !(rectA.left > rectB.right || rectA.top > rectB.bottom
			|| rectA.right < rectB.left || rectA.bottom < rectB.top);
}


static inline int32
rect_width(clipping_rect rect)
{
	return rect.right - rect.left;
}


static inline int32
rect_height(clipping_rect rect)
{
	return rect.bottom - rect.top;
}

#endif // __CLIPPING_H
