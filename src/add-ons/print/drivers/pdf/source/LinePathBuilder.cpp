/*

LinePathBuilder

Copyright (c) 2002 OpenBeOS. 

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

#include "LinePathBuilder.h"

static const float kNearZero   = 0.000000000001;
static const float kMinPenSize = 0.0001;

LinePathBuilder::LinePathBuilder(SubPath *subPath, float penSize, cap_mode capMode, join_mode joinMode, float miterLimit)
	: fSubPath(subPath)
	, fPenSize(penSize)
	, fCapMode(capMode)
	, fJoinMode(joinMode)
	, fMiterLimit(miterLimit)
	, fFirst(true)
{
}


// vector of length w/2 in direction of line [a, b]
bool
LinePathBuilder::Vector(BPoint a, BPoint b, float w, BPoint &v)
{
	v = b - a;
	float l = 2.0 * sqrt(v.x * v.x + v.y * v.y);
	if (fabs(l) <= kNearZero) return false;
	w /= l;
	v.x *= w;
	v.y *= w;	
	return true;
}


// calculate line [pa, pb] parallel to line [a, b] in distance of w/2
bool 
LinePathBuilder::Parallel(BPoint a, BPoint b, float w, BPoint &pa, BPoint &pb)
{
	BPoint d;
	if (!Vector(a, b, w, d)) return false;
	BPoint r(-d.y, d.x);
	
	pa = a - r; 
	pb = b - r; 
	return true;
}


// calculate intercept point e of lines [a, b] and [c, d]
bool 
LinePathBuilder::Cut(BPoint a, BPoint b, BPoint c, BPoint d, BPoint &e)
{
	BPoint r = b - a;
	BPoint s = d - c;
	BPoint v = c - a;
	float div = r.x * s.y - r.y * s.x;
	if (fabs(div) <= kNearZero) return false;
	float u = (r.y * v.x - r.x * v.y) / div;
	e.x = c.x + s.x * u;
	e.y = c.y + s.y * u;
	return true;
}


void 
LinePathBuilder::RoundCap(BPoint a, BPoint b, BPoint ra, BPoint rb, bool start)
{
	BPoint bezier[3];
	BPoint v;
	if (start) { // first quarter circle
		Vector(b, a, PenSize(), v);
		BPoint v055(0.55*v.x, 0.55*v.y);
		BPoint vr055(-v.y, v.x);
		bezier[0] = a + v + vr055;
		bezier[1] = ra + v055;
		bezier[2] = ra;
		AddPoint(a+v);
		BezierTo(bezier);
	} else { // second quarter circle
		Vector(a, b, PenSize(), v);
		BPoint v055(0.55*v.x, 0.55*v.y);
		BPoint vr055(v.y, -v.x);
		bezier[0] = rb + v055;
		bezier[1] = b + v + vr055; 
		bezier[2] = b + v;
		AddPoint(rb);
		BezierTo(bezier);
	}
}


void 
LinePathBuilder::Corner(BPoint a, BPoint b, bool start)
{
	BPoint ra, rb, r;
	if (Parallel(a, b, PenSize(), ra, rb)) {
		BPoint v;
		switch (LineCapMode()) {
			case B_ROUND_CAP:
				RoundCap(a, b, ra, rb, start);
				return;
			case B_BUTT_CAP: // nothing to do
				break;
			case B_SQUARE_CAP:
				Vector(a, b, PenSize(), v);
				ra -= v;
				rb += v;
				break;
		}
		if (start) r = ra; else r = rb;
		AddPoint(r);
	}
}


bool
LinePathBuilder::Inside(BPoint a, BPoint b, BPoint r)
{
	BPoint v;
	Vector(a, b, 2.0, v);
	
	if (kNearZero <= fabs(v.x)) return (r.x - b.x) / v.x <= 0.0;
	else if (kNearZero <= fabs(v.y)) return (r.y - b.y) / v.y <= 0.0;
	else return false; // should not reach here (length of v > 0.0!)
}


bool
LinePathBuilder::InMiterLimit(BPoint a, BPoint b) 
{
	BPoint d = a - b;
	return 2.0*sqrt(d.x*d.x + d.y*d.y)/PenSize() <= LineMiterLimit();
}


void
LinePathBuilder::RoundJoin(BPoint p[4])
{
	BPoint v[2];
	Vector(p[0], p[1], PenSize() * 0.63, v[0]);
	Vector(p[3], p[2], PenSize() * 0.63, v[1]);
	AddPoint(p[1]);
	BPoint bezier[3] = {p[1]+v[0], p[2]+v[1], p[2] };
	BezierTo(bezier);
}


void
LinePathBuilder::Connect(BPoint a, BPoint b, BPoint c)
{
	BPoint p[4], r, v;
	if (Parallel(a, b, PenSize(), p[0], p[1]) &&
		Parallel(b, c, PenSize(), p[2], p[3]) &&
		Cut(p[0], p[1], p[2], p[3], r)) {

		

		if (LineJoinMode() != B_BUTT_JOIN &&
			LineJoinMode() != B_SQUARE_JOIN && 
			Inside(p[0], p[1], r)) {
			if (!Inside(p[1], p[0], r) || !Inside(p[2], p[3], r)) {
				AddPoint(p[1]); AddPoint(p[2]);
			} else {
				AddPoint(r);
			}
			return;
		}
		
		switch (LineJoinMode()) {
			case B_ROUND_JOIN:
				RoundJoin(p);
				break;
			case B_MITER_JOIN:
				if (InMiterLimit(b, r)) {
					AddPoint(r);
					break;
				}
				// fall through
			case B_BEVEL_JOIN:
				AddPoint(p[1]); AddPoint(p[2]);
				break;
			case B_BUTT_JOIN: // ???
				AddPoint(p[1]); AddPoint(b); AddPoint(p[2]);
				break;
			case B_SQUARE_JOIN: // ???
				Vector(a, b, PenSize(), v); 
				AddPoint(p[1] + v); AddPoint(b + v);
				Vector(b, c, PenSize(), v); 
				AddPoint(b - v); AddPoint(p[2] - v);
				break;
		}
	}
}


bool
LinePathBuilder::IdenticalPoints(int i, int j)
{
	BPoint d = fSubPath->PointAt(i) - fSubPath->PointAt(j);
	return d.x * d.x + d.y * d.y < kNearZero;
}


// remove identical points
void
LinePathBuilder::OptimizeSubPath()
{
	int i, j = 0;
	const int n = fSubPath->CountPoints();
	for (i = 1; i < n; i++) {
		if (!IdenticalPoints(i, j)) {
			fSubPath->AtPut(++j, fSubPath->PointAt(i));
		}
	}
	fSubPath->Truncate(i-j-1);
	
	// check also first point == last point if path is closed
	if (fSubPath->IsClosed() && fSubPath->CountPoints() >= 2) {
		const int n = fSubPath->CountPoints()-1;
		if (IdenticalPoints(0, n)) fSubPath->Truncate(1); 
	}
}


BPoint
LinePathBuilder::PointAt(int i) 
{ 
	const int n = fSubPath->CountPoints(); 
	return fSubPath->PointAt((i+n) % n); 
}


void
LinePathBuilder::AddPoint(BPoint p)
{
	if (fFirst) {
		fFirst = false;
		MoveTo(p);
	} else {
		LineTo(p);
	}
}


void 
LinePathBuilder::CreateLinePath()
{
	OptimizeSubPath();
	if (fPenSize < kMinPenSize) fPenSize = kMinPenSize;
	
	const bool start = true;
	const bool end = false;
	const int n = fSubPath->CountPoints();

	if (n < 2) return;

	bool is_closed = fSubPath->IsClosed() && n != 2;

	if (fSubPath->IsClosed() && n == 2) {		
		switch (LineJoinMode()) {
			case B_ROUND_JOIN:
				fCapMode = B_ROUND_CAP;
				break;
			case B_MITER_JOIN: // fall through
			case B_BEVEL_JOIN: // fall through
			case B_BUTT_JOIN:
				fCapMode = B_BUTT_CAP;
				break;
			case B_SQUARE_JOIN:
				fCapMode = B_SQUARE_CAP;
				break;
		}
	}

	// ahead
	if (is_closed) {
		Connect(PointAt(n-1), PointAt(0), PointAt(1));
	} else {
		Corner(PointAt(0), PointAt(1), start);	
	}

	for (int i = 1; i < n-1; i++) {
		Connect(PointAt(i-1), PointAt(i), PointAt(i+1));
	}

	if (is_closed) {
		Connect(PointAt(n-2), PointAt(n-1), PointAt(n));
		Connect(PointAt(n-1), PointAt(n), PointAt(n+1));
	} else {
		Corner(PointAt(n-2), PointAt(n-1), end);	
	}

	// and back
	if (is_closed) {		
		ClosePath(); fFirst = true;
		Connect(PointAt(n+1), PointAt(n), PointAt(n-1));
		Connect(PointAt(n), PointAt(n-1), PointAt(n-2));
	} else {
		Corner(PointAt(n-1), PointAt(n-2), start);	
	}

	for (int i = n-2; i >= 1; i--) {
		Connect(PointAt(i+1), PointAt(i), PointAt(i-1));
	}

	if (is_closed) {
		Connect(PointAt(1), PointAt(0), PointAt(n-1));
	} else {
		Corner(PointAt(1), PointAt(0), end);	
	}
	ClosePath();
}

