/*

SubPath

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

#include <stdio.h>

#include "SubPath.h"


SubPath::SubPath(int size)
{
	fSize   = size <= 0 ? kInitialSize : size;
	fLength = 0;
	fPoints = new BPoint[fSize];
	fClosed = false;
}


SubPath::~SubPath()
{
	delete []fPoints;
}


void 
SubPath::MakeEmpty()
{ 
	fLength = 0; 
	fClosed = false; 
}


void
SubPath::Truncate(int numOfPoints)
{
	fLength -= numOfPoints;
	if (fLength < 0) fLength = 0;
}


void 
SubPath::CheckSize(int size)
{
	if (fSize < size) {
		fSize = size + kIncrement;
		BPoint* points = new BPoint[fSize];
		for (int i = 0; i < fLength; i++) points[i] = fPoints[i];
		delete []fPoints;
		fPoints = points;
	}
}


void 
SubPath::AddPoint(BPoint p)
{
	CheckSize(fLength + 1);
	fPoints[fLength] = p; fLength ++;
}


void 
SubPath::Close() 
{
	fClosed = true;
}


void 
SubPath::Open() 
{
	fClosed = false;
}


BPoint 
SubPath::PointAt(int i)
{
	if (InBounds(i)) {
		return fPoints[i];
	}
	return fPoints[0];
}

void
SubPath::AtPut(int i, BPoint p)
{
	if (InBounds(i)) {
		fPoints[i] = p;
	}
}

void
SubPath::Print()
{
	fprintf(stderr, "SubPath length = %d, size = %d ", (int)fLength, (int)fSize);
	for (int i = 0; i < fLength; i++) {
		if (i != 0) fprintf(stderr, ", ");
		fprintf(stderr, "(%f, %f)", PointAt(i).x, PointAt(i).y);
	}
	fprintf(stderr, "\n");
}
