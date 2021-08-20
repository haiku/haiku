/*

BezierBounds

Copyright (c) 2002 Haiku.

Author: 
	Michael Pfeiffer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/


#include "BezierBounds.h"
#include <float.h>
#include <math.h>

BRect BezierBounds(BPoint* points, int numOfPoints)
{
	BRect bounds(FLT_MAX, FLT_MAX, -FLT_MIN, -FLT_MIN);
	for (int i = 0; i < numOfPoints; i ++, points ++) {
		bounds.left   = min_c(bounds.left,   points->x);
		bounds.right  = max_c(bounds.right,  points->x);
		bounds.top    = min_c(bounds.top,    points->y);
		bounds.bottom = max_c(bounds.bottom, points->y);
	}
	return bounds;
}

static float PenSizeCorrection(float penSize, cap_mode capMode, join_mode joinMode, float miterLimit)
{
	const float halfPenSize = penSize / 2.0;
	float correction = 0.0;
	
	switch (capMode) {
		case B_ROUND_CAP:
		case B_BUTT_CAP:
			correction = halfPenSize;			
			break;
		case B_SQUARE_CAP:
			correction = M_SQRT2 * halfPenSize;
			break;			
	}
	
	switch (joinMode) {
		case B_ROUND_JOIN:
		case B_BEVEL_JOIN:
		case B_BUTT_JOIN:
			correction = max_c(correction, halfPenSize);
			break;
		case B_MITER_JOIN:
			correction = max_c(correction, halfPenSize * miterLimit);
			break;
		case B_SQUARE_JOIN:
			correction = max_c(correction, M_SQRT2 * halfPenSize);
			break;
	}
	
	return correction;
}

BRect BezierBounds(BPoint* points, int numOfPoints, float penSize, cap_mode capMode, join_mode joinMode, float miterLimit)
{
	BRect bounds = BezierBounds(points, numOfPoints);
	float w = -PenSizeCorrection(penSize, capMode, joinMode, miterLimit);
	bounds.InsetBy(w, w);
	return bounds;
}


