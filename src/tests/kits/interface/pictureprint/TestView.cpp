/*

TestView

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

#include "TestView.hpp"

static int NextNum() {
	static int n = 0;
	return n ++;
}

static BPoint NextPoint() {
	return BPoint(NextNum(), NextNum());
}

static BRect NextRect() {
	return BRect(NextNum(), NextNum(), NextNum(), NextNum());
}

bool TestView::Test(int no, BString& name) {
	switch (no) {
		case 0: name = "MoveTo"; MoveTo(NextPoint()); break;
		case 1: name = "StrokeLine"; StrokeLine(NextPoint(), NextPoint()); break;
		case 2: name = "StrokeRect"; StrokeRect(NextRect()); break;
		case 3: name = "FillRect"; FillRect(NextRect()); break;
		case 4: name = "DrawChar"; MoveTo(NextPoint()); DrawChar('O'); DrawChar('B'); DrawChar('O'); DrawChar('S'); break;
		case 5: name = "DrawString"; DrawString("OpenBeOS", NextPoint()); break;
		case 6: name = "BlendingMode"; 
			SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
			StrokeRect(NextRect());
			break;
//		case 7: name = ""; break;
/*
		case 6: name = "StrokeEllipse"; break;
		case 7: name = "FillEllipse"; break;
		case 8: name = "StrokeBezier"; break;
		case 9: name = "FillBezier"; break;
		case 10: name = "StrokePolygone"; break;
		case 11: name = "FillPolygone"; break;
		case 12: name = "FillRegion"; break;
		case 13: name = "StrokeTriangle"; break;
		case 14: name = "FillTriangle"; break;
		case 15: name = ""; break;
		case 16: name = ""; break;
		case 17: name = ""; break;
		case 18: name = ""; break;
		case 19: name = ""; break;
		case 20: name = ""; break;
		case 21: name = ""; break;
		case 22: name = ""; break;
*/
		default: return false;
	}
	return true;
}

