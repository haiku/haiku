/*

PDFLinePathBuilder

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

#include "PDFLinePathBuilder.h"

PDFLinePathBuilder::PDFLinePathBuilder(SubPath *subPath, PDFWriter *writer)
	: LinePathBuilder(subPath, writer->PenSize(), writer->LineCapMode(), writer->LineJoinMode(), writer->LineMiterLimit())
	, fWriter(writer)
{
}

void 
PDFLinePathBuilder::MoveTo(BPoint p)
{
	PDF_moveto(Pdf(), tx(p.x), ty(p.y));
}

void 
PDFLinePathBuilder::LineTo(BPoint p)
{
	PDF_lineto(Pdf(), tx(p.x), ty(p.y));
}

void 
PDFLinePathBuilder::BezierTo(BPoint p[3])
{
	PDF_curveto(Pdf(),
		tx(p[0].x), ty(p[0].y),
		tx(p[1].x), ty(p[1].y),
		tx(p[2].x), ty(p[2].y)); 
}

void 
PDFLinePathBuilder::ClosePath(void)
{
	PDF_closepath(Pdf());
}


