/*
 * Copyright 2006-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#include <Point.h>
#include <Gradient.h>
#include <GradientLinear.h>


// constructor
BGradientLinear::BGradientLinear()
{
	fData.linear.x1 = 0.0f;
	fData.linear.y1 = 0.0f;
	fData.linear.x2 = 0.0f;
	fData.linear.y2 = 0.0f;
	fType = TYPE_LINEAR;
}


// constructor
BGradientLinear::BGradientLinear(const BPoint& start, const BPoint& end)
{
	fData.linear.x1 = start.x;
	fData.linear.y1 = start.y;
	fData.linear.x2 = end.x;
	fData.linear.y2 = end.y;
	fType = TYPE_LINEAR;
}


// constructor
BGradientLinear::BGradientLinear(float x1, float y1, float x2, float y2)
{
	fData.linear.x1 = x1;
	fData.linear.y1 = y1;
	fData.linear.x2 = x2;
	fData.linear.y2 = y2;
	fType = TYPE_LINEAR;
}


// Start
BPoint
BGradientLinear::Start() const
{
	return BPoint(fData.linear.x1, fData.linear.y1);
}


// SetStart
void
BGradientLinear::SetStart(const BPoint& start)
{
	fData.linear.x1 = start.x;
	fData.linear.y1 = start.y;
}


// SetStart
void
BGradientLinear::SetStart(float x, float y)
{
	fData.linear.x1 = x;
	fData.linear.y1 = y;
}


// End
BPoint
BGradientLinear::End() const
{
	return BPoint(fData.linear.x2, fData.linear.y2);
}


// SetEnd
void
BGradientLinear::SetEnd(const BPoint& end)
{
	fData.linear.x2 = end.x;
	fData.linear.y2 = end.y;
}


// SetEnd
void
BGradientLinear::SetEnd(float x, float y)
{
	fData.linear.x2 = x;
	fData.linear.y2 = y;
}
