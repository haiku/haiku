/*

PDF Writer printer driver.

Copyright (c) 2001-2003 OpenBeOS. 
Copyright (c) 2006 Haiku.

Authors: 
	Philippe Houdoin
	Simon Gauvin	
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

#ifndef _PDF_SYSTEM_H
#define _PDF_SYSTEM_H

// PDF coordinate system
class PDFSystem {
private:
	float fHeight;
	float fX;
	float fY;
	float fScale;

public:
	PDFSystem()
		: fHeight(0), fX(0), fY(0), fScale(1) { }
	PDFSystem(float h, float x, float y, float s)
		: fHeight(h), fX(x), fY(y), fScale(s) { }

	void   SetHeight(float h)          { fHeight = h; }
	void   SetOrigin(float x, float y) { fX = x; fY = y; }
	void   SetScale(float scale)       { fScale = scale; }
	float  Height() const              { return fHeight; }
	BPoint Origin() const              { return BPoint(fX, fY); }
	float  Scale() const               { return fScale; }
	
	inline float tx(float x)    { return fX + fScale*x; }
	inline float ty(float y)    { return fHeight - (fY + fScale * y); }
	inline float scale(float f) { return fScale * f; }
};


#endif
