#ifndef UTILITY_H
#define UTILITY_H


#include <Rect.h>


inline BPoint
scale_direct(float x, float y, BRect area) 
{
	return BPoint(area.Width() * x + area.left, area.Height() * y + area.top);
}


inline BRect
scale_direct(float x1, float x2, float y1, float y2, BRect area) 
{
	return BRect(area.Width() * x1 + area.left, area.Height() * y1 + area.top,
		area.Width()* x2 + area.left, area.Height() * y2 + area.top);
}

static const float kPositionalX[] = { 0, 0.1, 0.25, 0.3, 0.7, 0.75, 0.9, 1.0 };
static const float kPositionalY[] = { 0, 0.1, 0.7, 0.8, 0.9, 1.0 };

inline BPoint
scale(int x, int y,BRect area)
{
	return scale_direct(kPositionalX[x], kPositionalY[y], area);
}


inline BRect
scale(int x1, int x2, int y1, int y2,BRect area)
{
	return scale_direct(kPositionalX[x1], kPositionalX[x2],
		kPositionalY[y1], kPositionalY[y2], area);
}


#endif	// UTILITY_H
