/*

PDF Writer printer driver.

Copyright (c) 2002 OpenBeOS. 

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

#include <math.h>
#include "Bezier.h"

/*
  Bezier curve of degree n with control points P[0] ... P[n]:
    P(t) = sum i from 0 to n of B(i, n, t) * P[i]
  Bernsteinpolynomial:
    B(i, n, t) = n! / (i! * (n-i)!) * t^i * (1-t)^(n-i)
*/

Bezier::Bezier(BPoint *points, int noOfPoints)
{
	int i;
	fPoints = new BPoint[noOfPoints];
	fBernsteinWeights = new double[noOfPoints];
	fNoOfPoints = noOfPoints;
	const int n = noOfPoints - 1;
	for (i = 0; i < noOfPoints; i++) {
		fPoints[i] = points[i];	
		fBernsteinWeights[i] = Fact(n) / (Fact(i) * Fact(n-i));
	}
}


Bezier::~Bezier() 
{
	delete []fPoints;
	delete []fBernsteinWeights;
}


double 
Bezier::Fact(int n)
{
	double f = 1;
	for (; n > 0; n --) f *= n;
	return f;
}


BPoint 
Bezier::PointAt(float t)
{
	const int n = fNoOfPoints - 1;
	double x, y;
	x = y = 0.0;
	for (int i = 0; i < fNoOfPoints; i ++) {
		double w = fBernsteinWeights[i] * pow(t, i) * pow(1 - t, n - i);
		x += w * fPoints[i].x;
		y += w * fPoints[i].y;
	}
	return BPoint(x, y);
}

