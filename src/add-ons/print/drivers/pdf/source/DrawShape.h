/*

DrawShape

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

#ifndef DRAW_SHAPE_H
#define DRAW_SHAPE_H

#include "PDFWriter.h"

class DrawShape : public BShapeIterator
{
	PDFWriter *fWriter;
	bool       fStroke;
	bool       fDrawn;
	BPoint     fCurrentPoint;
	SubPath    fSubPath;
	
	inline FILE *Log()			{ return fWriter->fLog; }
	inline PDF *Pdf()			{ return fWriter->fPdf; }
	inline float tx(float x)	{ return fWriter->tx(x); }
	inline float ty(float y)	{ return fWriter->ty(y); }
	inline float scale(float f) { return fWriter->scale(f); }

	inline bool IsDrawing() const  { return fWriter->IsDrawing(); }
	inline bool IsClipping() const { return fWriter->IsClipping(); }

	inline bool IsStroking() const { return fStroke; }
	inline bool IsFilling() const  { return !fStroke; }

	inline float PenSize() const { return fWriter->PenSize(); }
	inline bool TransformPath() const { return IsStroking() && IsClipping(); }

	enum {
		kMinBezierPoints = 2, // must be greater or equal to 2
		kMaxBezierPoints = 30
	};

	float BezierLength(BPoint *p, int n);	
	int   BezierPoints(BPoint *p, int n);
	void  CreateBezierPath(BPoint *p);
	void  EndSubPath();
	
public:
	DrawShape(PDFWriter *writer, bool stroke);
	~DrawShape();
	status_t IterateBezierTo(int32 bezierCount, BPoint *bezierPoints);
	status_t IterateClose(void);
	status_t IterateLineTo(int32 lineCount, BPoint *linePoints);
	status_t IterateMoveTo(BPoint *point);
	
	void Draw();
};

#endif
