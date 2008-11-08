/*
 * Copyright 2006-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#include <Point.h>
#include <Gradient.h>
#include <GradientConic.h>


// constructor
BGradientConic::BGradientConic()
{
	fData.conic.cx = 0.0f;
	fData.conic.cy = 0.0f;
	fData.conic.angle = 0.0f;
	fType = TYPE_CONIC;
}


// constructor
BGradientConic::BGradientConic(const BPoint& center, float angle)
{
	fData.conic.cx = center.x;
	fData.conic.cy = center.y;
	fData.conic.angle = angle;
	fType = TYPE_CONIC;
}


// constructor
BGradientConic::BGradientConic(float cx, float cy, float angle)
{
	fData.conic.cx = cx;
	fData.conic.cy = cy;
	fData.conic.angle = angle;
	fType = TYPE_CONIC;
}


// Center
BPoint
BGradientConic::Center() const
{
	return BPoint(fData.conic.cx, fData.conic.cy);
}


// SetCenter
void
BGradientConic::SetCenter(const BPoint& center)
{
	fData.conic.cx = center.x;
	fData.conic.cy = center.y;
}


// SetCenter
void
BGradientConic::SetCenter(float cx, float cy)
{
	fData.conic.cx = cx;
	fData.conic.cy = cy;
}


// Angle
float
BGradientConic::Angle() const
{
	return fData.conic.angle;
}


// SetAngle
void
BGradientConic::SetAngle(float angle)
{
	fData.conic.angle = angle;
}
