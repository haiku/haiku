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

#ifndef LINE_PATH_BUILDER_H
#define LINE_PATH_BUILDER_H

#include "SubPath.h"
#include <InterfaceDefs.h>

class LinePathBuilder
{
	SubPath   *fSubPath;
	float      fPenSize;
	cap_mode   fCapMode;
	join_mode  fJoinMode;
	float      fMiterLimit;
	bool       fFirst;

	float     PenSize() const        { return fPenSize; }
	cap_mode  LineCapMode() const    { return fCapMode; }
	join_mode LineJoinMode() const   { return fJoinMode; }
	float     LineMiterLimit() const { return fMiterLimit; }

	bool Vector(BPoint a, BPoint b, float w, BPoint &v);
	bool Parallel(BPoint a, BPoint b, float w, BPoint &pa, BPoint &pb);	
	bool Cut(BPoint a, BPoint b, BPoint c, BPoint d, BPoint &e);
	bool Inside(BPoint a, BPoint b, BPoint r);
	bool InMiterLimit(BPoint a, BPoint b);

	void RoundCap(BPoint a, BPoint b, BPoint ra, BPoint rb, bool start);
	void RoundJoin(BPoint p[4]);
	void AddPoint(BPoint p);
	void Corner(BPoint a, BPoint b, bool start);
	void Connect(BPoint a, BPoint b, BPoint c);

	bool IdenticalPoints(int i, int j);
	void OptimizeSubPath();

	BPoint PointAt(int i);


protected:
	virtual void MoveTo(BPoint p) = 0;
	virtual void LineTo(BPoint p) = 0;
	virtual void BezierTo(BPoint p[3]) = 0;
	virtual void ClosePath(void) = 0;
		
public:
	LinePathBuilder(SubPath *subPath, float penSize, cap_mode capMode, join_mode joinMode, float miterLimit);
	virtual ~LinePathBuilder() { }

	virtual void CreateLinePath();
};

#endif
